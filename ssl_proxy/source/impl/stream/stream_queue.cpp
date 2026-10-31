#include "stream_queue.hpp"

namespace ams::ssl::mitm::stream {

Queue g_stream_queue;
std::uint64_t g_stream_session_id = 0;
namespace {
std::atomic<std::uint64_t> g_next_connection_id{1};
}

std::uint64_t NextConnectionId() {
    return g_next_connection_id.fetch_add(1, std::memory_order_relaxed);
}

bool Queue::TryPush(const Frame &frame) {
    if (frame.size < 4 + HeaderSize || frame.size > MaxFrameSize)
        return false;
    if (m_lock.test_and_set(std::memory_order_acquire))
        return false;
    if (m_count == Capacity) {
        m_lock.clear(std::memory_order_release);
        return false;
    }
    m_frames[(m_head + m_count) % Capacity] = frame;
    ++m_count;
    m_lock.clear(std::memory_order_release);
    m_accepted_events.fetch_add(1, std::memory_order_relaxed);
    m_accepted_bytes.fetch_add(frame.size - 4 - HeaderSize,
                               std::memory_order_relaxed);
    return true;
}

PushStatus Queue::TryPushEvent(const Event &event, const void *payload,
                               std::size_t payload_size) {
    if (!payload || payload_size > MaxPayloadSize ||
        event.type != FrameType::Data)
        return PushStatus::Invalid;
    if (m_lock.test_and_set(std::memory_order_acquire))
        return PushStatus::Busy;
    if (m_count == Capacity) {
        m_lock.clear(std::memory_order_release);
        return PushStatus::Full;
    }
    Frame &slot = m_frames[(m_head + m_count) % Capacity];
    if (!EncodeFrame(slot, event, payload, payload_size)) {
        m_lock.clear(std::memory_order_release);
        return PushStatus::Invalid;
    }
    ++m_count;
    m_lock.clear(std::memory_order_release);
    m_accepted_events.fetch_add(1, std::memory_order_relaxed);
    m_accepted_bytes.fetch_add(payload_size, std::memory_order_relaxed);
    return PushStatus::Accepted;
}

bool Queue::TryPop(Frame &frame) {
    if (m_lock.test_and_set(std::memory_order_acquire))
        return false;
    if (m_count == 0) {
        m_lock.clear(std::memory_order_release);
        return false;
    }
    frame = m_frames[m_head];
    m_head = (m_head + 1) % Capacity;
    --m_count;
    m_lock.clear(std::memory_order_release);
    return true;
}

void Queue::RecordDrop(std::size_t payload_bytes) {
    RecordDrops(1, payload_bytes);
}

void Queue::RecordDrops(std::uint64_t events,
                        std::uint64_t payload_bytes) {
    m_dropped_events.fetch_add(events, std::memory_order_relaxed);
    m_dropped_bytes.fetch_add(payload_bytes, std::memory_order_relaxed);
}

Counters Queue::Snapshot() const {
    return {m_accepted_events.load(std::memory_order_relaxed),
            m_accepted_bytes.load(std::memory_order_relaxed),
            m_dropped_events.load(std::memory_order_relaxed),
            m_dropped_bytes.load(std::memory_order_relaxed)};
}

EnqueueResult EnqueueIo(Queue &queue, Event event, const void *payload,
                        std::size_t payload_size) {
    EnqueueResult result{};
    if (!payload_size || event.type != FrameType::Data)
        return result;
    const auto *bytes = static_cast<const std::uint8_t *>(payload);
    for (std::size_t offset = 0; offset < payload_size;) {
        const std::size_t chunk =
            payload_size - offset > MaxPayloadSize ? MaxPayloadSize
                                                   : payload_size - offset;
        const PushStatus status = queue.TryPushEvent(
            event, bytes ? bytes + offset : nullptr, chunk);
        if (status == PushStatus::Accepted) {
            ++result.accepted_events;
            result.accepted_bytes += chunk;
        } else if (status == PushStatus::Full) {
            const std::size_t remaining = payload_size - offset;
            const std::uint64_t remaining_events =
                remaining / MaxPayloadSize + (remaining % MaxPayloadSize != 0);
            queue.RecordDrops(remaining_events, remaining);
            result.dropped_events += remaining_events;
            result.dropped_bytes += remaining;
            break;
        } else {
            queue.RecordDrop(chunk);
            ++result.dropped_events;
            result.dropped_bytes += chunk;
        }
        offset += chunk;
        ++event.sequence;
    }
    return result;
}


EnqueueResult ConnectionProducer::Enqueue(Queue &queue, Event event,
                                          const void *payload,
                                          std::size_t payload_size) {
    if (!payload_size || event.type != FrameType::Data)
        return {};
    const std::uint64_t segments =
        payload_size / MaxPayloadSize + (payload_size % MaxPayloadSize != 0);
    if (m_lock.test_and_set(std::memory_order_acquire)) {
        m_next_sequence.fetch_add(segments, std::memory_order_relaxed);
        queue.RecordDrops(segments, payload_size);
        return {0, 0, segments, payload_size};
    }
    event.sequence = m_next_sequence.fetch_add(segments,
                                                std::memory_order_relaxed);
    const EnqueueResult result = EnqueueIo(queue, event, payload, payload_size);
    m_lock.clear(std::memory_order_release);
    return result;
}
} // namespace ams::ssl::mitm::stream
