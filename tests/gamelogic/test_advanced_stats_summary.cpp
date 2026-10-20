#include "doctest.h"
#include "GameLogic/Character/AdvancedStatsSummary.h"
#include <climits>
using namespace GameLogic::Character::AdvancedStats;
using Field = Network::Paris::AdvancedStatsField;
namespace { void Set(Values& values, Field field, int value) { values[static_cast<int>(field)] = value; } }

TEST_CASE("Power preserves the OpenGL dual wield rule without overflowing")
{
    Values values{};
    Set(values, Field::MinimumPhysBaseDmg, INT_MAX);
    Set(values, Field::MaximumPhysBaseDmg, INT_MAX);
    CHECK(ComputeTotalPower(values) == INT_MAX);
    Set(values, Field::HasDoubleWield, 1);
    CHECK(ComputeTotalPower(values) == 2LL * INT_MAX);
    Set(values, Field::MinimumWizBaseDmg, 100);
    Set(values, Field::MaximumWizBaseDmg, 200);
    Set(values, Field::CriticalDamageBonus, 70);
    Set(values, Field::CriticalDamageChance, 4050);
    Set(values, Field::AttackRatePvm, 12345);
    CHECK(ComputeTotalPower(values) == 2LL * INT_MAX + 150 + 70 + 41 + 123);
}

TEST_CASE("Defense includes server PvM defense and original rounded percentages")
{
    Values values{};
    Set(values, Field::DefensePvm, 7156);
    Set(values, Field::ArmorDamageDecrease, 5400);
    Set(values, Field::DamageReflection, 149);
    Set(values, Field::ShieldRateIncrease, 150);
    Set(values, Field::DefenseRatePvm, 38300);
    CHECK(ComputeTotalDefense(values) == 7596);
    Set(values, Field::DamageReflection, -150);
    Set(values, Field::IceResistance, 10);
    CHECK(ComputeTotalDefense(values) == 7603);
}
