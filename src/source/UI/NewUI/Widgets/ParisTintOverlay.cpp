#include "stdafx.h"
#include "ParisTintOverlay.h"
#include <algorithm>
namespace UI::Theme
{
void TintOverlay(int x, int y, int width, int height, float red, float green, float blue, float opacity)
{
    auto byte = [](float value) { return static_cast<BYTE>(std::clamp(value, 0.f, 1.f) * 255.f); };
    const DWORD color = (static_cast<DWORD>(byte(opacity)) << 24) | (static_cast<DWORD>(byte(red)) << 16)
        | (static_cast<DWORD>(byte(green)) << 8) | byte(blue);
    RenderColorQuadARGB(x, y, width, height, color);
}
}
