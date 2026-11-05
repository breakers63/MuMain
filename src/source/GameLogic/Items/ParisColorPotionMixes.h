#pragma once

#include "MixMgr.h"
#include "ParisColorPotions.h"

namespace GameLogic::Items::ColorPotions
{
inline SEASON3A::MIX_RECIPE* CreateRecipe(int channel, int index)
{
    using namespace SEASON3A;
    auto* recipe = new MIX_RECIPE{}; // Ownership passes to CMixRecipes.
    recipe->m_iMixIndex = index;
    recipe->m_iMixID = FirstMix + channel;
    recipe->m_iWidth = recipe->m_iHeight = 1;
    recipe->m_iNumRateData = 1;
    recipe->m_RateToken[0] = { MRCP_NUMBER, 75.f };
    recipe->m_iSuccessRate = 75;
    recipe->m_iNumMixSoruces = 2;
    constexpr int Jewels[] = { ITEM_POTION + 14, ITEM_POTION + 22, ITEM_POTION + 13 };
    for (int sourceIndex = 0; sourceIndex < 2; ++sourceIndex)
    {
        auto& source = recipe->m_MixSources[sourceIndex];
        source.m_sTypeMin = source.m_sTypeMax = sourceIndex == 0 ? ITEM_WING + 15 : Jewels[channel];
        source.m_iLevelMax = source.m_iOptionMax = source.m_iDurabilityMax = 255;
        source.m_iCountMin = source.m_iCountMax = sourceIndex == 0 ? 1 : 5;
    }
    return recipe;
}
}
