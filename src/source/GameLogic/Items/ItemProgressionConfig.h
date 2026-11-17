#pragma once
#include <string>

namespace GameLogic::Items::Progression {
constexpr int MaximumTier = 8;
int TierFor(int itemType, bool excellent);
const wchar_t* Suffix(int tier);
} // namespace GameLogic::Items::Progression
