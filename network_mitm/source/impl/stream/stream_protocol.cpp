#include "stream_protocol.hpp"

#include <cstring>

namespace ams::ssl::mitm::stream {
namespace {
void Put16(std::uint8_t *p, std::uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v >> 8);
    p[1] = static_cast<std::uint8_t>(v);
}
void Put32(std::uint8_t *p, std::uint32_t v) {
    for (int i = 3; i >= 0; --i) {
        p[i] = static_cast<std::uint8_t>(v);
        v >>= 8;
    }
}
void Put64(std::uint8_t *p, std::uint64_t v) {
    for (int i = 7; i >= 0; --i) {
        p[i] = static_cast<std::uint8_t>(v);
        v >>= 8;
    }
}
} // namespace

bool EncodeFrame(Frame &out, const Event &event, const void *payload,
                 std::size_t payload_size) {
    if (payload_size > MaxPayloadSize || (payload_size && !payload))
        return false;
    if (event.type != FrameType::Hello && event.type != FrameType::Data &&
        event.type != FrameType::Loss)
        return false;
    if (event.type == FrameType::Data &&
        (event.direction != Direction::ClientToServer &&
         event.direction != Direction::ServerToClient))
        return false;
    if (event.type != FrameType::Data && event.direction != Direction::None)
        return false;
    if (event.type == FrameType::Hello && payload_size != 0)
        return false;
    if (event.type == FrameType::Loss && payload_size != 16)
        return false;

    Put32(out.bytes, static_cast<std::uint32_t>(HeaderSize + payload_size));
    std::memcpy(out.bytes + 4, "NMTS", 4);
    Put16(out.bytes + 8, 1);
    out.bytes[10] = static_cast<std::uint8_t>(event.type);
    out.bytes[11] = static_cast<std::uint8_t>(event.direction);
    Put64(out.bytes + 12, event.session_id);
    Put64(out.bytes + 20, event.title_id);
    Put64(out.bytes + 28, event.connection_id);
    Put64(out.bytes + 36, event.sequence);
    Put64(out.bytes + 44, event.tick_ns);
    Put32(out.bytes + 52, static_cast<std::uint32_t>(payload_size));
    if (payload_size)
        std::memcpy(out.bytes + 56, payload, payload_size);
    out.size = 56 + payload_size;
    return true;
}

bool EncodeLoss(Frame &out, const Event &event, std::uint64_t dropped_events,
                std::uint64_t dropped_bytes) {
    std::uint8_t payload[16];
    Put64(payload, dropped_events);
    Put64(payload + 8, dropped_bytes);
    Event loss = event;
    loss.type = FrameType::Loss;
    loss.direction = Direction::None;
    return EncodeFrame(out, loss, payload, sizeof(payload));
}
} // namespace ams::ssl::mitm::stream
