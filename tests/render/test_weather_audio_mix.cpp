#include "doctest.h"
#include "Render/Weather/WeatherAudioMix.h"
using namespace Render::Weather;

TEST_CASE("rain and storms never use the snow wind track")
{
    State state;
    state.outdoor = state.exposed = true;
    state.rain = 1.f;
    state.snow = .8f; // A transition may retain the previous climate's intensity.
    for (const auto kind : {Kind::Rain, Kind::Storm})
    {
        state.kind = kind;
        const auto mix = AudioMix(state, false);
        CHECK(mix.rain == 1.f);
        CHECK(mix.snowWind == 0.f);
    }
}

TEST_CASE("snow and clear weather stop the rain track")
{
    State state;
    state.outdoor = state.exposed = true;
    state.rain = 1.f;
    state.snow = 1.f;
    for (const auto kind : {Kind::Snow, Kind::Blizzard, Kind::Fog, Kind::Clear})
    {
        state.kind = kind;
        const auto mix = AudioMix(state, false);
        CHECK(mix.rain == 0.f);
        CHECK((mix.snowWind > 0.f) == (kind == Kind::Snow || kind == Kind::Blizzard));
    }
    state.kind = Kind::Storm;
    state.exposed = false;
    CHECK(AudioMix(state, true).rain == 0.f);
    CHECK(AudioMix(state, true).day == 0.f);
}

TEST_CASE("blizzards do not trigger rainstorm lightning")
{
    Settings settings;
    settings.blizzard = true;
    const auto state = Evaluate(settings, 0, 0, true);
    CHECK(state.snow == 1.f);
    CHECK(state.rain == 0.f);
    CHECK(state.lightning == 0.f);
}
