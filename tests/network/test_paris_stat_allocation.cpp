#include "doctest.h"
#include "GameLogic/Character/StatAllocationState.h"
using GameLogic::Character::StatAllocationState;

TEST_CASE("full agility automatic assigns existing and future gains while closed")
{
    StatAllocationState state;
    state.full[1] = true;
    REQUIRE(state.StartAutomatic());
    CHECK_FALSE(state.percentMode);
    state.PlanAutomatic(20, 20, {500, 500, 500, 500, 0});
    CHECK(state.amounts[1] == 20);
    CHECK(state.Total() == 20);
    state.Stop(L"confirmed");
    state.open = false;
    CHECK(state.automatic);
    state.PlanAutomatic(5, 5, {500, 480, 500, 500, 0});
    CHECK(state.amounts[1] == 5);
    CHECK(state.Total() == 5);
    state.Stop(L"confirmed");
    state.PlanAutomatic(7, 7, {500, 475, 500, 500, 0});
    CHECK(state.amounts[1] == 7);
}

TEST_CASE("automatic full respects server room and current point balance")
{
    StatAllocationState state;
    state.full[1] = true;
    state.amounts[0] = 100;
    REQUIRE(state.StartAutomatic());
    state.PlanAutomatic(20, 4, {500, 3, 500, 500, 0});
    CHECK(state.amounts[1] == 3);
    CHECK(state.amounts[0] == 0);
    CHECK(state.Total() == 3);
    state.PlanAutomatic(5, 5, {500, 0, 500, 500, 0});
    CHECK(state.Total() == 0);
}

TEST_CASE("automatic requires a selected full stat or valid percentages")
{
    StatAllocationState state;
    CHECK_FALSE(state.StartAutomatic());
    CHECK_FALSE(state.automatic);
    state.percentMode = true;
    state.percentages[0] = 101;
    CHECK_FALSE(state.StartAutomatic());
}

TEST_CASE("percentage auto retains fractional shares across small gains")
{
    StatAllocationState state;
    state.percentMode = true;
    state.percentages[0] = state.percentages[1] = 50;
    REQUIRE(state.StartAutomatic());
    state.PlanAutomatic(1, 1, {500, 500, 500, 500, 0});
    CHECK(state.Total() == 0);
    state.PlanAutomatic(1, 2, {500, 500, 500, 500, 0});
    CHECK(state.amounts[0] == 1);
    CHECK(state.amounts[1] == 1);
}
