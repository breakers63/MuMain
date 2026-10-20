#pragma once
#include <array>
#include <string>
#include <cstring>
#include "MentorProtocol.h"
#include "Network/Server/WSclient.h"

namespace Network::Paris::GmEvents
{
constexpr int Count = 7, RecordSize = 184, ResponseSize = 8 + Count * RecordSize + 192;
struct Status
{
    bool Enabled = false;
    int Phase = 0, Participants = 0, Maximum = 0, Seconds = 0, Duration = 0, Map = 0;
    std::wstring Name, Requirement;
};
inline std::array<Status, Count> Events;
inline bool Authorized = false;
inline BYTE Result = 0;
inline DWORD Received = 0;
inline std::wstring Utf8(const BYTE *bytes, int capacity)
{
    int length = 0;
    while (length < capacity && bytes[length]) ++length;
    if (length == capacity) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char *>(bytes), length, nullptr, 0);
    std::wstring result(count, L'\0');
    if (count) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char *>(bytes), length, result.data(), count);
    return result;
}
inline bool Receive(const BYTE *packet, int size)
{
    if (size != ResponseSize || packet[0] != 0xC2 || ((packet[1] << 8) | packet[2]) != size
        || packet[3] != 0xFA || packet[4] != 0xD1 || packet[5] != 1 || packet[6] != Count) return false;
    std::array<Status, Count> next;
    for (int i = 0; i < Count; ++i)
    {
        const BYTE *row = packet + 8 + i * RecordSize;
        if (row[0] != i || row[1] > 1 || row[2] > 3) return false;
        auto &event = next[i];
        event.Enabled = row[1] != 0; event.Phase = row[2];
        int *values[] = {&event.Participants, &event.Maximum, &event.Seconds, &event.Duration, &event.Map};
        for (int n = 0; n < 5; ++n)
        {
            std::memcpy(values[n], row + 4 + n * 4, 4);
            if (*values[n] < 0) return false;
        }
        event.Name = Utf8(row + 24, 64); event.Requirement = Utf8(row + 88, 96);
    }
    if(!Authorized) Network::Paris::Mentor::Trace("gm-response-accepted");
    Events = std::move(next); Result = packet[7]; Received = timeGetTime(); Authorized = true;
    return true;
}
inline void Request(BYTE action, BYTE event, const wchar_t *announcement = L"")
{
    if (!SocketClient || event >= Count || action > 4) { Network::Paris::Mentor::Trace("gm-request-rejected"); return; }
    if(!Authorized) Network::Paris::Mentor::Trace("gm-request-sent");
    BYTE packet[200] = {0xC1, 200, 0xFA, 0xD0, 1, action, event, 0};
    if (action == 4)
    {
        int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, announcement, -1, nullptr, 0, nullptr, nullptr);
        if (length < 2 || length > 192) return;
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, announcement, -1, reinterpret_cast<char *>(packet + 8), 192, nullptr, nullptr);
    }
    SocketClient->Send(packet, sizeof(packet));
}
} // namespace Network::Paris::GmEvents
