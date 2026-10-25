#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <utility>

namespace Network::Paris::Warp
{
struct Requirement
{
    int level, maximumLevel, minimumReset, maximumReset, zen;
};

inline std::unordered_map<int, Requirement> Requirements;

inline bool Receive(std::span<const std::uint8_t> packet)
{
    constexpr int HeaderSize = 8;
    constexpr int EntrySize = 24;
    constexpr int MaximumEntries = 2048;
    if (packet.size() < HeaderSize || packet[0] != 0xC2 || packet[3] != 0xFA || packet[4] != 0xC1 || packet[5] != 1)
        return false;
    const int length = (packet[1] << 8) | packet[2];
    const int count = packet[6] | (packet[7] << 8);
    if (count > MaximumEntries || length != packet.size() || length != HeaderSize + count * EntrySize)
        return false;
    const auto read = [&packet](int offset)
    {
        const auto value = std::uint32_t(packet[offset]) | (std::uint32_t(packet[offset + 1]) << 8)
            | (std::uint32_t(packet[offset + 2]) << 16) | (std::uint32_t(packet[offset + 3]) << 24);
        return static_cast<std::int32_t>(value);
    };
    std::unordered_map<int, Requirement> next;
    for (int offset = HeaderSize; offset < length; offset += EntrySize)
    {
        const int index = read(offset);
        Requirement requirement{read(offset + 4), read(offset + 8), read(offset + 12), read(offset + 16), read(offset + 20)};
        if (index < 0 || requirement.level < 0 || requirement.zen < 0 || requirement.maximumLevel < -1
            || requirement.minimumReset < -1 || requirement.maximumReset < -1
            || (requirement.maximumLevel >= 0 && requirement.maximumLevel < requirement.level)
            || (requirement.maximumReset >= 0 && requirement.minimumReset > requirement.maximumReset)
            || !next.emplace(index, requirement).second)
            return false;
    }
    Requirements = std::move(next);
    return true;
}

inline const Requirement* Find(int index)
{
    const auto it = Requirements.find(index);
    return it == Requirements.end() ? nullptr : &it->second;
}
}
