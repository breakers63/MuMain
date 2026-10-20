#include "doctest.h"
#include "UI/NewUI/HUD/Skills/SkillIconSelection.h"
#include <array>
TEST_CASE("Skill icons resolve slots separately from pet commands")
{
    std::array<int, 650> skills{};
    skills[3] = 41;
    skills[150] = 345;
    const std::span<const int> slots(skills);
    CHECK(UI::Skills::ResolveIconSkill(3, slots, 120, 125) == 41);
    CHECK(UI::Skills::ResolveIconSkill(150, slots, 120, 125) == 345);
    CHECK(UI::Skills::ResolveIconSkill(120, slots, 120, 125) == 120);
    CHECK(UI::Skills::ResolveIconSkill(124, slots, 120, 125) == 124);
    CHECK(UI::Skills::ResolveIconSkill(-1, slots, 120, 125) == 0);
    CHECK(UI::Skills::ResolveIconSkill(650, slots, 120, 125) == 0);
    CHECK(UI::Skills::ResolveIconSkill(5, slots, 120, 125) == 0);
}
