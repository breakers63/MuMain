#pragma once
#include <array>
namespace UI::Shortcuts
{
enum class Action { None, Vip, Ranking, Bank, Info, Market, Reset, Events, AddStats, BattlePass, KillLog, PostLog };
struct Binding { int key; bool control; Action action; };
inline constexpr int RankingKey = 0x76; // F7
inline constexpr std::array<Binding, 11> Bindings{{
    {'V', true, Action::Vip}, {RankingKey, false, Action::Ranking}, {'J', true, Action::Bank},
    {'I', true, Action::Info}, {'O', false, Action::Market}, {'Y', false, Action::Reset},
    {'H', false, Action::Events}, {'S', false, Action::AddStats}, {'B', true, Action::BattlePass},
    {'K', true, Action::KillLog}, {'P', true, Action::PostLog}
}};
inline constexpr Action Resolve(int key, bool control, bool alt, bool shift)
{
    if (alt || shift) return Action::None;
    for (const auto& binding : Bindings)
        if (binding.key == key && binding.control == control) return binding.action;
    return Action::None;
}
}
