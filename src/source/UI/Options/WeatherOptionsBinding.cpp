#include "stdafx.h"
#include "WeatherOptionsBinding.h"
#include "Render/Look/LookPresetSetting.h"
#include "Render/PostProcessing/PostProcessingSetting.h"
#include "Render/Weather/WeatherSettings.h"
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>
#include <algorithm>
#include <array>
namespace UI::Options::Weather
{
namespace
{
constexpr const char* Flags[] = {"weather_rain",     "weather_storm",   "weather_day",     "weather_snow",
                                 "weather_blizzard", "weather_fog",     "weather_natural", "weather_ambient",
                                 "weather_stars",    "weather_distance", "weather_sky", "weather_bloom_enabled"};
constexpr const char* Minutes[] = {"weather_rain_minutes", "weather_storm_minutes",    "weather_day_minutes",
                                   "weather_snow_minutes", "weather_blizzard_minutes", "weather_fog_minutes"};
constexpr const char* Levels[] = {"weather_sharpness", "weather_fog_density", "weather_bloom", "weather_atmosphere"};
std::array<bool*, 12> Booleans()
{
    auto& s = Render::Weather::Options();
    return {&s.rain, &s.storm,         &s.dayNight,     &s.snow,  &s.blizzard,
            &s.fog,  &s.naturalColors, &s.ambientLight, &s.stars, &s.distanceFog, &s.sky, &s.bloomEnabled};
}
std::array<int*, 6> Durations()
{
    auto& s = Render::Weather::Options();
    return {&s.rainMinutes, &s.stormMinutes, &s.dayNightMinutes, &s.snowMinutes, &s.blizzardMinutes, &s.fogMinutes};
}
std::array<int*, 4> Percentages()
{
    auto& s = Render::Weather::Options();
    return {&s.sharpness, &s.fogDensity, &s.bloom, &s.atmosphere};
}
} // namespace
void Bind(Rml::DataModelConstructor& model, std::function<void(const char*)> dirty)
{
    const auto flags = Booleans();
    const auto minutes = Durations();
    const auto levels = Percentages();
    for (std::size_t i = 0; i < flags.size(); ++i)
        model.Bind(Flags[i], flags[i]);
    for (std::size_t i = 0; i < minutes.size(); ++i)
        model.Bind(Minutes[i], minutes[i]);
    for (std::size_t i = 0; i < levels.size(); ++i)
        model.Bind(Levels[i], levels[i]);
    model.Bind("weather_quality", &Render::Weather::Options().quality);
    model.BindEventCallback("weather_toggle",
                            [dirty](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
                            {
                                if (args.size() != 1)
                                    return;
                                const int index = args[0].Get<int>(-1);
                                const auto fields = Booleans();
                                if (index < 0 || index >= static_cast<int>(fields.size()))
                                    return;
                                *fields[index] = !*fields[index];
                                dirty(Flags[index]);
                                if ((index == 6 || index == 11) && *fields[index])
                                {
                                    Render::Look::ChooseEnhancedFeature(Render::Look::EnhancedFeature::PostProcessing, true);
                                    Render::PostProcessing::SetPostProcessingEnabled(true);
                                }
                                Render::Weather::SaveOptions();
                                if (index < 6)
                                    Render::Weather::RestartCycle();
                            });
    model.BindEventCallback("weather_duration",
                            [dirty](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
                            {
                                if (args.size() != 2)
                                    return;
                                const int index = args[0].Get<int>(-1), direction = args[1].Get<int>(0);
                                const auto fields = Durations();
                                if (index < 0 || index >= static_cast<int>(fields.size()) ||
                                    (direction != -1 && direction != 1))
                                    return;
                                *fields[index] = Render::Weather::NextDuration(*fields[index], direction);
                                dirty(Minutes[index]);
                                Render::Weather::SaveOptions();
                                Render::Weather::RestartCycle();
                            });
    model.BindEventCallback("weather_adjust",
                            [dirty](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
                            {
                                if (args.size() != 2)
                                    return;
                                const int index = args[0].Get<int>(-1), direction = args[1].Get<int>(0);
                                const auto fields = Percentages();
                                if (index < 0 || index >= static_cast<int>(fields.size()) ||
                                    (direction != -1 && direction != 1))
                                    return;
                                constexpr int PercentStep = 10;
                                *fields[index] = std::clamp(*fields[index] + direction * PercentStep, 0, 100);
                                dirty(Levels[index]);
                                Render::Weather::SaveOptions();
                            });
    model.BindEventCallback("weather_quality_select",
                            [dirty](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
                            {
                                if (args.size() != 1)
                                    return;
                                const int index = args[0].Get<int>(-1);
                                if (index < 0 || index > 2)
                                    return;
                                auto& settings = Render::Weather::Options();
                                // Change image processing only; preserve the player's other
                                // graphics choices such as lighting, filtering and antialiasing.
                                Render::Look::ChooseEnhancedFeature(Render::Look::EnhancedFeature::PostProcessing,
                                                                    index != 0);
                                Render::PostProcessing::SetPostProcessingEnabled(index != 0);
                                settings.quality = index;
                                // Modern enables Vulkan post-processing; Ultra HD applies stronger
                                // grading, sharpening, bloom and weather particles.
                                constexpr int Sharpness[] = {0, 30, 75};
                                constexpr int FogDensity[] = {0, 35, 75};
                                constexpr int Bloom[] = {0, 35, 75};
                                constexpr int Atmosphere[] = {0, 20, 45};
                                settings.sharpness = Sharpness[index];
                                settings.fogDensity = FogDensity[index];
                                settings.bloom = Bloom[index];
                                settings.atmosphere = Atmosphere[index];
                                settings.naturalColors = index > 0;
                                settings.sky = index > 0;
                                settings.bloomEnabled = index > 0;
                                dirty("weather_quality");
                                dirty("weather_sharpness");
                                dirty("weather_fog_density");
                                dirty("weather_bloom");
                                dirty("weather_atmosphere");
                                dirty("weather_natural");
                                dirty("weather_sky");
                                dirty("weather_bloom_enabled");
                                Render::Weather::SaveOptions();
                            });
}
} // namespace UI::Options::Weather
