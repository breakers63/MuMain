#include "doctest.h"
#include "UI/NewUI/Character/ExperienceProgress.h"

TEST_CASE("Paris experience starts at the current level boundary")
{
    using UI::Character::ExperienceProgress;
    CHECK(ExperienceProgress(100, 440, 2, false) == 0);
    CHECK(ExperienceProgress(270, 440, 2, false) == doctest::Approx(.5));
    CHECK(ExperienceProgress(440, 440, 2, false) == 1);
}

TEST_CASE("Paris experience clamps stale and invalid values")
{
    using UI::Character::ExperienceProgress;
    CHECK(ExperienceProgress(0, 440, 2, false) == 0);
    CHECK(ExperienceProgress(9000000000LL, 440, 2, false) == 1);
    CHECK(ExperienceProgress(270, 100, 2, false) == 0);
    CHECK(ExperienceProgress(0, 0, 1, true) == 0);
}
