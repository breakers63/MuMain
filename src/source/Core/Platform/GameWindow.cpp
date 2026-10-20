#include "stdafx.h"

#include "Core/Platform/GameWindow.h"
#include "Core/Platform/sdl3/SDLWindowFlags.h"
#include "Core/Platform/sdl3/SDLResizeHitTest.h"

#include <SDL3/SDL_video.h>

namespace Core::Platform
{
namespace
{
SDL_Window* g_gameWindow = nullptr;

SDL_HitTestResult SDLCALL WindowResizeHitTest(SDL_Window* window, const SDL_Point* point, void*)
{
    if (SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED)) return SDL_HITTEST_NORMAL;
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return ResizeHitTest(point->x, point->y, width, height);
}

SDL_WindowID FindFirstOpenedWindow()
{
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);
    SDL_WindowID first = 0;
    for (int i = 0; windows != nullptr && i < count; ++i)
    {
        const SDL_WindowID id = SDL_GetWindowID(windows[i]);
        if (first == 0 || id < first)
        {
            first = id;
        }
    }
    SDL_free(windows);
    return first;
}
} // namespace

SDL_Window* CreateGameWindow(int width, int height, bool fullscreen)
{
    SDL_Window* window = SDL_CreateWindow(GameWindowTitle, width, height, BuildSDLWindowFlags(fullscreen, true));
    constexpr int MinimumWidth = 640, MinimumHeight = 480;
    if (window)
    {
        SDL_SetWindowMinimumSize(window, MinimumWidth, MinimumHeight);
        SDL_SetWindowHitTest(window, WindowResizeHitTest, nullptr);
    }
    return window;
}

SDL_Window* GameWindow()
{
    if (g_gameWindow != nullptr)
    {
        return g_gameWindow;
    }
    // Kept by id, so that asking every frame does not list the windows.
    static SDL_WindowID gameWindowId = 0;
    if (gameWindowId == 0 || SDL_GetWindowFromID(gameWindowId) == nullptr)
    {
        gameWindowId = FindFirstOpenedWindow();
    }
    return gameWindowId != 0 ? SDL_GetWindowFromID(gameWindowId) : nullptr;
}

void SetGameWindow(SDL_Window* window)
{
    g_gameWindow = window;
}

SDL_Window* MessageBoxParent(SDL_Window* preferred)
{
#if defined(_EDITOR) && defined(__APPLE__)
    (void)preferred;
    return nullptr;
#else
    return preferred;
#endif
}
} // namespace Core::Platform
