#pragma once
#include "WeatherCycle.h"
#include <algorithm>

namespace Render::Weather
{
struct WeatherAudioMix
{
    float rain = 0.f, snowWind = 0.f, day = 0.f, night = 0.f;
};

inline WeatherAudioMix AudioMix(const State& state, bool dayNight)
{
    WeatherAudioMix mix;
    if (!state.outdoor || !state.exposed)
        return mix;
    if (state.kind == Kind::Rain || state.kind == Kind::Storm)
        mix.rain = std::clamp(state.rain, 0.f, 1.f);
    if (state.kind == Kind::Snow || state.kind == Kind::Blizzard)
        mix.snowWind = state.snow * (state.kind == Kind::Blizzard ? .75f : .45f);
    if (dayNight)
    {
        mix.day = (1.f - state.night) * .55f;
        mix.night = state.night * .55f;
    }
    return mix;
}
}
