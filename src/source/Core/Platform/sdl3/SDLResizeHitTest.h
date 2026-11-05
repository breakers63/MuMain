#pragma once
#include <SDL3/SDL_video.h>

namespace Core::Platform
{
inline SDL_HitTestResult ResizeHitTest(int x, int y, int width, int height)
{
    constexpr int GripPixels = 6;
    const bool left = x < GripPixels, right = x >= width - GripPixels;
    const bool top = y < GripPixels, bottom = y >= height - GripPixels;
    if (top) return left ? SDL_HITTEST_RESIZE_TOPLEFT : right ? SDL_HITTEST_RESIZE_TOPRIGHT : SDL_HITTEST_RESIZE_TOP;
    if (bottom) return left ? SDL_HITTEST_RESIZE_BOTTOMLEFT : right ? SDL_HITTEST_RESIZE_BOTTOMRIGHT : SDL_HITTEST_RESIZE_BOTTOM;
    if (left) return SDL_HITTEST_RESIZE_LEFT;
    return right ? SDL_HITTEST_RESIZE_RIGHT : SDL_HITTEST_NORMAL;
}
}
