#pragma once

#include "stream_queue.hpp"

namespace ams::ssl::mitm::stream {

// Connect and Send are used only by the sender worker. A failed or zero-byte
// Send terminates this TCP connection; the next Pump starts a new stream.
class Transport {
  public:
    virtual ~Transport() = default;
    virtual bool Connect() = 0;
    virtual std::ptrdiff_t Send(const void *data, std::size_t size) = 0;
    virtual void Close() = 0;
};

class Sender {
  public:
    Sender(Queue &queue, Transport &transport, std::uint64_t session_id);
    bool Pump();
    bool SendHeartbeat();
    bool IsConnected() const { return m_connected; }

  private:
    bool SendFrame(const Frame &frame);
    void Disconnect();
    Queue &m_queue;
    Transport &m_transport;
    std::uint64_t m_session_id;
    bool m_connected = false;
    std::uint64_t m_reported_events = 0;
    std::uint64_t m_reported_bytes = 0;
};

// Starts the single Switch worker. Failure leaves SSL forwarding operational.
bool StartSender(std::uint32_t destination_ipv4, std::uint16_t destination_port,
                 std::uint64_t session_id);

} // namespace ams::ssl::mitm::stream
