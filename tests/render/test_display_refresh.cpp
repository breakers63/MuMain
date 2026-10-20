#include "doctest.h"
#include "Core/Platform/sdl3/SDLFullscreenMode.h"
TEST_CASE("fullscreen selects higher refresh only for the same image mode")
{
 SDL_DisplayMode current{};current.w=1920;current.h=1080;current.pixel_density=1;current.refresh_rate=60;
 auto candidate=current;candidate.refresh_rate=144;CHECK(Core::Platform::IsFasterMatchingMode(current,candidate));
 candidate.refresh_rate=60;CHECK_FALSE(Core::Platform::IsFasterMatchingMode(current,candidate));
 candidate.refresh_rate=30;CHECK_FALSE(Core::Platform::IsFasterMatchingMode(current,candidate));
 candidate.refresh_rate=240;candidate.w=1280;CHECK_FALSE(Core::Platform::IsFasterMatchingMode(current,candidate));
 candidate.w=1920;candidate.pixel_density=2;CHECK_FALSE(Core::Platform::IsFasterMatchingMode(current,candidate));
 candidate.pixel_density=1;candidate.refresh_rate=0;CHECK_FALSE(Core::Platform::IsFasterMatchingMode(current,candidate));
}
