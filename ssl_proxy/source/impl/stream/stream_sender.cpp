#include "stream_sender.hpp"

namespace ams::ssl::mitm::stream {

Sender::Sender(Queue &queue, Transport &transport, std::uint64_t session_id)
    : m_queue(queue), m_transport(transport), m_session_id(session_id) {}

void Sender::Disconnect() {
    if (m_connected) {
        m_transport.Close();
        m_connected = false;
    }
}

bool Sender::SendFrame(const Frame &frame) {
    std::size_t offset = 0;
    while (offset < frame.size) {
        const auto sent = m_transport.Send(frame.bytes + offset,
                                           frame.size - offset);
        if (sent <= 0 || static_cast<std::size_t>(sent) > frame.size - offset) {
            Disconnect();
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

bool Sender::SendHeartbeat() {
    if (!m_connected) return false;
    Frame hello{};
    const Event event{FrameType::Hello, Direction::None, m_session_id,
                      0, 0, 0, 0};
    return EncodeFrame(hello, event, nullptr, 0) && SendFrame(hello);
}

bool Sender::Pump() {
    Frame frame{};
    if (!m_connected) {
        if (!m_transport.Connect())
            return false;
        m_connected = true;
        if (!SendHeartbeat())
            return false;
    }

    const auto counters = m_queue.Snapshot();
    if (counters.dropped_events != m_reported_events ||
        counters.dropped_bytes != m_reported_bytes) {
        const Event event{FrameType::Loss, Direction::None, m_session_id,
                          0, 0, 0, 0};
        if (!EncodeLoss(frame, event, counters.dropped_events,
                        counters.dropped_bytes) || !SendFrame(frame))
            return false;
        m_reported_events = counters.dropped_events;
        m_reported_bytes = counters.dropped_bytes;
    }

    if (!m_queue.TryPop(frame))
        return true;
    if (!SendFrame(frame)) {
        // The peer will discard a truncated frame. A complete send does not
        // prove that the peer durably saved it; only failed sends are counted.
        const std::size_t payload_size = frame.size - 4 - HeaderSize;
        m_queue.RecordDrop(payload_size);
        return false;
    }
    return true;
}

} // namespace ams::ssl::mitm::stream
