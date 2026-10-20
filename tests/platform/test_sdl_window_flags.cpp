#include <doctest.h>
#include <SDL3/SDL_video.h>

#include "Core/Platform/sdl3/SDLWindowFlags.h"
#include "Core/Platform/sdl3/SDLResizeHitTest.h"

TEST_CASE("Window resize grips include every edge and corner without capturing the center")
{
    using Core::Platform::ResizeHitTest;
    CHECK(ResizeHitTest(0, 0, 1024, 768) == SDL_HITTEST_RESIZE_TOPLEFT);
    CHECK(ResizeHitTest(1023, 0, 1024, 768) == SDL_HITTEST_RESIZE_TOPRIGHT);
    CHECK(ResizeHitTest(0, 767, 1024, 768) == SDL_HITTEST_RESIZE_BOTTOMLEFT);
    CHECK(ResizeHitTest(1023, 767, 1024, 768) == SDL_HITTEST_RESIZE_BOTTOMRIGHT);
    CHECK(ResizeHitTest(0, 300, 1024, 768) == SDL_HITTEST_RESIZE_LEFT);
    CHECK(ResizeHitTest(1023, 300, 1024, 768) == SDL_HITTEST_RESIZE_RIGHT);
    CHECK(ResizeHitTest(300, 0, 1024, 768) == SDL_HITTEST_RESIZE_TOP);
    CHECK(ResizeHitTest(300, 767, 1024, 768) == SDL_HITTEST_RESIZE_BOTTOM);
    CHECK(ResizeHitTest(300, 300, 1024, 768) == SDL_HITTEST_NORMAL);
}

TEST_CASE("SDL windows always request high pixel density [platform][sdl]")
{
    const SDL_WindowFlags flags = Core::Platform::BuildSDLWindowFlags(false, false);

    CHECK((flags & SDL_WINDOW_HIGH_PIXEL_DENSITY) != 0);
}

TEST_CASE("SDL window flags preserve fullscreen and resizable requests [platform][sdl]")
{
    const SDL_WindowFlags flags = Core::Platform::BuildSDLWindowFlags(true, true);

    CHECK((flags & SDL_WINDOW_FULLSCREEN) != 0);
    CHECK((flags & SDL_WINDOW_RESIZABLE) != 0);
}

TEST_CASE("SDL startup focus follows the input-focus window flag [platform][sdl]")
{
    const SDL_WindowFlags unfocusedFullscreen = SDL_WINDOW_FULLSCREEN;
    const SDL_WindowFlags focusedFullscreen = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_INPUT_FOCUS;

    CHECK_FALSE(Core::Platform::HasSDLWindowInputFocus(unfocusedFullscreen));
    CHECK(Core::Platform::HasSDLWindowInputFocus(focusedFullscreen));
}
