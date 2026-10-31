/*
 * Copyright (c) Mary Guillemard <mary@mary.zone>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace ams::ssl::sf::impl {

// The original SSL service owns the returned count. Never clamp it: an
// out-of-bounds count is invalid for capture, but must remain unchanged to the
// caller. A partial Write exports only the prefix accepted by that service.
constexpr std::size_t CaptureLength(std::uint32_t returned_count,
                                    std::size_t buffer_size) {
    return returned_count != 0 && returned_count <= buffer_size ? returned_count
                                                                : 0;
}

} // namespace ams::ssl::sf::impl
