#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <string_view>

namespace GameLogic::Items::StorePrice
{
    inline constexpr std::array<std::uint8_t, 7> Currencies{0, 1, 2, 3, 4, 5, 9};
    inline constexpr const wchar_t* Names[]{L"Zen", L"Paris Coins", L"Bless", L"Soul", L"Life", L"Creation", L"Chaos"};

    inline bool ParseAmount(std::wstring_view text, int& amount)
    {
        int value = 0;
        for (const wchar_t digit : text)
        {
            if (digit < L'0' || digit > L'9') return false;
            const int number = digit - L'0';
            if (value > (std::numeric_limits<int>::max() - number) / 10) return false;
            value = value * 10 + number;
        }
        amount = value;
        return true;
    }

    inline bool Select(const std::array<std::wstring_view, Currencies.size()>& inputs,
        bool extended, int& amount, std::uint8_t& currency)
    {
        int selected = -1, selectedAmount = 0;
        for (std::size_t index = 0; index < inputs.size(); ++index)
        {
            int value = 0;
            if (!ParseAmount(inputs[index], value)) return false;
            if (value == 0) continue;
            if (selected >= 0 || (!extended && Currencies[index] != 0)) return false;
            selected = static_cast<int>(index);
            selectedAmount = value;
        }
        if (selected < 0) return false;
        amount = selectedAmount;
        currency = Currencies[selected];
        return true;
    }
}
