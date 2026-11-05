#pragma once
#include "WeatherCycle.h"
#include <array>

namespace Render::Weather
{
struct SkyPalette
{
    std::array<float, 3> top{}, horizon{}, textureTint{};
    float stars = 0.f;
};
float SkyHour(const Settings& settings, std::uint64_t elapsedMs);
SkyPalette SampleSkyPalette(float hour, const State& weather, int map);
}
