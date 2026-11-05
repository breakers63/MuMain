#pragma once
#include <cstdint>

namespace Network::Viewport
{
    // The upper nibble contains summon/build flags; all twelve lower bits identify the model.
    constexpr std::uint16_t DecodeMonsterType(std::uint8_t high, std::uint8_t low)
    {
        constexpr std::uint8_t TypeHighMask = 0x0F;
        return (static_cast<std::uint16_t>(high & TypeHighMask) << 8) | low;
    }
}
