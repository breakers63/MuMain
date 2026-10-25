#include "stdafx.h"
#include "Network/Paris/ResetProtocol.h"

#include <cstring>
#include <mutex>

namespace Network::Paris::Reset
{
namespace
{
std::mutex SnapshotMutex;
Snapshot Data;
unsigned int Received = 0;
}

std::pair<Snapshot, unsigned int> GetSnapshot()
{
    std::lock_guard lock(SnapshotMutex);
    return {Data, Received};
}

bool Receive(const unsigned char* packet, int size)
{
    constexpr int MinimumSize = 25;
    constexpr int MaximumSize = 16024;
    constexpr int HeaderSize = 24;
    constexpr int MaximumLines = 128;
    if (!packet || size < MinimumSize || size > MaximumSize || packet[0] != 0xC2
        || ((packet[1] << 8) | packet[2]) != size || packet[3] != 0xFA || packet[4] != 0xC7
        || packet[5] != 1 || packet[6] > 1 || packet[7] > 1)
        return false;

    const int count = packet[20] | (packet[21] << 8);
    if (count < 1 || count > MaximumLines)
        return false;

    Snapshot next;
    std::memcpy(&next.Token, packet + 8, sizeof(next.Token));
    std::memcpy(&next.NextReset, packet + 16, sizeof(next.NextReset));
    if (next.NextReset < 0 || (packet[6] && next.Token == 0))
        return false;
    next.Available = packet[6] != 0;
    next.CanReset = next.Available && packet[7] != 0;

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

    {
        std::lock_guard lock(SnapshotMutex);
        Data = std::move(next);
        Received = timeGetTime();
    }
    return true;
}
} // namespace Network::Paris::Reset
