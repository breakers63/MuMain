#include "doctest.h"
#include "UI/HUD/ResourceGauge.h"
#include "UI/HUD/ExperienceProgress.h"
#include "UI/HUD/PerformanceQuality.h"
#include <limits>

TEST_CASE("Experience fills once per level and restarts at the next lower bound")
{
    CHECK(UI::HUD::MakeExperienceProgress(125000,100000,200000).levelFraction == doctest::Approx(.25));
    CHECK(UI::HUD::MakeExperienceProgress(200000,100000,200000).levelFraction == 1.f);
    CHECK(UI::HUD::MakeExperienceProgress(200000,200000,350000).levelFraction == 0.f);
    CHECK(UI::HUD::MakeExperienceProgress(215000,200000,350000).levelFraction == doctest::Approx(.1));
    CHECK(UI::HUD::MakeExperienceProgress(100,200000,350000).levelFraction == 0.f);
    CHECK(UI::HUD::MakeExperienceProgress(100,0,0).levelFraction == 0.f);
    const auto master=UI::HUD::MakeExperienceProgress(5000000000LL,4000000000LL,8000000000LL);
    CHECK(master.levelFraction == doctest::Approx(.25));
    CHECK(master.segmentFraction == doctest::Approx(.5));
    CHECK(master.segment == 2);
}

TEST_CASE("HUD resources retain server precision and show an empty depleted gauge")
{
    const auto large = UI::HUD::MakeResourceGauge(150000, 300000);
    CHECK(large.current == 150000);
    CHECK(large.fraction == doctest::Approx(.5));
    const auto empty = UI::HUD::MakeResourceGauge(0, 300000);
    CHECK(empty.current == 0);
    CHECK(empty.fraction == 0);
    CHECK(UI::HUD::MakeResourceGauge(50, 0).fraction == 0);
    CHECK(UI::HUD::MakeResourceGauge(400, 300).current == 300);
    const auto maximum = std::numeric_limits<std::uint32_t>::max();
    const auto full = UI::HUD::MakeResourceGauge(maximum, maximum);
    CHECK(full.current == maximum);
    CHECK(full.fraction == 1);
}

TEST_CASE("HUD performance colors match OpenGL thresholds")
{
    using namespace UI::HUD::Performance;
    CHECK(FpsColor(50) == Good);
    CHECK(FpsColor(49) == Fair);
    CHECK(FpsColor(24) == Poor);
    CHECK(PingColor(-1) == Unavailable);
    CHECK(PingColor(99) == Good);
    CHECK(PingColor(100) == Fair);
    CHECK(PingColor(250) == Fair);
    CHECK(PingColor(251) == Poor);
}
