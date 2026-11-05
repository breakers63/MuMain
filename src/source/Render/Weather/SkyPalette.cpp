#include "SkyPalette.h"
#include <algorithm>
#include <cmath>

namespace Render::Weather
{
float SkyHour(const Settings& settings, std::uint64_t elapsedMs)
{
    if (!settings.dayNight)
        return 12.f;
    constexpr std::uint64_t MillisecondsPerMinute = 60000;
    const auto period = static_cast<std::uint64_t>(std::clamp(settings.dayNightMinutes, 1, 120)) * MillisecondsPerMinute;
    return std::fmod(12.f + 24.f * static_cast<float>(elapsedMs % period) / period, 24.f);
}

namespace
{
void ApplySkyWeather(SkyPalette& palette, const State& weather)
{
    const bool blizzard = weather.kind == Kind::Blizzard;
    const float dark = std::clamp(weather.rain * .14f + weather.storm * .38f, 0.f, .62f);
    const float cold = std::clamp(weather.snow * (blizzard ? .65f : .35f), 0.f, .85f);
    const float haze = std::clamp(weather.rain * .20f + weather.storm * .45f + weather.snow * .20f + weather.fog * .70f, 0.f, .88f);
    constexpr float ColdTop[] = {.42f, .54f, .72f}, ColdHorizon[] = {.70f, .78f, .88f};
    constexpr float FogColor[] = {.46f, .48f, .50f};
    for (int c = 0; c < 3; ++c)
    {
        palette.top[c] *= 1.f - dark;
        palette.horizon[c] *= 1.f - dark * .70f;
        palette.top[c] = std::lerp(palette.top[c], ColdTop[c], cold * .45f);
        palette.horizon[c] = std::lerp(palette.horizon[c], ColdHorizon[c], cold * .55f);
        palette.horizon[c] = std::lerp(palette.horizon[c], FogColor[c], haze);
        palette.top[c] = std::lerp(palette.top[c], .92f, weather.lightning * .62f);
        palette.horizon[c] = std::lerp(palette.horizon[c], 1.f, weather.lightning * .72f);
    }
    palette.stars *= 1.f - std::clamp(weather.rain * .55f + weather.storm + weather.snow * .45f + weather.fog * .85f, 0.f, 1.f);
}

void TextureTint(SkyPalette& palette)
{
    std::array<float, 3> average{};
    for (int c = 0; c < 3; ++c)
        average[c] = (palette.top[c] + palette.horizon[c]) * .5f;
    const float maximum = std::max({average[0], average[1], average[2], .0001f});
    const float luminance = average[0] * .25f + average[1] * .50f + average[2] * .25f;
    const float brightness = std::clamp(.24f + luminance * 1.35f, .22f, 1.f);
    constexpr float TextureTintStrength = .58f;
    for (int c = 0; c < 3; ++c)
        palette.textureTint[c] = std::clamp(std::lerp(1.f, average[c] / maximum, TextureTintStrength) * brightness, 0.f, 1.25f);
}
}

SkyPalette SampleSkyPalette(float hour, const State& weather, int map)
{
    constexpr float Top[4][3] = {{.018f,.030f,.100f},{.235f,.375f,.660f},{.195f,.475f,.850f},{.110f,.160f,.390f}};
    constexpr float Horizon[4][3] = {{.050f,.095f,.205f},{.930f,.500f,.285f},{.720f,.850f,.970f},{.930f,.350f,.190f}};
    constexpr float Stars[] = {1.f, .08f, 0.f, .12f};
    hour = std::fmod(std::fmod(hour, 24.f) + 24.f, 24.f);
    const int phase = static_cast<int>(hour / 6.f), next = (phase + 1) % 4;
    float blend = (hour - phase * 6.f) / 6.f;
    blend = blend * blend * (3.f - 2.f * blend);
    SkyPalette palette;
    for (int c = 0; c < 3; ++c)
    {
        palette.top[c] = std::lerp(Top[phase][c], Top[next][c], blend);
        palette.horizon[c] = std::lerp(Horizon[phase][c], Horizon[next][c], blend);
    }
    if (map == 2)
    {
        palette.top[0] *= .88f;
        palette.top[1] *= .98f;
        palette.top[2] *= 1.14f;
        palette.horizon[0] *= .92f;
        palette.horizon[1] *= 1.02f;
        palette.horizon[2] *= 1.10f;
    }
    palette.stars = std::lerp(Stars[phase], Stars[next], blend);
    ApplySkyWeather(palette, weather);
    TextureTint(palette);
    return palette;
}
}
