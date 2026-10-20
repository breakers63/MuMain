#pragma once
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "RankUserCard.h"
#include "Character/CharacterManager.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzCharacter.h"
#include <map>
#include <string>
#include <cstring>
namespace UI::Character::RankUser
{
extern std::array<GLuint, 32> Emblems;
}
namespace UI::Character::RankingHover
{
struct Snapshot
{
    wchar_t Name[32]{}, Class[64]{}, Rank[64]{};
    std::array<int, 8> Values{};
    unsigned short Fields = 0;
    int BaseClass = -1, Emblem = -1;
    DWORD Received = 0;
};
inline std::map<std::wstring, Snapshot> Cache;
inline std::wstring Hovered, Requested;
inline DWORD HoverSince = 0, RequestedAt = 0;
inline void BeginFrame()
{
    Hovered.clear();
}
inline void Hover(const wchar_t *name)
{
    if (name && *name)
        Hovered = name;
}
inline void Clear()
{
    Cache.clear();
    Requested.clear();
    Hovered.clear();
    HoverSince = RequestedAt = 0;
}
inline bool Receive(const BYTE *packet, int size)
{
    if (!packet || size != 204 || packet[0] != 0xC1 || packet[1] != 204 || packet[2] != 0xED || packet[3] != 0x65 ||
        packet[4] != 1)
        return false;
    Snapshot value;
    std::memcpy(&value.Fields, packet + 5, 2);
    short emblem = -1;
    std::memcpy(&emblem, packet + 9, 2);
    value.Emblem = emblem;
    value.BaseClass = gCharacterManager.GetBaseClass(
        gCharacterManager.ChangeServerClassTypeToClientClassType(static_cast<SERVER_CLASS_TYPE>(packet[11])));
    for (int i = 0; i < 8; ++i)
        std::memcpy(&value.Values[i], packet + 12 + i * 4, 4);
    auto read = [](const BYTE *source, int size, wchar_t *destination, int capacity) {
        int length = 0;
        while (length < size && source[length])
            ++length;
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char *>(source), length, destination,
                            capacity - 1);
    };
    read(packet + 44, 32, value.Name, 32);
    read(packet + 76, 64, value.Rank, 64);
    read(packet + 140, 64, value.Class, 64);
    if (Requested.empty() || Requested != value.Name)
        return true;
    if (Cache.size() >= 128)
        Cache.clear();
    value.Received = GetTickCount();
    Cache[Requested] = value;
    Requested.clear();
    return true;
}
inline void Render()
{
    static std::wstring previous;
    const DWORD now = GetTickCount();
    if (previous != Hovered)
    {
        previous = Hovered;
        HoverSince = now;
    }
    if (Hovered.empty() || now - HoverSince < 250)
        return;
    auto found = Cache.find(Hovered);
    if ((found == Cache.end() || now - found->second.Received >= 300000) && SocketClient &&
        SocketClient->IsConnected() && (Requested != Hovered || now - RequestedAt >= 5000) && now - RequestedAt >= 400)
    {
        BYTE packet[15]{0xC1, 15, 0xED, 0x64, 1};
        WideCharToMultiByte(CP_UTF8, 0, Hovered.data(), static_cast<int>(Hovered.size()),
                            reinterpret_cast<char *>(packet + 5), 10, nullptr, nullptr);
        SocketClient->Send(packet, sizeof(packet));
        Requested = Hovered;
        RequestedAt = now;
    }
    if (found == Cache.end())
        return;
    const auto &value = found->second;
    RankUserCardData data;
    data.Name = value.Name;
    data.Class = value.Class;
    data.Rank = value.Rank;
    data.Values = value.Values;
    data.Fields = value.Fields;
    data.BaseClass = value.BaseClass;
    if (value.Emblem >= 0 && value.Emblem < 32)
        data.Emblem = RankUser::Emblems[value.Emblem];
    for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        if (CharactersClient[i].Object.Live && CharactersClient[i].Object.Kind == KIND_PLAYER &&
            wcscmp(CharactersClient[i].ID, value.Name) == 0)
        {
            data.Character = &CharactersClient[i];
            break;
        }
    const int width = 242,
              height = std::min(400, static_cast<int>(width * g_fScreenRate_x / g_fScreenRate_y * 440.f / 500.f));
    RankCard::Render(data, std::clamp(MouseX + 18, 4, 636 - width), std::clamp(MouseY + 18, 4, 429 - height), width,
                     height);
}
} // namespace UI::Character::RankingHover
