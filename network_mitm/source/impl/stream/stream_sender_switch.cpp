#include "stream_sender.hpp"

#include <stratosphere.hpp>
#include <cerrno>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

extern "C" {
#include <switch/services/bsd.h>
}

namespace ams::ssl::mitm::stream {
namespace {
constexpr std::size_t SocketPoolSize = 256_KB;
alignas(os::MemoryPageSize) u8 g_socket_pool[SocketPoolSize];
// Pump may hold a full data frame alongside hello/loss control frames.
alignas(os::ThreadStackAlignment) u8 g_sender_stack[64_KB];
os::ThreadType g_sender_thread;
std::uint32_t g_destination_ipv4;
std::uint16_t g_destination_port;
std::uint64_t g_session_id;

class SocketTransport final : public Transport {
  public:
    SocketTransport(std::uint32_t ip, std::uint16_t port)
        : m_ip(ip), m_port(port) {}

    bool Connect() override {
        Close();
        m_fd = bsdSocket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (m_fd < 0) return false;
        sockaddr_in address{};
        address.sin_len = sizeof(address);
        address.sin_family = AF_INET;
        address.sin_port = socket::InetHtons(m_port);
        address.sin_addr.s_addr = socket::InetHtonl(m_ip);
        const int result = bsdConnect(m_fd,
            reinterpret_cast<const sockaddr *>(&address), sizeof(address));
        if (result == 0) return true;
        if (g_bsdErrno != EINPROGRESS && g_bsdErrno != EWOULDBLOCK) {
            Close();
            return false;
        }
        pollfd wait{m_fd, POLLOUT, 0};
        if (bsdPoll(&wait, 1, 500) != 1 || (wait.revents & POLLOUT) == 0) {
            Close();
            return false;
        }
        int socket_error = 0;
        socklen_t error_size = sizeof(socket_error);
        if (bsdGetSockOpt(m_fd, SOL_SOCKET, SO_ERROR, &socket_error,
                          &error_size) != 0 || socket_error != 0) {
            Close();
            return false;
        }
        return true;
    }

    std::ptrdiff_t Send(const void *data, std::size_t size) override {
        pollfd wait{m_fd, POLLOUT, 0};
        if (bsdPoll(&wait, 1, 500) != 1 || (wait.revents & POLLOUT) == 0)
            return -1;
        return bsdSend(m_fd, data, size, MSG_DONTWAIT);
    }

    void Close() override {
        if (m_fd >= 0) {
            bsdClose(m_fd);
            m_fd = -1;
        }
    }

    ~SocketTransport() override { Close(); }

  private:
    std::uint32_t m_ip;
    std::uint16_t m_port;
    int m_fd = -1;
};

void SenderThread(void *) {
    BsdInitConfig config{};
    config.version = 7;
    config.tmem_buffer = g_socket_pool;
    config.tmem_buffer_size = sizeof(g_socket_pool);
    config.tcp_tx_buf_size = 16_KB;
    config.tcp_rx_buf_size = 16_KB;
    config.tcp_tx_buf_max_size = 0;
    config.tcp_rx_buf_max_size = 0;
    config.udp_tx_buf_size = 0;
    config.udp_rx_buf_size = 0;
    config.sb_efficiency = 1;
    const Result initialized = bsdInitialize(&config, 1, 1 << 1);
    if (R_FAILED(initialized)) {
        AMS_LOG("TCP stream BSD init failed: %d-%d\n",
                initialized.GetModule() + 2000, initialized.GetValue());
        return;
    }

    SocketTransport transport(g_destination_ipv4, g_destination_port);
    Sender sender(g_stream_queue, transport, g_session_id);
    u32 backoff_ms = 250;
    u64 last_heartbeat_ns = 0;
    for (;;) {
        const bool was_connected = sender.IsConnected();
        bool healthy = sender.Pump();
        if (healthy) {
            const u64 now_ns = static_cast<u64>(
                os::GetSystemTick().ToTimeSpan().GetNanoSeconds());
            if (!was_connected) {
                last_heartbeat_ns = now_ns;
            } else if (now_ns - last_heartbeat_ns >= 10'000'000'000ULL) {
                healthy = sender.SendHeartbeat();
                if (healthy) last_heartbeat_ns = now_ns;
            }
        }
        if (healthy) {
            backoff_ms = 250;
            os::SleepThread(TimeSpan::FromMilliSeconds(10));
        } else {
            os::SleepThread(TimeSpan::FromMilliSeconds(backoff_ms));
            if (backoff_ms < 2000) backoff_ms *= 2;
        }
    }
}
} // namespace

bool StartSender(std::uint32_t ip, std::uint16_t port,
                 std::uint64_t session_id) {
    if (ip == 0 || port == 0) return false;
    g_destination_ipv4 = ip;
    g_destination_port = port;
    g_session_id = session_id;
    const Result created = os::CreateThread(&g_sender_thread, SenderThread,
        nullptr, g_sender_stack, sizeof(g_sender_stack), 40);
    if (R_FAILED(created)) return false;
    os::StartThread(&g_sender_thread);
    return true;
}
} // namespace ams::ssl::mitm::stream
