#pragma once

#include "Network/Paris/AdvancedStatsSnapshot.h"

namespace GameLogic::Character::AdvancedStats
{
using Values = decltype(Network::Paris::AdvancedStatsSnapshot::values);
long long ComputeTotalPower(const Values& values);
long long ComputeTotalDefense(const Values& values);
}
