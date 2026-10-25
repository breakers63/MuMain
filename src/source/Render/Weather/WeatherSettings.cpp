#include "stdafx.h"
#include "WeatherSettings.h"
#include "Core/Platform/WinCompat.h"
#include "Core/Platform/WinIni.h"
#include <filesystem>
#include <algorithm>
namespace Render::Weather
{
namespace
{
std::filesystem::path ConfigPath()
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path() / L"config.ini";
}
std::uint64_t anchor = 0;
Settings Load()
{
    Settings settings;
    const auto path = ConfigPath();
    const auto read = [&](const wchar_t* key, int value)
    { return GetPrivateProfileIntW(L"ParisWeather", key, value, path.wstring().c_str()); };
    settings.rain = read(L"rain", 0) != 0;
    settings.storm = read(L"storm", 0) != 0;
    settings.dayNight = read(L"dayNight", 0) != 0;
    settings.snow = read(L"snow", 0) != 0;
    settings.blizzard = read(L"blizzard", 0) != 0;
    settings.fog = read(L"fog", 0) != 0;
    settings.naturalColors = read(L"naturalColors", 0) != 0;
    settings.ambientLight = read(L"ambientLight", 0) != 0;
    settings.stars = read(L"stars", 0) != 0;
    settings.distanceFog = read(L"distanceFog", 0) != 0;
    settings.rainMinutes = std::clamp(static_cast<int>(read(L"rainMinutes", 30)), 1, 120);
    settings.stormMinutes = std::clamp(static_cast<int>(read(L"stormMinutes", 30)), 1, 120);
    settings.dayNightMinutes = std::clamp(static_cast<int>(read(L"dayNightMinutes", 30)), 1, 120);
    settings.snowMinutes = std::clamp(static_cast<int>(read(L"snowMinutes", 30)), 1, 120);
    settings.blizzardMinutes = std::clamp(static_cast<int>(read(L"blizzardMinutes", 30)), 1, 120);
    settings.fogMinutes = std::clamp(static_cast<int>(read(L"fogMinutes", 30)), 1, 120);
    settings.sharpness = std::clamp(static_cast<int>(read(L"sharpness", 0)), 0, 100);
    settings.fogDensity = std::clamp(static_cast<int>(read(L"fogDensity", 0)), 0, 100);
    settings.bloom = std::clamp(static_cast<int>(read(L"bloom", 0)), 0, 100);
    settings.atmosphere = std::clamp(static_cast<int>(read(L"atmosphere", 0)), 0, 100);
    settings.quality = std::clamp(static_cast<int>(read(L"quality", 1)), 0, 2);
    settings.sky = read(L"sky", settings.quality != 0) != 0;
    settings.bloomEnabled = read(L"bloomEnabled", settings.quality != 0) != 0;
    return settings;
}
} // namespace
Settings& Options()
{
    static Settings settings = Load();
    return settings;
}
void SaveOptions()
{
    const auto path = ConfigPath();
    const auto& settings = Options();
    const auto write = [&](const wchar_t* key, int value)
    {
        const auto text = std::to_wstring(value);
        WritePrivateProfileStringW(L"ParisWeather", key, text.c_str(), path.wstring().c_str());
    };
    write(L"rain", settings.rain);
    write(L"storm", settings.storm);
    write(L"dayNight", settings.dayNight);
    write(L"snow", settings.snow);
    write(L"blizzard", settings.blizzard);
    write(L"fog", settings.fog);
    write(L"naturalColors", settings.naturalColors);
    write(L"ambientLight", settings.ambientLight);
    write(L"stars", settings.stars);
    write(L"distanceFog", settings.distanceFog);
    write(L"rainMinutes", settings.rainMinutes);
    write(L"stormMinutes", settings.stormMinutes);
    write(L"dayNightMinutes", settings.dayNightMinutes);
    write(L"snowMinutes", settings.snowMinutes);
    write(L"blizzardMinutes", settings.blizzardMinutes);
    write(L"fogMinutes", settings.fogMinutes);
    write(L"sharpness", settings.sharpness);
    write(L"fogDensity", settings.fogDensity);
    write(L"bloom", settings.bloom);
    write(L"atmosphere", settings.atmosphere);
    write(L"quality", settings.quality);
    write(L"sky", settings.sky);
    write(L"bloomEnabled", settings.bloomEnabled);
}
void RestartCycle()
{
    anchor = GetTickCount64();
}
std::uint64_t ElapsedMs()
{
    if (anchor == 0)
        RestartCycle();
    return GetTickCount64() - anchor;
}
} // namespace Render::Weather
