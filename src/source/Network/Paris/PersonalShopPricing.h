#pragma once
#include <cstdint>

namespace Network::Paris::PersonalShopPricing
{
    bool CanUse(std::uint8_t currency);
    bool Send(int slot, int amount, std::uint8_t currency);
    const wchar_t* CurrencyName(std::uint8_t currency);
}
