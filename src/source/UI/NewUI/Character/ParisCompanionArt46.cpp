#include "stdafx.h"
#include "UI/NewUI/Character/ParisCompanionArt46.h"
#include "UI/Legacy/UIControls.h"
namespace UI::Theme
{
    void ParisCompanionArt46::Border(int x, int y, int width) const
    {
        constexpr int sourceX[] = {0, 10, 224, 234}, sourceY[] = {64, 74, 125, 135};
        const int targetX[] = {x, x + 3, x + width - 3, x + width}, targetY[] = {y, y + 3, y + 19, y + 22};
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
            {
                if (row == 1 && col == 1)
                    continue;
                RenderBitmap(icons, targetX[col], targetY[row], targetX[col + 1] - targetX[col],
                             targetY[row + 1] - targetY[row], (sourceX[col] + .5f) / 512.f,
                             (sourceY[row] + .5f) / 256.f, (sourceX[col + 1] - sourceX[col] - 1.f) / 512.f,
                             (sourceY[row + 1] - sourceY[row] - 1.f) / 256.f);
            }
    }
    void ParisCompanionArt46::Load()
    {
        if (icons == BITMAP_UNKNOWN)
            icons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_CompanionIcons46.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    }
    void ParisCompanionArt46::Unload()
    {
        if (icons != BITMAP_UNKNOWN)
            DeleteBitmap(icons);
        icons = BITMAP_UNKNOWN;
    }
    void ParisCompanionArt46::Tab(int x, int y, int width, int index, const wchar_t *label, bool selected) const
    {
        UI::Character::ParisStyle::Solid(x, y, width, 22, selected ? .16f : .015f, selected ? .11f : .015f, .015f);
        if (icons != BITMAP_UNKNOWN)
        {
            EnableAlphaTest();
            UI::Paris::Color(1, 1, 1, 1);
            Border(x, y, width);
            RenderBitmap(icons, x + 3, y + 5, 12, 12, index * 48.f / 512.f, 0, 48.f / 512.f, 48.f / 256.f);
        }
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(245, 245, 245, 255);
        g_pRenderText->RenderText(x + 16, y + 6, label, width - 18, 0, RT3_SORT_CENTER);
    }
} // namespace UI::Theme
