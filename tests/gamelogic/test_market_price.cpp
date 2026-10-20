#include "doctest.h"
#include "GameLogic/Items/MarketPrice.h"
#include <limits>

using namespace GameLogic::Items::MarketPrice;
TEST_CASE("Market price accepts one positive supported currency and never truncates amounts")
{
    std::array<std::string, 7> fields{};
    CHECK_FALSE(Select(fields, true));
    fields[0] = "50000";
    REQUIRE(Select(fields, false));
    CHECK(Select(fields, false)->amount == 50000);
    fields[2] = "10";
    CHECK_FALSE(Select(fields, true));
    fields[0] = "0";
    CHECK_FALSE(Select(fields, false));
    REQUIRE(Select(fields, true));
    CHECK(Select(fields, true)->currency == 2);
    fields[2] = "2147483647";
    CHECK(Select(fields, true)->amount == std::numeric_limits<int>::max());
    for (const auto* invalid : {"2147483648", "-1", "1x", "1.5", "+1", " 2"})
    {
        fields[2] = invalid;
        CHECK_FALSE(Select(fields, true));
    }
}
TEST_CASE("Market currencies round trip through the personal shop price cache")
{
    for (auto currency : Currencies)
    {
        const auto price = ToPrice(currency, 70000);
        REQUIRE(price);
        CHECK(price->Amount == 70000);
        CHECK(Currency(*price) == currency);
    }
    CHECK(ToPrice(4, 1)->PriceItemType == 0xE010); // Life
    CHECK(ToPrice(9, 1)->PriceItemType == 0xC00F); // Chaos
    CHECK_FALSE(ToPrice(6, 1));
    CHECK_FALSE(ToPrice(2, 0));
}
