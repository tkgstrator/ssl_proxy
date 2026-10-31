#include "stream_config.hpp"

namespace ams::ssl::mitm::stream {
Config g_stream_config{};
namespace {
bool ParseIpv4(const char *text, std::size_t length, std::uint32_t &out) {
    if (!text || !length || length > 16)
        return false;
    if (text[length - 1] == 0)
        --length;
    if (!length || length > 15)
        return false;
    std::uint32_t value = 0;
    std::size_t position = 0;
    for (unsigned part = 0; part < 4; ++part) {
        if (position == length)
            return false;
        unsigned octet = 0;
        unsigned digits = 0;
        while (position < length && text[position] >= '0' &&
               text[position] <= '9') {
            octet = octet * 10 + static_cast<unsigned>(text[position++] - '0');
            if (++digits > 3 || octet > 255)
                return false;
        }
        if (!digits)
            return false;
        value = (value << 8) | octet;
        if (part != 3) {
            if (position == length || text[position++] != '.')
                return false;
        }
    }
    const unsigned first_octet = value >> 24;
    if (position != length || first_octet == 0 || first_octet == 127 ||
        first_octet >= 224 || value == 0xFFFFFFFFu)
        return false;
    out = value;
    return true;
}
} // namespace

Config ParseConfig(bool enabled, const char *host, std::size_t host_size,
                   std::uint32_t port, bool port_present) {
    Config out{false, 0, 0};
    if (!enabled || !port_present || port == 0 || port > 65535)
        return out;
    std::uint32_t ipv4 = 0;
    if (!ParseIpv4(host, host_size, ipv4))
        return out;
    out.active = true;
    out.destination_ipv4 = ipv4;
    out.destination_port = static_cast<std::uint16_t>(port);
    return out;
}
} // namespace ams::ssl::mitm::stream
