#include "GameLogic/Character/AdvancedStatsSummary.h"
namespace GameLogic::Character::AdvancedStats
{
int PercentBasisPointsToWhole(std::int32_t value)
{
    if (value >= 0)
    {
        return static_cast<int>((static_cast<long long>(value) + 50) / 100);
    }

    return static_cast<int>((static_cast<long long>(value) - 50) / 100);
}

long long SafeAverage(std::int32_t a, std::int32_t b)
{
    return (static_cast<long long>(a) + static_cast<long long>(b)) / 2LL;
}

long long ComputeTotalPower(const std::array<std::int32_t, FieldCount>& values)
{
    using Field = Network::Paris::AdvancedStatsField;
    auto get = [&](Field field) -> std::int32_t { return values[static_cast<int>(field)]; };

    long long total = 0;

    // OpenMU stores the displayed physical base damage at 55% while two
    // one-handed weapons are equipped, and doubles the selected physical
    // hit later in the real attack calculation (55% + 55% = 110%).
    // Apply the same rule here so dual wield never appears weaker.
    long long physicalDamage = SafeAverage(get(Field::MinimumPhysBaseDmg), get(Field::MaximumPhysBaseDmg));
    if (get(Field::HasDoubleWield) > 0)
    {
        physicalDamage *= 2LL;
    }

    total += physicalDamage;
    total += SafeAverage(get(Field::MinimumWizBaseDmg), get(Field::MaximumWizBaseDmg));
    total += SafeAverage(get(Field::MinimumCurseBaseDmg), get(Field::MaximumCurseBaseDmg));

    total += get(Field::CriticalDamageBonus);
    total += get(Field::ExcellentDamageBonus);
    total += get(Field::SkillDamageBonus);
    total += get(Field::FinalDamageBonus);

    total += PercentBasisPointsToWhole(get(Field::CriticalDamageChance));
    total += PercentBasisPointsToWhole(get(Field::ExcellentDamageChance));
    total += PercentBasisPointsToWhole(get(Field::DoubleDamageChance));
    total += PercentBasisPointsToWhole(get(Field::DefenseIgnoreChance));
    total += PercentBasisPointsToWhole(get(Field::ShieldBypassChance));

    total += get(Field::AttackRatePvm) / 100;
    total += get(Field::AttackRatePvp) / 100;

    return total;
}

long long ComputeTotalDefense(const std::array<std::int32_t, FieldCount>& values)
{
    using Field = Network::Paris::AdvancedStatsField;
    auto get = [&](Field field) -> std::int32_t { return values[static_cast<int>(field)]; };

    long long total = 0;
    total += PercentBasisPointsToWhole(get(Field::DamageReflection));
    total += PercentBasisPointsToWhole(get(Field::FullyReflectDamageAfterHitChance));
    total += PercentBasisPointsToWhole(get(Field::ArmorDamageDecrease));
    total += PercentBasisPointsToWhole(get(Field::SoulBarrierReceiveDecrement));
    total += PercentBasisPointsToWhole(get(Field::ShieldRateIncrease));
    total += PercentBasisPointsToWhole(get(Field::ShieldDecreaseRateIncrease));
    total += PercentBasisPointsToWhole(get(Field::ShieldBypassChance));

    total += get(Field::DefenseRatePvm) / 100;
    total += get(Field::DefenseRatePvp) / 100;

    total += get(Field::IceResistance);
    total += get(Field::PoisonResistance);
    total += get(Field::LightningResistance);
    total += get(Field::FireResistance);
    total += get(Field::EarthResistance);
    total += get(Field::WindResistance);
    total += get(Field::WaterResistance);

    return total;
}
} // namespace GameLogic::Character::AdvancedStats
