#include "stdafx.h"
#include "Network/Paris/RankUserProtocol.h"

#include "Network/Server/SocketSystem.h"
#include "Network/Server/WSclient.h"

#include <cstring>

namespace Network::Paris::RankUser
{
Snapshot Data;
std::uint32_t Received = 0;

bool Request(std::uint16_t target)
{
    if (!SocketClient || !SocketClient->IsConnected())
        return false;
    const std::uint8_t packet[] = {0xC1, 7, 0xFA, 0xC2, 1,
        static_cast<std::uint8_t>(target & 0xFF), static_cast<std::uint8_t>(target >> 8)};
    SocketClient->Send(packet, sizeof(packet));
    return true;
}

bool Receive(const std::uint8_t* packet, int size)
{
    constexpr int PacketSize = 204;
    constexpr int ValuesOffset = 12;
    constexpr int NameOffset = 44;
    constexpr int NameSize = 32;
    constexpr int RankOffset = 76;
    constexpr int RankSize = 64;
    constexpr int ClassOffset = 140;
    constexpr int ClassSize = 64;
    if (!packet || size != PacketSize || packet[0] != 0xC1 || packet[1] != PacketSize
        || packet[2] != 0xFA || packet[3] != 0xC3 || packet[4] != 1 || packet[6] != 0)
        return false;

    Snapshot next;
    next.fields = packet[5];
    next.target = static_cast<std::uint16_t>(packet[7] | (packet[8] << 8));
    next.emblem = static_cast<std::int16_t>(packet[9] | (packet[10] << 8));
    if (next.emblem < -1 || next.emblem > 99)
        return false;
    for (int i = 0; i < static_cast<int>(next.values.size()); ++i)
    {
        std::memcpy(&next.values[static_cast<std::size_t>(i)], packet + ValuesOffset + i * sizeof(std::int32_t), sizeof(std::int32_t));
        if (next.values[static_cast<std::size_t>(i)] < 0)
            return false;
    }

    const auto readText = [packet](int offset, int capacity, std::wstring& output)
    {
        int length = 0;
        while (length < capacity && packet[offset + length] != 0)
            ++length;
        if (length == 0)
        {
            output.clear();
            return true;
        }
        const int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(packet + offset), length, nullptr, 0);
        if (wideLength <= 0 || wideLength > 64)
            return false;
        output.resize(static_cast<std::size_t>(wideLength));
        return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(packet + offset), length, output.data(), wideLength) == wideLength;
    };
    if (!readText(NameOffset, NameSize, next.name)
        || !readText(RankOffset, RankSize, next.rank)
        || !readText(ClassOffset, ClassSize, next.characterClass))
        return false;

    Data = std::move(next);
    Received = timeGetTime();
    return true;
}
} // namespace Network::Paris::RankUser
