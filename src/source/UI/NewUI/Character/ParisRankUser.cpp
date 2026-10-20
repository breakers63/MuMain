#include "stdafx.h"
#include "ParisRankUser.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/NewUISystem.h"
#include "NewUICustomRanking.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "ClassPortraits.h"
#include "Character/CharacterManager.h"
#include "RankUserCard.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzCharacter.h"
#include <array>
#include <cstring>
#include <algorithm>

namespace UI::Character::RankUser
{
    UI::Theme::ParisFrameAssets Art;
    std::array<GLuint, 32> Emblems;
    int Target = -1, RequestedTarget = -1, Emblem = -1;
    unsigned short Fields = 0;
    DWORD RequestedAt = 0, ReceivedAt = 0;
    std::array<int, 8> Values;
    wchar_t Name[64]{}, Rank[64]{}, Class[64]{};

    void Load()
    {
        Art.Load();
        Portraits::Load();
        RankCard::Load();
        Emblems.fill(BITMAP_UNKNOWN);
        for (int i = 0; i < 32; ++i)
        {
            wchar_t file[128];
            swprintf_s(file, L"Data\\Custom\\Interface\\RankUser\\Rank%02d.tga", i + 1);
            Emblems[i] = Bitmaps.LoadImage(file, GL_LINEAR, GL_CLAMP_TO_EDGE);
        }
        Target = RequestedTarget = -1;
        ReceivedAt = RequestedAt = 0;
    }

    void Unload()
    {
        Art.Unload();
        Portraits::Unload();
        RankCard::Unload();
        for (auto image : Emblems) if (image != BITMAP_UNKNOWN) DeleteBitmap(image);
        Emblems.fill(BITMAP_UNKNOWN);
        Target = RequestedTarget = -1;
        Fields = 0;
    }

    int ReadInt(const BYTE* p)
    {
        int value;
        std::memcpy(&value, p, sizeof(value));
        return value;
    }

    void ReadText(const BYTE* input, int count, wchar_t (&output)[64])
    {
        int length = 0;
        while (length < count && input[length]) ++length;
        const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(input), length, output, 63);
        output[written > 0 ? written : 0] = 0;
    }

    bool Receive(const BYTE* packet, int size)
    {
        if (size != 204 || packet[0] != 0xC1 || packet[1] != size
            || packet[2] != 0xFA || packet[3] != 0xC3 || packet[4] != 1) return false;
        const int target = packet[7] | (packet[8] << 8);
        if (target != RequestedTarget || packet[6] != 0) return false;
        const short emblem = static_cast<short>(packet[9] | (packet[10] << 8));
        if (emblem < -1 || emblem > 99) return false;
        for (int i = 0; i < 8; ++i) if (ReadInt(packet + 12 + i * 4) < 0) return false;
        Fields = packet[5];
        Target = target;
        Emblem = emblem;
        for (int i = 0; i < 8; ++i) Values[i] = ReadInt(packet + 12 + i * 4);
        ReadText(packet + 44, 32, Name);
        ReadText(packet + 76, 64, Rank);
        ReadText(packet + 140, 64, Class);
        ReceivedAt = timeGetTime();
        return true;
    }

    // Rank inspection includes the local player without changing combat selection.
    int HoveredPlayer()
    {
        if (MouseOnWindow || !g_pNewUISystem || g_pNewUISystem->CheckMouseUse()
            || !SEASON3B::CheckMouseIn(0, 0, 640, 429)) return -1;
        int nearest = -1;
        float distance = 1.e30f;
        for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        {
            const auto& character = CharactersClient[i];
            const auto& object = character.Object;
            if (!object.Live || !object.Visible || object.Alpha <= 0.f
                || character.Dead || object.Kind != KIND_PLAYER) continue;
            if (!CollisionDetectLineToOBB(MousePosition, MouseTarget, object.OBB)) continue;
            const float dx = object.Position[0] - MousePosition[0];
            const float dy = object.Position[1] - MousePosition[1];
            const float dz = object.Position[2] - MousePosition[2];
            const float candidate = dx * dx + dy * dy + dz * dz;
            if (candidate < distance) { distance = candidate; nearest = i; }
        }
        return nearest;
    }

    void Render()
    {
        const int hovered = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? HoveredPlayer() : -1;
        if (hovered < 0)
        {
            RequestedTarget = -1;
            return;
        }
        const auto& character = CharactersClient[hovered];
        if (!character.Object.Live || character.Dead || character.Object.Kind != KIND_PLAYER) return;
        const int key = static_cast<unsigned short>(character.Key);
        const DWORD now = timeGetTime();
        if (SocketClient != nullptr && (RequestedTarget != key || RequestedAt == 0 || now - RequestedAt >= 600))
        {
            RequestedAt = now;
            RequestedTarget = key;
            BYTE request[] = {0xC1, 7, 0xFA, 0xC2, 1,
                static_cast<BYTE>(key), static_cast<BYTE>(key >> 8)};
            SocketClient->Send(request, sizeof(request));
        }
        if (key != Target || now - ReceivedAt > 1800 || Fields == 0) return;
        const int width=242;
        const int height=std::min(400, static_cast<int>(width*g_fScreenRate_x/g_fScreenRate_y*440.f/500.f));
        const int x=std::clamp(MouseX+18,4,636-width),y=std::clamp(MouseY+18,4,429-height);
        RankUserCardData data;
        data.Name=Name;data.Class=Class;data.Rank=Rank;data.Fields=Fields;data.Values=Values;
        data.BaseClass=gCharacterManager.GetBaseClass(character.Class);
        data.Character=&character;
        if(Emblem>=0 && Emblem<32)data.Emblem=Emblems[Emblem];
        RankCard::Render(data,x,y,width,height);
    }
}
