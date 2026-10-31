#include "../network_mitm/source/impl/stream/stream_config.hpp"
#include "../network_mitm/source/impl/stream/stream_protocol.hpp"
#include "../network_mitm/source/impl/stream/stream_queue.hpp"
#include <cassert>
#include <cstring>
#include <atomic>
#include <thread>
#include <sys/mman.h>

using namespace ams::ssl::mitm::stream;

int main() {
    auto disabled = ParseConfig(false, "192.168.1.7", 11, 4567, true);
    assert(!disabled.active);
    auto active = ParseConfig(true, "192.168.1.7", 11, 4567, true);
    assert(active.active && active.destination_ipv4 == 0xC0A80107);
    assert(active.destination_port == 4567);
    assert(!ParseConfig(true, nullptr, 0, 4567, true).active);
    assert(!ParseConfig(true, "192.168.1.999", 13, 4567, true).active);
    assert(!ParseConfig(true, "224.0.0.1", 9, 4567, true).active);
    assert(!ParseConfig(true, "255.255.255.255", 15, 4567, true).active);
    assert(!ParseConfig(true, "192.168.1.7", 11, 0, true).active);
    assert(!ParseConfig(true, "192.168.1.7", 11, 65536, true).active);
    assert(!ParseConfig(true, "192.168.1.7", 11, 4567, false).active);

    Frame frame{};
    Event event{FrameType::Data, Direction::ClientToServer, 0x0102030405060708ULL,
        0x1112131415161718ULL, 3, 4, 5};
    const unsigned char payload[] = {0xAA, 0xBB};
    assert(EncodeFrame(frame, event, payload, 2));
    assert(frame.size == 58);
    const unsigned char fixture[] = {
        0x00,0x00,0x00,0x36, 0x4e,0x4d,0x54,0x53, 0x00,0x01,0x02,0x01,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,
        0x00,0x00,0x00,0x02, 0xaa,0xbb,
    };
    assert(std::memcmp(frame.bytes, fixture, sizeof(fixture)) == 0);
    assert(!EncodeFrame(frame, event, payload, 8193));
    Event hello{FrameType::Hello, Direction::None, 1, 0, 0, 0, 0};
    assert(EncodeFrame(frame, hello, nullptr, 0));
    assert(frame.size == 56 && frame.bytes[10] == 1 && frame.bytes[55] == 0);
    assert(EncodeLoss(frame, hello, 2, 8193));
    assert(frame.size == 72 && frame.bytes[10] == 3);
    assert(frame.bytes[63] == 2 && frame.bytes[70] == 0x20 && frame.bytes[71] == 1);

    Queue queue;
    unsigned char bytes[8193]{};
    auto result = EnqueueIo(queue, event, bytes, sizeof(bytes));
    assert(result.accepted_bytes == 8193 && result.accepted_events == 2);
    Frame first{}, second{};
    assert(queue.TryPop(first) && queue.TryPop(second));
    assert(first.size == 8248 && second.size == 57);
    assert(first.bytes[55] == 0 && second.bytes[55] == 1);

    for (unsigned i=0; i<Queue::Capacity; ++i) assert(queue.TryPush(first));
    auto overflow = EnqueueIo(queue, event, payload, sizeof(payload));
    assert(overflow.dropped_events == 1 && overflow.dropped_bytes == 2);
    auto counters = queue.Snapshot();
    assert(counters.dropped_events == 1 && counters.dropped_bytes == 2);
    void *unreadable = mmap(nullptr, 4096, PROT_NONE,
                            MAP_PRIVATE | MAP_ANON, -1, 0);
    assert(unreadable != MAP_FAILED);
    auto saturated = EnqueueIo(queue, event, unreadable, 8192);
    assert(saturated.dropped_events == 1 && saturated.dropped_bytes == 8192);
    assert(munmap(unreadable, 4096) == 0);
    assert(EnqueueIo(queue, event, nullptr, 0).accepted_events == 0);
    Queue invalid_source;
    auto missing = EnqueueIo(invalid_source, event, nullptr, 2);
    assert(missing.dropped_events == 1 && missing.dropped_bytes == 2);
    Queue partial;
    for (unsigned i=0; i<Queue::Capacity-1; ++i) assert(partial.TryPush(first));
    auto divided = EnqueueIo(partial, event, bytes, sizeof(bytes));
    assert(divided.accepted_events == 1 && divided.accepted_bytes == 8192);
    assert(divided.dropped_events == 1 && divided.dropped_bytes == 1);

    Queue concurrent;
    ConnectionProducer producer;
    std::atomic<bool> start{false};
    EnqueueResult outcomes[4]{};
    std::thread workers[4];
    for (int i=0; i<4; ++i) {
        workers[i] = std::thread([&, i] {
            while (!start.load(std::memory_order_acquire)) {}
            outcomes[i] = producer.Enqueue(concurrent, event, bytes, sizeof(bytes));
        });
    }
    start.store(true, std::memory_order_release);
    for (auto &worker : workers) worker.join();
    std::uint64_t accounted_events = 0, accounted_bytes = 0;
    for (const auto &outcome : outcomes) {
        accounted_events += outcome.accepted_events + outcome.dropped_events;
        accounted_bytes += outcome.accepted_bytes + outcome.dropped_bytes;
    }
    assert(accounted_events == 8 && accounted_bytes == 4 * sizeof(bytes));
    auto observed = concurrent.Snapshot();
    assert(observed.accepted_events + observed.dropped_events == 8);
    assert(observed.accepted_bytes + observed.dropped_bytes == 4 * sizeof(bytes));
    std::uint64_t previous_sequence = 0;
    bool first_sequence = true;
    Frame queued{};
    while (concurrent.TryPop(queued)) {
        std::uint64_t sequence = 0;
        for (int i=36; i<44; ++i) sequence = (sequence << 8) | queued.bytes[i];
        assert(first_sequence || sequence > previous_sequence);
        previous_sequence = sequence;
        first_sequence = false;
    }

    Queue gap_queue;
    ConnectionProducer gap_producer;
    for (unsigned i=0; i<Queue::Capacity; ++i) assert(gap_queue.TryPush(first));
    auto dropped_call = gap_producer.Enqueue(gap_queue, event, payload, sizeof(payload));
    assert(dropped_call.dropped_events == 1);
    while (gap_queue.TryPop(queued)) {}
    auto next_call = gap_producer.Enqueue(gap_queue, event, payload, sizeof(payload));
    assert(next_call.accepted_events == 1 && gap_queue.TryPop(queued));
    assert(queued.bytes[43] == 1);
}
