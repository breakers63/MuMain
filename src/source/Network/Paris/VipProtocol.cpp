#include "stdafx.h"
#include "Network/Paris/VipProtocol.h"

#include <cstring>

namespace Network::Paris::Vip
{
Snapshot Data;
std::uint32_t Received = 0;

bool Receive(const std::uint8_t* packet, int size)
{
    constexpr int MinimumSize = 33;
    constexpr int MaximumSize = 16032;
    constexpr int HeaderSize = 32;
    constexpr int MaximumLines = 128;
    if (!packet || size < MinimumSize || size > MaximumSize || packet[0] != 0xC2
        || ((packet[1] << 8) | packet[2]) != size || packet[3] != 0xFA || packet[4] != 0xB3
        || packet[5] != 1 || packet[6] > 3 || packet[7] < 1 || packet[7] > 3 || packet[8] > 1)
        return false;

    const int count = packet[24] | (packet[25] << 8);
    if (count < 1 || count > MaximumLines)
        return false;

    Snapshot next;
    next.CurrentTier = packet[6];
    next.RequestedPlan = packet[7];
    next.CanBuy = packet[8] != 0;
    std::memcpy(&next.RemainingDays, packet + 12, sizeof(next.RemainingDays));
    std::memcpy(&next.Quote, packet + 16, sizeof(next.Quote));
    if (next.RemainingDays < 0 || (next.CanBuy && next.Quote == 0))
        return false;

    int offset = HeaderSize;
    for (int i = 0; i < count; ++i)
    {
        int end = offset;
        while (end < size && packet[end] != 0)
            ++end;
        if (end == size || end - offset > 1024)
            return false;
        const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(packet + offset), end - offset, nullptr, 0);
        if (length <= 0 || length > 512)
            return false;
        std::wstring line(static_cast<std::size_t>(length), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                reinterpret_cast<const char*>(packet + offset), end - offset, line.data(), length) != length)
            return false;
        next.Lines.push_back(std::move(line));
        offset = end + 1;
    }
    if (offset != size)
        return false;

    Data = std::move(next);
    Received = timeGetTime();
    return true;
}
} // namespace Network::Paris::Vip
