#include "doctest.h"
#include "Render/Models/PreviewFraming.h"
#include <limits>

TEST_CASE("Monster preview fits large and wide monsters without a fixed minimum scale")
{
    using namespace Render::MonsterPreviews::Framing;
    const float extent = 1000;
    const float wide = Scale(extent, extent, extent, 1);
    const float narrow = Scale(extent, extent, extent, .5f);
    CHECK(wide > 0);
    CHECK(wide < .1f);
    CHECK(narrow < wide);
    CHECK(Scale(extent * 10, extent * 10, extent * 10, 1) == doctest::Approx(wide / 10));
    constexpr float Radians = .01745329252f;
    const float halfHeight = Distance * std::tan(FieldOfView * .5f * Radians);
    // The nearest corners stay inside both limits, including a narrow image.
    CHECK(extent * wide / (Distance - extent * wide) < halfHeight / Distance);
    CHECK(extent * narrow / (Distance - extent * narrow) < halfHeight * .5f / Distance);
    CHECK(Scale(1000, 50, 50, 1) > Scale(50, 1000, 50, 1));
}

TEST_CASE("Invalid preview framing inputs cannot create inverted or infinite models")
{
    using Render::MonsterPreviews::Framing::Scale;
    CHECK(Scale(0, 0, 0, 1) == 0);
    CHECK(Scale(-1, 100, 100, 1) == 0);
    CHECK(Scale(100, 100, 100, 0) == 0);
    CHECK(Scale(std::numeric_limits<float>::infinity(), 100, 100, 1) == 0);
}
