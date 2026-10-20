#pragma once
#include <SDL3/SDL_video.h>
namespace Core::Platform
{
// Keep the requested resolution, pixel density and colour format while selecting
// the fastest refresh the connected monitor actually advertises.
inline bool IsFasterMatchingMode(const SDL_DisplayMode& current, const SDL_DisplayMode& candidate)
{
    return candidate.w == current.w && candidate.h == current.h
        && candidate.pixel_density == current.pixel_density && candidate.format == current.format
        && candidate.refresh_rate > current.refresh_rate;
}
void ApplyPreferredFullscreenMode(SDL_Window* window, int width, int height);
}
