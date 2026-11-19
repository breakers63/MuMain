#include "doctest.h"
#include "Render/Weather/PrecipitationFall.h"
#include <initializer_list>

using namespace Render::Weather;

namespace
{
int CountLandings(int framesPerSecond)
{
    constexpr int DurationSeconds = 60;
    std::uint64_t previousCycle = 0;
    int landings = 0;
    for (int frame = 0; frame <= framesPerSecond * DurationSeconds; ++frame)
    {
        const auto elapsed = static_cast<std::uint64_t>(frame) * 1000 / framesPerSecond;
        const auto fall = SamplePrecipitationFall(elapsed, .25f, 100.f, 1500.f);
        landings += HasLanded(previousCycle, fall.cycle) ? 1 : 0;
        previousCycle = fall.cycle;
    }
    return landings;
}
}

TEST_CASE("snow lands once per fall at every display refresh rate")
{
    for (int fps : {25, 30, 60, 144, 240})
    {
        CAPTURE(fps);
        CHECK(CountLandings(fps) == 4);
    }
}

TEST_CASE("snow landing waits for the floor crossing and does not repeat")
{
    const auto before = SamplePrecipitationFall(11249, .25f, 100.f, 1500.f);
    const auto after = SamplePrecipitationFall(11250, .25f, 100.f, 1500.f);
    CHECK(before.height == doctest::Approx(.1f));
    CHECK(after.height == doctest::Approx(1500.f));
    CHECK(HasLanded(before.cycle, after.cycle));
    CHECK_FALSE(HasLanded(after.cycle, after.cycle));
    CHECK_FALSE(HasLanded(0, after.cycle));
    CHECK_FALSE(HasLanded(10, 1));
}

TEST_CASE("snow fall keeps its height bounded during long sessions")
{
    for (std::uint64_t time : {0ULL, 1ULL, 3600000ULL, 86400000ULL})
    {
        const auto fall = SamplePrecipitationFall(time, .73f, 100.f, 1500.f);
        CHECK(fall.height >= 0.f);
        CHECK(fall.height <= 1500.f);
        CHECK(fall.cycle != 0);
    }
}
