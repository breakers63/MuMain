#pragma once

#include <cstdint>

namespace Network::Paris
{
bool RequestJewelBank(const wchar_t* character, const wchar_t* command);
bool RequestVip(int plan, bool purchase, std::uint64_t quote);
bool RequestAdvancedStats();
bool RequestStatLimits(const wchar_t* character, std::uint32_t request);
bool RequestStatAllocation(const wchar_t* character, int stat, int amount);
bool RequestReset(bool reset, std::uint64_t token);
bool RequestBattlePass(std::uint8_t operation, int level, bool premium);
} // namespace Network::Paris
