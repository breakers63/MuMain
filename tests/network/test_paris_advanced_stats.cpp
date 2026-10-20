#include "doctest.h"
#include "GameLogic/Character/AdvancedStatsSummary.h"
#include <limits>

TEST_CASE("Paris summary preserves the original dual wield rule")
{
    using namespace GameLogic::Character::AdvancedStats;
    using Field = Network::Paris::AdvancedStatsField;
    Values values{};
    values[static_cast<int>(Field::MinimumPhysBaseDmg)] = 55;
    values[static_cast<int>(Field::MaximumPhysBaseDmg)] = 55;
    CHECK(ComputeTotalPower(values) == 55);
    values[static_cast<int>(Field::HasDoubleWield)] = 1;
    CHECK(ComputeTotalPower(values) == 110);
    CHECK(ComputeTotalDefense(values) == 0);
}

TEST_CASE("Paris summary uses wide arithmetic for extreme server values")
{
    using namespace GameLogic::Character::AdvancedStats;
    using Field = Network::Paris::AdvancedStatsField;
    Values values{};
    values[static_cast<int>(Field::MinimumPhysBaseDmg)] = std::numeric_limits<int>::max();
    values[static_cast<int>(Field::MaximumPhysBaseDmg)] = std::numeric_limits<int>::max();
    values[static_cast<int>(Field::HasDoubleWield)] = 1;
    CHECK(ComputeTotalPower(values) == 4294967294LL);
    values[static_cast<int>(Field::DamageReflection)] = std::numeric_limits<int>::max();
    CHECK(ComputeTotalDefense(values) == 21474836);
}
