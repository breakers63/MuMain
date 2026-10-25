#include "SDLFullscreenMode.h"
#include <SDL3/SDL_stdinc.h>
namespace Core::Platform
{
void ApplyPreferredFullscreenMode(SDL_Window* window, int width, int height)
{
    if (!window || width <= 0 || height <= 0) return;
    SDL_DisplayID display = SDL_GetDisplayForWindow(window);
    if (!display) display = SDL_GetPrimaryDisplay();
    SDL_DisplayMode selected{};
    if (SDL_GetClosestFullscreenDisplayMode(display, width, height, 0.0f, false, &selected))
    {
        int count = 0;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &count);
        if (modes)
        {
            for (int i = 0; i < count; ++i)
                if (modes[i] && IsFasterMatchingMode(selected, *modes[i])) selected = *modes[i];
            SDL_free(modes);
        }
        // A failed exclusive mode request still has a usable desktop fallback.
        if (!SDL_SetWindowFullscreenMode(window, &selected)) SDL_SetWindowFullscreenMode(window, nullptr);
    }
    else SDL_SetWindowFullscreenMode(window, nullptr);
    SDL_SetWindowSize(window, width, height);
    SDL_SetWindowFullscreen(window, true);
}
}
