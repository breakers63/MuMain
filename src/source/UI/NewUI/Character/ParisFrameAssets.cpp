#include "stdafx.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/NewUICharacterInfoWindow.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"

namespace UI::Theme
{
void ParisFrameAssets::Load()
{
    if(statsButton==BITMAP_UNKNOWN)statsButton=Bitmaps.LoadImage(L"Data\\Interface\\Paris_ButtonBase54.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
    if (frame == BITMAP_UNKNOWN)
        frame = Bitmaps.LoadImage(L"Data\\Interface\\Paris_Stats.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    if (buttons == BITMAP_UNKNOWN)
        buttons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_Stats_Buttons.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    if (inventoryButtons == BITMAP_UNKNOWN)
        inventoryButtons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_Inventory_Buttons.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    if (shopButtons == BITMAP_UNKNOWN)
        shopButtons = Bitmaps.LoadImage(L"Data\\Interface\\Paris_Shop_Buttons.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
}

void ParisFrameAssets::Unload()
{
    if (frame != BITMAP_UNKNOWN)
        DeleteBitmap(frame);
    if (buttons != BITMAP_UNKNOWN)
        DeleteBitmap(buttons);
    if (statsButton != BITMAP_UNKNOWN)
        DeleteBitmap(statsButton);
    statsButton = BITMAP_UNKNOWN;
    if (inventoryButtons != BITMAP_UNKNOWN)
        DeleteBitmap(inventoryButtons);
    if (shopButtons != BITMAP_UNKNOWN)
        DeleteBitmap(shopButtons);
    frame = buttons = inventoryButtons = shopButtons = BITMAP_UNKNOWN;
}

void ParisFrameAssets::Frame(int x, int y, int w, int h, bool borderOnly) const
{
    if (frame == BITMAP_UNKNOWN) { Character::ParisStyle::Box(x, y, w, h); return; }
    // Nine slices keep corner ornaments proportional on wide windows.
    constexpr int sourceW = 496, sourceH = 1048, corner = 40, edge = 16;
    const int sx[] = {0, corner, sourceW - corner, sourceW};
    const int sy[] = {0, corner, sourceH - corner, sourceH};
    const int dx[] = {x, x + edge, x + w - edge, x + w};
    const int dy[] = {y, y + edge, y + h - edge, y + h};
    EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
        {
            if (borderOnly && row == 1 && col == 1) continue;
            RenderBitmap(frame, dx[col], dy[row], dx[col+1]-dx[col], dy[row+1]-dy[row],
                sx[col]/512.f, sy[row]/2048.f, (sx[col+1]-sx[col])/512.f, (sy[row+1]-sy[row])/2048.f);
        }
}

void ParisFrameAssets::Stone(int x, int y, int w, int h, bool light, bool brown) const
{
    if (frame != BITMAP_UNKNOWN)
    {
        EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(frame, x, y, w, h, 50.f/512, 180.f/2048, 390.f/512, 500.f/2048);
    }
    EnableAlphaTest();
    if (brown) UI::Paris::Color(.37f, .24f, .12f, light ? .62f : .32f);
    else UI::Paris::Color(.32f, .32f, .32f, light ? .24f : .03f);
    RenderColor(x,y,w,h); EndRenderColor(); EnableAlphaTest(); UI::Paris::Color(1,1,1,1);
}

void ParisFrameAssets::Exit(int x, int y) const
{
    if (buttons != BITMAP_UNKNOWN) Character::ParisStyle::Footer(buttons, x, y, 0);
    else { Character::ParisStyle::Box(x,y,36,29); Character::ParisStyle::Label(x,y+9,36,L"X"); }
}

void ParisFrameAssets::SpriteButton(int x, int y, int w, int h, int tile, bool enabled, bool shop) const
{
    const GLuint atlas = shop ? shopButtons : inventoryButtons;
    if (atlas == BITMAP_UNKNOWN)
    {
        InventoryButton(x, y, w, h, buttons, enabled);
        return;
    }
    EnableAlphaTest();
    const float brightness = enabled ? 1.f : .4f;
    UI::Paris::Color(brightness, brightness, brightness, 1.f);
    RenderBitmap(atlas, x, y, w, h, (tile % 2) * .5f, (tile / 2) * .5f, .5f, .5f);
    UI::Paris::Color(1, 1, 1, 1);
}

void ParisFrameAssets::InventoryButton(int x, int y, int w, int h, GLuint icon, bool enabled, bool expansion,
                                       float sourceScale) const
{
    if (statsButton != BITMAP_UNKNOWN)
    {
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(statsButton, x, y, w, h, 0, 0, 1, 1);
    }
    else
        Stone(x, y, w, h);
    if (statsButton == BITMAP_UNKNOWN && buttons != BITMAP_UNKNOWN)
    {
        constexpr int source[] = {0, 24, 232, 256};
        const int dx[] = {x, x + 3, x + w - 3, x + w}, dy[] = {y, y + 3, y + h - 3, y + h};
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
            {
                if (row == 1 && col == 1)
                    continue;
                RenderBitmap(buttons, dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row],
                             source[col] / 512.f, source[row] / 512.f, (source[col + 1] - source[col]) / 512.f,
                             (source[row + 1] - source[row]) / 512.f);
            }
    }
    const auto& bitmap = Bitmaps[icon];
    if (bitmap.Width > 0 && bitmap.Height > 0)
    {
        const float brightness = enabled ? 1.f : .4f;
        UI::Paris::Color(brightness, brightness, brightness, 1);
        RenderBitmap(icon, x + 4, y + 4, w - 8, h - 8, 8.f * sourceScale / bitmap.Width,
                     6.f * sourceScale / bitmap.Height, 20.f * sourceScale / bitmap.Width,
                     17.f * sourceScale / bitmap.Height);
    }
    UI::Paris::Color(1, 1, 1, 1);
    if (expansion)
    {
        Character::ParisStyle::Solid(x + w - 9, y + h - 9, 8, 8, .06f, .06f, .06f);
        Character::ParisStyle::Solid(x + w - 8, y + h - 6, 6, 2, .93f, .72f, .25f);
        Character::ParisStyle::Solid(x + w - 6, y + h - 8, 2, 6, .93f, .72f, .25f);
    }
}

void ParisFrameAssets::InventoryGrid(int x, int y, int columns, int rows) const
{
    Stone(x, y, columns * 20, rows * 20);
    for (int col = 0; col <= columns; ++col)
        Character::ParisStyle::Solid(x + col * 20, y, 1, rows * 20, .43f, .40f, .33f);
    for (int row = 0; row <= rows; ++row)
        Character::ParisStyle::Solid(x, y + row * 20, columns * 20, 1, .43f, .40f, .33f);
}
} // namespace UI::Theme
