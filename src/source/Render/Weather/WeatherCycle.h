#pragma once
#include <array>
#include <cstdint>
namespace Render::Weather
{
enum class Kind
{
    Clear,
    Rain,
    Storm,
    Snow,
    Blizzard,
    Fog
};
inline constexpr std::array<int, 6> Durations{1, 5, 15, 30, 60, 120};
struct Settings
{
    bool rain = false, storm = false, dayNight = false, snow = false, blizzard = false, fog = false;
    int rainMinutes = 30, stormMinutes = 30, dayNightMinutes = 30, snowMinutes = 30, blizzardMinutes = 30,
        fogMinutes = 30;
    bool naturalColors = false, ambientLight = false, stars = false, distanceFog = false;
    bool sky = true, bloomEnabled = true;
    int quality = 1, sharpness = 0, fogDensity = 0, bloom = 0, atmosphere = 0;
};
struct State
{
    Kind kind = Kind::Clear;
    float rain = 0, storm = 0, snow = 0, fog = 0, wind = 0, night = 0, lightning = 0;
    bool outdoor = false, exposed = false;
};
bool OutdoorMap(int map);
Kind ScheduledKind(const Settings& settings, int map, std::uint64_t elapsedMs);
State Evaluate(const Settings& settings, int map, std::uint64_t elapsedMs, bool exposed);
int NextDuration(int current, int direction);
} // namespace Render::Weather
