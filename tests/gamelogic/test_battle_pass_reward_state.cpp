#include "doctest.h"
#include "GameLogic/Social/BattlePassRewardState.h"

TEST_CASE("Battle Pass preserves claimed icons across premium expiration")
{
    using GameLogic::Social::BattlePassRewardState::Unlocked;
    CHECK(Unlocked(false, 0, 10, true, false, true));
    CHECK(Unlocked(true, 10, 10, false, false, false));
    CHECK_FALSE(Unlocked(true, 10, 10, true, false, false));
    CHECK(Unlocked(true, 10, 10, true, true, false));
    CHECK_FALSE(Unlocked(true, 9, 10, false, true, false));
    CHECK_FALSE(Unlocked(false, 10, 10, false, true, false));
}
