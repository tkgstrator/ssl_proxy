#include "../network_mitm/source/impl/stream/stream_sender.hpp"
#include <cassert>
#include <cstring>
#include <vector>

using namespace ams::ssl::mitm::stream;

struct FakeTransport final : Transport {
    bool available = true;
    int connect_count = 0;
    int close_count = 0;
    std::size_t max_chunk = 3;
    int fail_after = -1;
    int calls = 0;
    std::vector<std::uint8_t> received;

    bool Connect() override { ++connect_count; return available; }
    std::ptrdiff_t Send(const void *data, std::size_t size) override {
        if (calls++ == fail_after) return -1;
        const auto *p = static_cast<const std::uint8_t *>(data);
        const auto n = size < max_chunk ? size : max_chunk;
        received.insert(received.end(), p, p + n);
        return static_cast<std::ptrdiff_t>(n);
    }
    void Close() override { ++close_count; }
};

static Event DataEvent() {
    return {FrameType::Data, Direction::ClientToServer, 99, 5, 7, 1, 12};
}

int main() {
    Queue queue;
    FakeTransport transport;
    Sender sender(queue, transport, 99);
    assert(sender.Pump()); // Connect and announce an idle module.
    assert(transport.connect_count == 1);
    assert(transport.received.size() == 56);
    assert(transport.received[10] == 1); // hello

    const char payload[] = "abc";
    assert(EnqueueIo(queue, DataEvent(), payload, 3).accepted_events == 1);
    assert(sender.Pump());
    assert(transport.received.size() == 56 + 59);
    assert(transport.received[56 + 10] == 2);
    assert(std::memcmp(transport.received.data() + 56 + 56, payload, 3) == 0);
    assert(sender.SendHeartbeat());
    assert(transport.received.size() == 56 + 59 + 56);
    assert(transport.received[56 + 59 + 10] == 1);
    assert(transport.received[56 + 59 + 12 + 7] == 99);

    Queue unavailable_queue;
    FakeTransport unavailable;
    unavailable.available = false;
    Sender waiting(unavailable_queue, unavailable, 100);
    assert(!waiting.Pump());
    assert(unavailable.received.empty());
    Frame full{};
    assert(EncodeFrame(full, DataEvent(), payload, 3));
    for (std::size_t i = 0; i < Queue::Capacity; ++i) assert(unavailable_queue.TryPush(full));
    assert(EnqueueIo(unavailable_queue, DataEvent(), payload, 3).dropped_events == 1);
    unavailable.available = true;
    assert(waiting.Pump());
    assert(unavailable.received.size() >= 56 + 72);
    assert(unavailable.received[56 + 10] == 3); // cumulative loss precedes queued data
    assert(unavailable.received[56 + 63] == 1);

    Queue interrupted_queue;
    FakeTransport interrupted;
    interrupted.max_chunk = 2;
    Sender reconnecting(interrupted_queue, interrupted, 101);
    assert(reconnecting.Pump());
    assert(EnqueueIo(interrupted_queue, DataEvent(), payload, 3).accepted_events == 1);
    interrupted.fail_after = interrupted.calls + 1; // fail after a partial data send
    assert(!reconnecting.Pump());
    assert(interrupted.close_count == 1);
    assert(interrupted_queue.Snapshot().dropped_events == 1);
    interrupted.fail_after = -1;
    interrupted.received.clear();
    assert(reconnecting.Pump());
    assert(interrupted.connect_count == 2);
    assert(interrupted.received[10] == 1);
    assert(interrupted.received[56 + 10] == 3);
    assert(interrupted.received[56 + 63] == 1);
}
