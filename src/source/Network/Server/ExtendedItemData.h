#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace Network::Items
{
// Returns the serialized length for the current item's option and socket bits. Callers can
// compare this with the supplied span to reject both truncated and trailing item data.
inline std::optional<std::size_t> ExtendedItemLength(std::span<const std::uint8_t> data)
{
    constexpr std::size_t BaseLength = 5;
    constexpr std::uint8_t MaximumSockets = 5;
    constexpr unsigned MaximumItemNumber = 512;
    constexpr std::uint8_t MaximumLevel = 15;
    if (data.size() < BaseLength || (((data[0] & 15u) << 8) | data[1]) >= MaximumItemNumber
        || data[2] > MaximumLevel)
        return std::nullopt;

    const auto flags = data[4];
    std::size_t length = BaseLength;
    for (const auto flag : {1u, 8u, 16u, 32u})
        if (flags & flag)
            ++length;
    if (length > data.size())
        return std::nullopt;
    if (flags & 128u)
    {
        if (length == data.size())
            return std::nullopt;
        const auto sockets = data[length] & 15u;
        if (sockets > MaximumSockets)
            return std::nullopt;
        length += 1 + sockets;
    }
    return length <= data.size() ? std::optional<std::size_t>(length) : std::nullopt;
}
} // namespace Network::Items
