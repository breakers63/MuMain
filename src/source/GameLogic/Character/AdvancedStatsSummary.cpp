#include "GameLogic/Character/AdvancedStatsSummary.h"

namespace GameLogic::Character::AdvancedStats
{
namespace
{
using Field = Network::Paris::AdvancedStatsField;
long long Get(const Values& values, Field field) { return values[static_cast<int>(field)]; }
long long WholePercent(long long value) { return (value + (value >= 0 ? 50 : -50)) / 100; }
long long Average(const Values& values, Field minimum, Field maximum)
{
    return (Get(values, minimum) + Get(values, maximum)) / 2;
}
}

long long ComputeTotalPower(const Values& values)
{
    long long physical = Average(values, Field::MinimumPhysBaseDmg, Field::MaximumPhysBaseDmg);
    if (Get(values, Field::HasDoubleWield) > 0)
        physical *= 2;
    long long total = physical + Average(values, Field::MinimumWizBaseDmg, Field::MaximumWizBaseDmg)
        + Average(values, Field::MinimumCurseBaseDmg, Field::MaximumCurseBaseDmg);
    for (Field field : {Field::CriticalDamageBonus, Field::ExcellentDamageBonus, Field::SkillDamageBonus, Field::FinalDamageBonus})
        total += Get(values, field);
    for (Field field : {Field::CriticalDamageChance, Field::ExcellentDamageChance, Field::DoubleDamageChance,
                        Field::DefenseIgnoreChance, Field::ShieldBypassChance})
        total += WholePercent(Get(values, field));
    return total + Get(values, Field::AttackRatePvm) / 100 + Get(values, Field::AttackRatePvp) / 100;
}

long long ComputeTotalDefense(const Values& values)
{
    long long total = Get(values, Field::DefensePvm);
    for (Field field : {Field::DamageReflection, Field::FullyReflectDamageAfterHitChance, Field::ArmorDamageDecrease,
                        Field::SoulBarrierReceiveDecrement, Field::ShieldRateIncrease, Field::ShieldDecreaseRateIncrease,
                        Field::ShieldBypassChance})
        total += WholePercent(Get(values, field));
    total += Get(values, Field::DefenseRatePvm) / 100 + Get(values, Field::DefenseRatePvp) / 100;
    for (Field field : {Field::IceResistance, Field::PoisonResistance, Field::LightningResistance, Field::FireResistance,
                        Field::EarthResistance, Field::WindResistance, Field::WaterResistance})
        total += Get(values, field);
    return total;
}
}
