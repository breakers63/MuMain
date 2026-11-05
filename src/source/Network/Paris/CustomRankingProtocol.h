#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Network::Paris::CustomRanking
{
enum class Tab : std::uint8_t { Resets, Guilds, Quests };
struct Entry
{
    std::wstring name;
    std::wstring secondary;
    int primaryValue = 0;
    int secondaryValue = 0;
};
struct Snapshot
{
    Tab tab = Tab::Resets;
    std::vector<Entry> entries;
};
extern Snapshot Data;
extern std::uint32_t Received;
bool Receive(const std::uint8_t* packet, int size);
bool Request(Tab tab);
} // namespace Network::Paris::CustomRanking
