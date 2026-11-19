#include "doctest.h"
#include "UI/HUD/ResourceGauge.h"
#include "UI/HUD/PerformanceQuality.h"
#include <limits>

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
