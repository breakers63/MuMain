#pragma once
#include "Network/Paris/AdvancedStatsField.h"
#include <array>
#include <cstdint>
namespace GameLogic::Character::AdvancedStats
{
constexpr int FieldCount = static_cast<int>(Network::Paris::AdvancedStatsField::Count);
using Values = std::array<std::int32_t, FieldCount>;
long long ComputeTotalPower(const Values& values);
long long ComputeTotalDefense(const Values& values);
} // namespace GameLogic::Character::AdvancedStats
