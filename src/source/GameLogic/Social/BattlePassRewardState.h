#pragma once
namespace GameLogic::Social::BattlePassRewardState
{
    inline bool Unlocked(bool active, int currentLevel, int rewardLevel,
        bool premiumReward, bool premiumActive, bool claimed)
    {
        return claimed || (active && currentLevel >= rewardLevel && (!premiumReward || premiumActive));
    }
}
