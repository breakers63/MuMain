#include "WeatherCycle.h"
#include <algorithm>
#include <cmath>
namespace Render::Weather
{
bool OutdoorMap(int map)
{
    constexpr int OutdoorMaps[] = {0,  2,  3,  7,  8,  10, 30, 31, 33, 34, 35, 37, 38,
                                   40, 41, 51, 56, 57, 58, 63, 80, 81, 82, 83, 84, 94};
    return std::find(std::begin(OutdoorMaps), std::end(OutdoorMaps), map) != std::end(OutdoorMaps);
}
Kind ScheduledKind(const Settings& settings, int map, std::uint64_t elapsedMs)
{
    if (!OutdoorMap(map))
        return Kind::Clear;
    struct Step
    {
        Kind kind;
        int minutes;
    };
    std::array<Step, 10> steps{};
    std::size_t stepCount = 0;
    const auto add = [&](bool enabled, Kind kind, int minutes)
    {
        if (enabled)
        {
            steps[stepCount++] = {kind, std::clamp(minutes, 1, 120)};
            steps[stepCount++] = {Kind::Clear, 1};
        }
    };
    const bool devias = map == 2;
    add(settings.rain, devias ? Kind::Snow : Kind::Rain, devias ? settings.snowMinutes : settings.rainMinutes);
    add(settings.storm, devias ? Kind::Blizzard : Kind::Storm, settings.stormMinutes);
    add(settings.snow, Kind::Snow, settings.snowMinutes);
    add(settings.blizzard, Kind::Blizzard, settings.blizzardMinutes);
    add(settings.fog, Kind::Fog, settings.fogMinutes);
    int minutes = 0;
    for (std::size_t index = 0; index < stepCount; ++index)
        minutes += steps[index].minutes;
    if (minutes == 0)
        return Kind::Clear;
    auto position = (elapsedMs / 60000) % minutes;
    for (std::size_t index = 0; index < stepCount; ++index)
    {
        const auto& step = steps[index];
        if (position < static_cast<unsigned>(step.minutes))
            return step.kind;
        position -= step.minutes;
    }
    return Kind::Clear;
}
State Evaluate(const Settings& settings, int map, std::uint64_t elapsedMs, bool exposed)
{
    State state;
    state.outdoor = OutdoorMap(map);
    state.exposed = state.outdoor && exposed;
    if (!state.outdoor)
        return state;
    state.kind = ScheduledKind(settings, map, elapsedMs);
    if (settings.dayNight)
    {
        constexpr double Tau = 6.283185307179586;
        const auto duration = static_cast<std::uint64_t>(std::clamp(settings.dayNightMinutes, 1, 120)) * 60000;
        state.night = static_cast<float>((1 - std::cos(Tau * (elapsedMs % duration) / duration)) * .5);
    }
    if (!state.exposed)
        return state;
    state.rain = state.kind == Kind::Rain ? .65f : state.kind == Kind::Storm ? 1.f : 0.f;
    state.storm = state.kind == Kind::Storm || state.kind == Kind::Blizzard ? 1.f : 0.f;
    state.snow = state.kind == Kind::Snow ? .75f : state.kind == Kind::Blizzard ? 1.f : 0.f;
    state.wind = state.kind == Kind::Blizzard ? 1.f : state.storm * .65f + state.rain * .2f;
    state.fog = state.kind == Kind::Fog ? .65f : state.storm * .25f;
    if (state.kind == Kind::Storm)
    {
        const auto phase = elapsedMs % 8200;
        if (phase < 90)
            state.lightning = 1.f - static_cast<float>(phase) / 90;
        else if (phase >= 170 && phase < 245)
            state.lightning = .6f * (1.f - static_cast<float>(phase - 170) / 75);
    }
    return state;
}
int NextDuration(int current, int direction)
{
    auto found = std::find(Durations.begin(), Durations.end(), current);
    const auto index = found == Durations.end() ? 0 : static_cast<int>(found - Durations.begin());
    return Durations[(index + (direction < 0 ? -1 : 1) + Durations.size()) % Durations.size()];
}
} // namespace Render::Weather
