#pragma once

namespace UI::Soul
{
    inline bool IsMilestoneUnlocked(bool systemEnabled, bool abilityEnabled, int committedRank, int requiredRank)
    {
        return systemEnabled && abilityEnabled && committedRank >= requiredRank;
    }
}
