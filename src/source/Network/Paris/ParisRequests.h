#pragma once
#include <cstdint>
namespace Network::Paris
{
bool RequestJewelBank(const wchar_t* character, const wchar_t* command);
bool RequestVip(int plan, bool purchase, std::uint64_t quote);
bool RequestAdvancedStats();
bool RequestReset(bool reset, std::uint64_t token);
} // namespace Network::Paris
