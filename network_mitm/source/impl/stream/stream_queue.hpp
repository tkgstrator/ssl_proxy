#pragma once

#include "stream_protocol.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace ams::ssl::mitm::stream {

struct Counters {
    std::uint64_t accepted_events;
    std::uint64_t accepted_bytes;
    std::uint64_t dropped_events;
    std::uint64_t dropped_bytes;
};

enum class PushStatus { Accepted, Full, Busy, Invalid };

class Queue {
  public:
    static constexpr std::size_t Capacity = 8;

    bool TryPush(const Frame &frame);
    PushStatus TryPushEvent(const Event &event, const void *payload,
                            std::size_t payload_size);
    bool TryPop(Frame &frame);
    void RecordDrop(std::size_t payload_bytes);
    void RecordDrops(std::uint64_t events, std::uint64_t payload_bytes);
    Counters Snapshot() const;

  private:
    std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
    Frame m_frames[Capacity]{};
    std::size_t m_head = 0;
    std::size_t m_count = 0;
    std::atomic<std::uint64_t> m_accepted_events{0};
    std::atomic<std::uint64_t> m_accepted_bytes{0};
    std::atomic<std::uint64_t> m_dropped_events{0};
    std::atomic<std::uint64_t> m_dropped_bytes{0};
};

struct EnqueueResult {
    std::uint64_t accepted_events;
    std::uint64_t accepted_bytes;
    std::uint64_t dropped_events;
    std::uint64_t dropped_bytes;
};

EnqueueResult EnqueueIo(Queue &queue, Event event, const void *payload,
                        std::size_t payload_size);

class ConnectionProducer {
  public:
    EnqueueResult Enqueue(Queue &queue, Event event, const void *payload,
                          std::size_t payload_size);

  private:
    std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
    std::atomic<std::uint64_t> m_next_sequence{0};
};

extern Queue g_stream_queue;
extern std::uint64_t g_stream_session_id;
std::uint64_t NextConnectionId();

} // namespace ams::ssl::mitm::stream
