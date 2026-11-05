#include "stdafx.h"
#include "Network/Paris/CustomRankingProtocol.h"
#include "Network/Server/SocketSystem.h"
#include "Network/Server/WSclient.h"

#include <cstring>

namespace Network::Paris::CustomRanking
{
namespace
{
constexpr std::uint8_t RankingHead = 0xED;
constexpr std::uint8_t Version = 1;
constexpr int MaximumEntries = 50;
constexpr int NameLength = 10;

bool DecodeText(const std::uint8_t* bytes, int capacity, std::wstring& result)
{
    int length = 0;
    while (length < capacity && bytes[length] != 0) ++length;
    const int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char*>(bytes), length, nullptr, 0);
    if ((length > 0 && needed == 0) || needed > 32) return false;
    result.assign(static_cast<std::size_t>(needed), L'\0');
    return needed == 0 || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char*>(bytes), length, result.data(), needed) == needed;
}

std::int32_t ReadInt32(const std::uint8_t* bytes)
{
    std::int32_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}
} // namespace

Snapshot Data;
std::uint32_t Received = 0;

bool Request(Tab tab)
{
    if (!SocketClient || !SocketClient->IsConnected()) return false;
    std::uint8_t subcode = 0x50;
    if (tab == Tab::Guilds) subcode = 0x52;
    else if (tab == Tab::Quests) subcode = 0x58;
    const std::uint8_t packet[] = {0xC1, 5, RankingHead, subcode, Version};
    SocketClient->Send(packet, sizeof(packet));
    return true;
}

bool Receive(const std::uint8_t* packet, int size)
{
    if (!packet || size < 7) return false;
    const bool shortPacket = packet[0] == 0xC1 || packet[0] == 0xC3;
    const bool longPacket = packet[0] == 0xC2 || packet[0] == 0xC4;
    if (!shortPacket && !longPacket) return false;
    const int header = shortPacket ? 2 : 3;
    const int subcodeOffset = header + 1;
    const int payload = subcodeOffset + 1;
    const int declared = shortPacket ? packet[1] : (packet[1] << 8) | packet[2];
    if (declared != size || size <= payload || packet[header] != RankingHead) return false;

    Tab tab;
    int entrySize;
    switch (packet[subcodeOffset])
    {
    case 0x51: tab = Tab::Resets; entrySize = NameLength + 4; break;
    case 0x53: tab = Tab::Guilds; entrySize = NameLength * 2 + 8; break;
    case 0x59: tab = Tab::Quests; entrySize = NameLength + 8; break;
    default: return false;
    }
    if (size < payload + 2 || packet[payload] != Version) return false;
    const int count = packet[payload + 1];
    const int rowsOffset = payload + 2;
    if (count > MaximumEntries || size != rowsOffset + count * entrySize) return false;

    Snapshot next;
    next.tab = tab;
    next.entries.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index)
    {
        const std::uint8_t* row = packet + rowsOffset + index * entrySize;
        Entry entry;
        if (!DecodeText(row, NameLength, entry.name)) return false;
        if (tab == Tab::Guilds)
        {
            if (!DecodeText(row + NameLength, NameLength, entry.secondary)) return false;
            entry.primaryValue = ReadInt32(row + NameLength * 2);
            entry.secondaryValue = ReadInt32(row + NameLength * 2 + 4);
        }
        else if (tab == Tab::Quests)
        {
            entry.primaryValue = ReadInt32(row + NameLength);
            entry.secondaryValue = ReadInt32(row + NameLength + 4);
        }
        else
        {
            entry.primaryValue = ReadInt32(row + NameLength);
        }
        if (entry.primaryValue < 0 || entry.secondaryValue < 0 || entry.name.empty()) return false;
        next.entries.push_back(std::move(entry));
    }
    Data = std::move(next);
    Received = timeGetTime();
    return true;
}
} // namespace Network::Paris::CustomRanking
