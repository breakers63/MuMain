#include "stdafx.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "ClassPortraits.h"
#include <algorithm>
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzTexture.h"
namespace UI::Character::Portraits
{
namespace
{
GLuint atlas = BITMAP_UNKNOWN;
}
void Load()
{
    if (atlas == BITMAP_UNKNOWN)
        atlas = Bitmaps.LoadImage(L"Data\\Interface\\Paris_ClassPortraitsV13.jpg", GL_LINEAR, GL_CLAMP_TO_EDGE);
}
void Unload()
{
    if (atlas != BITMAP_UNKNOWN)
        DeleteBitmap(atlas);
    atlas = BITMAP_UNKNOWN;
}
void RenderPortrait(int baseClass, int x, int y, int width, int height)
{
    constexpr int tiles[]{1, 0, 2, 3, 4, 5}, left[]{3, 519, 1035}, top[]{3, 519};
    if (atlas == BITMAP_UNKNOWN || baseClass < 0 || baseClass >= 6 || width <= 0 || height <= 0)
        return;
    const int tile = tiles[baseClass];
    const float aspect = width * g_fScreenRate_x / (height * g_fScreenRate_y);
    const float cropWidth = std::min(500.f, 500.f * aspect);
    const float cropHeight = std::min(500.f, 500.f / aspect);
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    RenderBitmap(atlas, x, y, width, height, (left[tile % 3] + (500.f - cropWidth) / 2) / 2048.f,
                 (top[tile / 3] + (500.f - cropHeight) / 2) / 1024.f, cropWidth / 2048.f, cropHeight / 1024.f);
}
void Render(int baseClass, int x, int y, int width, int height)
{
    constexpr int tiles[]{1, 0, 2, 3, 4, 5};
    if (atlas == BITMAP_UNKNOWN || baseClass < 0 || baseClass >= 6)
        return;
    constexpr int left[]{3, 519, 1035}, top[]{3, 519};
    const int tile = tiles[baseClass];
    const float side = std::min(width * g_fScreenRate_x, height * g_fScreenRate_y);
    const int squareWidth = static_cast<int>(side / g_fScreenRate_x),
              squareHeight = static_cast<int>(side / g_fScreenRate_y);
    x += (width - squareWidth) / 2;
    y += (height - squareHeight) / 2;
    width = squareWidth;
    height = squareHeight;
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    RenderBitmap(atlas, x, y, width, height, left[tile % 3] / 2048.f, top[tile / 3] / 1024.f, 500.f / 2048.f,
                 500.f / 1024.f);
}
} // namespace UI::Character::Portraits
