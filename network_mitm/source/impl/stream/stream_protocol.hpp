#pragma once

#include <cstddef>
#include <cstdint>

namespace ams::ssl::mitm::stream {

constexpr std::size_t MaxPayloadSize = 8192;
constexpr std::size_t HeaderSize = 52;
constexpr std::size_t MaxFrameSize = 4 + HeaderSize + MaxPayloadSize;

enum class FrameType : std::uint8_t { Hello = 1, Data = 2, Loss = 3 };
enum class Direction : std::uint8_t {
    None = 0,
    ClientToServer = 1,
    ServerToClient = 2,
};

struct Event {
    FrameType type;
    Direction direction;
    std::uint64_t session_id;
    std::uint64_t title_id;
    std::uint64_t connection_id;
    std::uint64_t sequence;
    std::uint64_t tick_ns;
};

struct Frame {
    std::uint8_t bytes[MaxFrameSize];
    std::size_t size;
};

bool EncodeFrame(Frame &out, const Event &event, const void *payload,
                 std::size_t payload_size);
bool EncodeLoss(Frame &out, const Event &event, std::uint64_t dropped_events,
                std::uint64_t dropped_bytes);

} // namespace ams::ssl::mitm::stream
