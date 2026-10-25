#include "doctest.h"
#include "GameLogic/Character/StatAllocationState.h"

using GameLogic::Character::StatAllocationState;

TEST_CASE("Bulk allocation caps full attributes at server room and live balance")
{
    StatAllocationState state;
    state.full = {true, true, false, false, false};
    state.amounts[2] = 20;
    state.PlanFull(150, {100, 100, 100, 100, 100});
    CHECK(state.amounts == std::array<int, 5>{100, 50, 0, 0, 0});
    CHECK(state.Total() == 150);
    state.PlanFull(500000, {100000, 100000, 100, 100, 100});
    CHECK(state.amounts[0] == StatAllocationState::MaxAmount);
    CHECK(state.amounts[1] == StatAllocationState::MaxAmount);
}

TEST_CASE("Automatic percentages retain fractional shares across point gains")
{
    StatAllocationState state;
    state.percentMode = true;
    state.percentages = {50, 50, 0, 0, 0};
    REQUIRE(state.StartAutomatic());
    const std::array<int, 5> room{100, 100, 100, 100, 100};
    state.PlanAutomatic(1, 1, room);
    CHECK(state.Total() == 0);
    state.PlanAutomatic(1, 2, room);
    CHECK(state.amounts == std::array<int, 5>{1, 1, 0, 0, 0});
    CHECK(state.Total() <= 2);
    state.PlanPercent(100, {0, 10, 0, 0, 0});
    CHECK(state.amounts[0] == 0);
    CHECK(state.amounts[1] == 10);
}

TEST_CASE("Automatic allocation rejects empty or excessive percentages and empty full selection")
{
    StatAllocationState state;
    CHECK_FALSE(state.StartAutomatic());
    state.percentMode = true;
    CHECK_FALSE(state.StartAutomatic());
    state.percentages = {60, 60, 0, 0, 0};
    CHECK_FALSE(state.StartAutomatic());
    state.PlanPercent(200, {100, 100, 100, 100, 100});
    CHECK(state.Total() == 0);
    state.percentages = {100, 0, 0, 0, 0};
    REQUIRE(state.StartAutomatic());
    state.PlanAutomatic(200, 20, {100, 100, 100, 100, 100});
    CHECK(state.Total() == 20);
}
