#include "stdafx.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/Character/ParisAllocationArt.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
namespace UI::Character
{
void ParisAllocationArt::Draw(int x, int y, int width, int height, int sx, int sy, int sw, int sh) const
{
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    RenderBitmap(texture, x, y, width, height, (sx + .5f) / 1024.f, (sy + .5f) / 1024.f, (sw - 1.f) / 1024.f,
                 (sh - 1.f) / 1024.f);
}
void ParisAllocationArt::Load()
{
    if (texture == BITMAP_UNKNOWN)
        texture = Bitmaps.LoadImage(L"Data\\Interface\\Paris_AllocationControls47.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
}
void ParisAllocationArt::Unload()
{
    if (texture != BITMAP_UNKNOWN)
        DeleteBitmap(texture);
    texture = BITMAP_UNKNOWN;
}
void ParisAllocationArt::Frame(int x, int y, int width, int height, Control control) const
{
    constexpr int sourceY[] = {0, 100, 220, 300, 420, 540, 680};
    constexpr int sourceWidth[] = {272, 561, 64, 391, 389, 378, 376};
    constexpr int sourceHeight[] = {78, 88, 63, 86, 86, 96, 96};
    if (texture == BITMAP_UNKNOWN)
    {
        ParisStyle::Box(x, y, width, height);
        return;
    }
    const int top = sourceY[control], sw = sourceWidth[control], sh = sourceHeight[control];
    if (control == Checkbox)
    {
        Draw(x, y, width, height, 0, top, sw, sh);
        return;
    }
    // Sample an empty interior patch, then nine-slice only the ornate edges.
    Draw(x, y, width, height, 24, top + 24, 24, 20);
    constexpr int corner = 24, targetCorner = 4, edge = 8;
    const int xs[] = {0, corner, sw - corner, sw}, ys[] = {top, top + corner, top + sh - corner, top + sh};
    const int dx[] = {x, x + targetCorner, x + width - targetCorner, x + width},
              dy[] = {y, y + targetCorner, y + height - targetCorner, y + height};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
        {
            if (row == 1 && col == 1)
                continue;
            if (row != 1 && col == 1)
                Draw(dx[col], row == 0 ? y : y + height - 2, dx[col + 1] - dx[col], 2, xs[col],
                     row == 0 ? top : top + sh - edge, xs[col + 1] - xs[col], edge);
            else if (row == 1 && col != 1)
                Draw(col == 0 ? x : x + width - 2, dy[row], 2, dy[row + 1] - dy[row], col == 0 ? 0 : sw - edge, ys[row],
                     edge, ys[row + 1] - ys[row]);
            else
                Draw(dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row], xs[col], ys[row],
                     xs[col + 1] - xs[col], ys[row + 1] - ys[row]);
        }
}
} // namespace UI::Character
