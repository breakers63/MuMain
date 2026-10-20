#pragma once
#include "GameLogic/Items/PersonalShopCurrency.h"
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>

namespace GameLogic::Items::MarketPrice
{
inline constexpr std::array<std::uint8_t, 7> Currencies{0, 1, 2, 3, 4, 5, 9};
inline constexpr std::array<std::uint16_t, 7> PriceTypes{0, 0xFFF0, 0xE00D, 0xE00E, 0xE010, 0xE016, 0xC00F};
struct Selection { std::uint8_t currency = 0; int amount = 0; };
inline std::optional<int> Amount(std::string_view text)
{
    if (text.empty()) return 0;
    for (char c : text) if (c < '0' || c > '9') return std::nullopt;
    int amount = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), amount);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return std::nullopt;
    return amount;
}
inline std::optional<Selection> Select(const std::array<std::string, 7>& amounts, bool extended)
{
    std::optional<Selection> selected;
    for (std::size_t i = 0; i < amounts.size(); ++i)
    {
        const auto value = Amount(amounts[i]);
        if (!value) return std::nullopt;
        if (*value == 0) continue;
        if (selected || (i != 0 && !extended)) return std::nullopt;
        selected = Selection{Currencies[i], *value};
    }
    return selected;
}
inline std::optional<PersonalShop::ItemPrice> ToPrice(std::uint8_t currency, int amount)
{
    if (amount <= 0) return std::nullopt;
    for (std::size_t i = 0; i < Currencies.size(); ++i)
        if (currency == Currencies[i])
            return PersonalShop::ItemPrice{amount, i == 0 ? PersonalShop::PriceKind::Zen : i == 1 ? PersonalShop::PriceKind::AccountCoin : PersonalShop::PriceKind::Item, PriceTypes[i]};
    return std::nullopt;
}
inline std::optional<std::uint8_t> Currency(const PersonalShop::ItemPrice& price)
{
    if (price.Kind == PersonalShop::PriceKind::Zen) return 0;
    for (std::size_t i = 1; i < Currencies.size(); ++i)
        if (price.PriceItemType == PriceTypes[i]) return Currencies[i];
    return std::nullopt;
}
}
