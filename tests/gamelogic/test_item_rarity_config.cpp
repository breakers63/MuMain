#include "doctest.h"
#include "GameLogic/Items/ItemRarityConfig.h"
#include "../../src/ThirdParty/json.hpp"
#include <fstream>
#include <iterator>

namespace
{
std::string Sample()
{
    std::ifstream file(std::string(MU_CLIENT_SOURCE_DIR) + "/../bin/Data/Custom/ParisItemRarity.json");
    return {std::istreambuf_iterator<char>(file), {}};
}
}

TEST_CASE("Rarity configuration preserves OpenGL equipment bonuses and translations")
{
    using namespace GameLogic::Items::Rarity;
    const auto sample = Sample();
    REQUIRE(Replace(sample.data(), static_cast<int>(sample.size())));
    const auto& config = Current();
    CHECK(config.Enabled);
    CHECK(ExcellentCount(0xff) == 6);
    CHECK(ExcellentCount(0xc0) == 0);
    CHECK(Localized(config.Tiers[3].Name, config.Tiers[3].NameTranslations, 0) == L"LEGENDARY");
    const int armorCategory = config.CategoryByItem[3584];
    REQUIRE(armorCategory >= 0);
    const auto& bonuses = config.Categories[armorCategory].Bonuses;
    REQUIRE(bonuses.size() == 3);
    CHECK(bonuses[0].Level == 13);
    CHECK(bonuses[1].Level == 14);
    CHECK(bonuses[2].Level == 15);
    CHECK(bonuses[0].Values[3] == 6);
    CHECK(bonuses[2].Values[3] == 12);
}

TEST_CASE("Invalid server rarity updates retain the last valid configuration")
{
    using namespace GameLogic::Items::Rarity;
    const auto sample = Sample();
    REQUIRE(Replace(sample.data(), static_cast<int>(sample.size())));
    const auto expected = Current().Tiers[3].Border;
    auto json = nlohmann::json::parse(sample);
    json["Rarities"][3]["Border"][0] = 999;
    const auto invalid = json.dump();
    CHECK_FALSE(Replace(invalid.data(), static_cast<int>(invalid.size())));
    CHECK_FALSE(Replace(nullptr, 10));
    CHECK_FALSE(Replace("{", 1));
    CHECK_FALSE(Replace(sample.data(), MaximumConfigurationBytes + 1));
    CHECK(Current().Tiers[3].Border == expected);
}
