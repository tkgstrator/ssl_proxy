#pragma once

#include <cstddef>
#include <cstdint>

namespace ams::ssl::mitm::stream {

struct Config {
    bool active;
    std::uint32_t destination_ipv4; // Network byte order represented as a host integer.
    std::uint16_t destination_port;
};

Config ParseConfig(bool enabled, const char *host, std::size_t host_size,
                   std::uint32_t port, bool port_present);

extern Config g_stream_config;

} // namespace ams::ssl::mitm::stream
