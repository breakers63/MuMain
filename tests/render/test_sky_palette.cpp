#include "doctest.h"
#include "Render/Weather/SkyPalette.h"
#include <cmath>
using namespace Render::Weather;

TEST_CASE("Pegasus sky day cycle starts at noon and wraps")
{
    Settings settings;
    CHECK(SkyHour(settings, 0) == 12.f);
    settings.dayNight = true;
    settings.dayNightMinutes = 1;
    CHECK(SkyHour(settings, 0) == 12.f);
    CHECK(SkyHour(settings, 15000) == 18.f);
    CHECK(SkyHour(settings, 30000) == 0.f);
    CHECK(SkyHour(settings, 60000) == 12.f);
}

TEST_CASE("Pegasus sky keeps phase colours and weather suppresses stars")
{
    State clear;
    const auto noon = SampleSkyPalette(12.f, clear, 0);
    CHECK(noon.top[2] == doctest::Approx(.850f));
    CHECK(noon.stars == 0.f);
    const auto night = SampleSkyPalette(0.f, clear, 0);
    CHECK(night.stars == 1.f);
    State storm;
    storm.storm = 1.f;
    CHECK(SampleSkyPalette(0.f, storm, 0).stars == 0.f);
    storm.lightning = 1.f;
    CHECK(SampleSkyPalette(0.f, storm, 0).top[0] > night.top[0]);
    for (int hour = 0; hour < 24; ++hour)
        for (float value : SampleSkyPalette(static_cast<float>(hour), clear, 2).textureTint)
        {
            CHECK(std::isfinite(value));
            CHECK(value >= 0.f);
            CHECK(value <= 1.25f);
        }
}
