#include "doctest.h"
#include "GameLogic/Character/SoulAllocation.h"
#include <array>

namespace
{
struct Rule { bool Enabled = true; int MaximumRank = 100, Cost = 10; };
struct Snapshot
{
    bool Enabled = true, AllowReallocation = true;
    long long Total = 1000;
    std::array<int, 2> Ranks{10, 0};
    std::array<long long, 2> Paid{55, 0};
    std::array<Rule, 2> Abilities;
};
}
TEST_CASE("Soul rank changes use current costs and refund historical paid cost proportionally")
{
    Snapshot data;
    auto draft = data.Ranks;
    CHECK(GameLogic::Character::SoulAllocation::Cost(data, draft) == 55);
    REQUIRE(GameLogic::Character::SoulAllocation::Change(data, draft, 0, -1));
    CHECK(GameLogic::Character::SoulAllocation::Cost(data, draft) == 50);
    REQUIRE(GameLogic::Character::SoulAllocation::Change(data, draft, 0, -10));
    CHECK(draft[0] == 0);
    CHECK(GameLogic::Character::SoulAllocation::Cost(data, draft) == 0);
    REQUIRE(GameLogic::Character::SoulAllocation::Change(data, draft, 1, 10));
    CHECK(GameLogic::Character::SoulAllocation::Cost(data, draft) == 100);
}
TEST_CASE("Soul cannot overspend or bypass rank and server configuration limits")
{
    Snapshot data; auto draft = data.Ranks;
    data.Total = 60;
    CHECK_FALSE(GameLogic::Character::SoulAllocation::Change(data, draft, 1, 1));
    CHECK(draft == data.Ranks);
    data.AllowReallocation = false;
    CHECK_FALSE(GameLogic::Character::SoulAllocation::Change(data, draft, 0, -1));
    data.Total = 10000; data.Abilities[1].Enabled = false;
    CHECK_FALSE(GameLogic::Character::SoulAllocation::Change(data, draft, 1, 10));
    data.Abilities[1].Enabled = true; data.Abilities[1].MaximumRank = 3;
    REQUIRE(GameLogic::Character::SoulAllocation::Change(data, draft, 1, 10));
    CHECK(draft[1] == 3);
    CHECK_FALSE(GameLogic::Character::SoulAllocation::Change(data, draft, 1, 1));
    CHECK_FALSE(GameLogic::Character::SoulAllocation::Change(data, draft, -1, 1));
    data.Enabled = false;
    CHECK_FALSE(GameLogic::Character::SoulAllocation::Change(data, draft, 0, 1));
}
