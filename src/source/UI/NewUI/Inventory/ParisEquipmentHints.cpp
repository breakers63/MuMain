#include "stdafx.h"
#include "UI/NewUI/Inventory/ParisEquipmentHints.h"


#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"

namespace UI::Inventory::EquipmentHints
{
    constexpr int HintCount = 12;
    static GLuint Images[HintCount] = {};
    static bool Loaded = false;

    void Load()
    {
        if (Loaded) return;
        constexpr const wchar_t* paths[HintCount] = {
            L"Data\\Interface\\Paris_EquipPet60.tga",
            L"Data\\Interface\\Paris_EquipHelm60.tga",
            L"Data\\Interface\\Paris_EquipWings60.tga",
            L"Data\\Interface\\Paris_EquipBow60.tga",
            L"Data\\Interface\\Paris_EquipArmor60.tga",
            L"Data\\Interface\\Paris_EquipShield60.tga",
            L"Data\\Interface\\Paris_EquipGloves60.tga",
            L"Data\\Interface\\Paris_EquipPants60.tga",
            L"Data\\Interface\\Paris_EquipBoots60.tga",
            L"Data\\Interface\\Paris_EquipPendantOriginal60.tga",
            L"Data\\Interface\\Paris_EquipRing60.tga",
            L"Data\\Interface\\Paris_EquipRing260.tga"
        };
        for (int i = 0; i < HintCount; ++i)
            Images[i] = Bitmaps.LoadImage(paths[i], GL_LINEAR, GL_CLAMP_TO_EDGE);
        Loaded = true;
    }

    void Unload()
    {
        if (!Loaded) return;
        for (auto& image : Images)
        {
            if (image != BITMAP_UNKNOWN) DeleteBitmap(image);
            image = BITMAP_UNKNOWN;
        }
        Loaded = false;
    }

    int HintIndex(int slot)
    {
        switch (slot)
        {
        case EQUIPMENT_HELPER: return 0;
        case EQUIPMENT_HELM: return 1;
        case EQUIPMENT_WING: return 2;
        case EQUIPMENT_WEAPON_RIGHT: return 3;
        case EQUIPMENT_ARMOR: return 4;
        case EQUIPMENT_WEAPON_LEFT: return 5;
        case EQUIPMENT_GLOVES: return 6;
        case EQUIPMENT_PANTS: return 7;
        case EQUIPMENT_BOOTS: return 8;
        case EQUIPMENT_AMULET: return 9;
        case EQUIPMENT_RING_RIGHT: return 10;
        case EQUIPMENT_RING_LEFT: return 11;
        default: return -1;
        }
    }

    void RenderSlotFrame(int x, int y, int width, int height)
    {
        using UI::Character::ParisStyle::Solid;
        constexpr float BorderRed = .48f, BorderGreen = .36f, BorderBlue = .14f;
        constexpr float Background = .035f;
        Solid(x, y, width, height, BorderRed, BorderGreen, BorderBlue);
        Solid(x + 1, y + 1, width - 2, height - 2, Background, Background, Background);
    }

    void RenderStorageFrames(int gridX, int gridY, int zenX, int zenY)
    {
        constexpr int Columns = 8, Rows = 8, CellSize = 20;
        constexpr int ZenWidth = 129, ZenHeight = 14;
        for (int row = 0; row < Rows; ++row)
            for (int column = 0; column < Columns; ++column)
                RenderSlotFrame(gridX + column * CellSize, gridY + row * CellSize,
                    CellSize, CellSize);
        RenderSlotFrame(zenX, zenY, ZenWidth, ZenHeight);
    }

    void Render(int slot, int x, int y, int width, int height)
    {
        const int hint = HintIndex(slot);
        if (!Loaded || hint < 0 || Images[hint] == BITMAP_UNKNOWN) return;
        constexpr int Inset = 2;
        constexpr float PendantUV = 28.f / 32.f;
        const float uv = hint == 9 ? PendantUV : 1.f;
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(Images[hint], x + Inset, y + Inset, width - Inset * 2,
            height - Inset * 2, 0, 0, uv, uv);
    }
}
