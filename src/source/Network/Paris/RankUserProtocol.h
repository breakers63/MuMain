#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace Network::Paris::RankUser
{
struct Snapshot
{
    std::uint16_t target = 0;
    std::uint16_t fields = 0;
    std::int16_t emblem = -1;
    std::array<std::int32_t, 8> values{};
    std::wstring name;
    std::wstring rank;
    std::wstring characterClass;
};

extern Snapshot Data;
extern std::uint32_t Received;
bool Receive(const std::uint8_t* packet, int size);
bool Request(std::uint16_t target);
} // namespace Network::Paris::RankUser
