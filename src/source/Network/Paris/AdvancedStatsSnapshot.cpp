#include "Network/Paris/AdvancedStatsSnapshot.h"

#include <bit>

namespace Network::Paris
{
std::optional<AdvancedStatsSnapshot> DecodeAdvancedStatsSnapshot(std::span<const std::uint8_t> packet)
{
    constexpr std::uint8_t Head = 0xED, Snapshot = 0x30, LegacyVersion = 2, ExtendedVersion = 3;
    constexpr std::size_t PayloadHeaderSize = 4, ValueSize = sizeof(std::int32_t);
    if (packet.size() < 8)
        return std::nullopt;
    const bool longPacket = packet[0] == 0xC2 || packet[0] == 0xC4;
    if (!longPacket && packet[0] != 0xC1 && packet[0] != 0xC3)
        return std::nullopt;
    const std::size_t headOffset = longPacket ? 3 : 2;
    const std::size_t payload = headOffset + 2;
    const std::size_t declaredSize = longPacket ? (packet[1] << 8) | packet[2] : packet[1];
    if (declaredSize != packet.size() || packet.size() < payload + PayloadHeaderSize
        || packet[headOffset] != Head || packet[headOffset + 1] != Snapshot)
        return std::nullopt;
    const int count = packet[payload + 1];
    const bool legacy = packet[payload] == LegacyVersion && count == AdvancedStatsSnapshot::LegacyFieldCount;
    const bool extended = packet[payload] == ExtendedVersion && count == AdvancedStatsSnapshot::FieldCount;
    const std::size_t valuesOffset = payload + PayloadHeaderSize;
    if ((!legacy && !extended) || packet.size() != valuesOffset + count * ValueSize)
        return std::nullopt;

    AdvancedStatsSnapshot result;
    result.receivedFields = count;
    for (int index = 0; index < count; ++index)
    {
        const auto* value = packet.data() + valuesOffset + index * ValueSize;
        const std::uint32_t bits = std::uint32_t(value[0]) | (std::uint32_t(value[1]) << 8)
            | (std::uint32_t(value[2]) << 16) | (std::uint32_t(value[3]) << 24);
        result.values[index] = std::bit_cast<std::int32_t>(bits);
    }
    return result;
}
}
