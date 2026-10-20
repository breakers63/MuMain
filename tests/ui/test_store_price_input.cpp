#include "doctest.h"
#include "GameLogic/Items/StorePriceInput.h"

namespace Prices = GameLogic::Items::StorePrice;

TEST_CASE("personal shop accepts exactly one supported currency")
{
    for (std::size_t index = 0; index < Prices::Currencies.size(); ++index)
    {
        std::array<std::wstring_view, 7> rows{};
        rows[index] = L"2147483647";
        int amount = 0;
        std::uint8_t currency = 255;
        REQUIRE(Prices::Select(rows, true, amount, currency));
        CHECK(amount == 2147483647);
        CHECK(currency == Prices::Currencies[index]);
        CHECK(Prices::Select(rows, false, amount, currency) == (index == 0));
    }
}

TEST_CASE("personal shop rejects ambiguous, zero, negative and overflowing prices")
{
    std::array<std::wstring_view, 7> rows{};
    int amount = 17;
    std::uint8_t currency = 9;
    CHECK_FALSE(Prices::Select(rows, true, amount, currency));
    rows[0] = L"123";
    rows[1] = L"1";
    CHECK_FALSE(Prices::Select(rows, true, amount, currency));
    rows[1] = L"0";
    REQUIRE(Prices::Select(rows, true, amount, currency));
    CHECK(amount == 123);
    CHECK(currency == 0);
    for (auto invalid : {L"-1", L"1x", L"2147483648", L"99999999999999999999", L"1.5", L"+1"})
    {
        rows[0] = invalid;
        CHECK_FALSE(Prices::Select(rows, true, amount, currency));
    }
}
