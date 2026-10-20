#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace GameLogic::Character::SoulAllocation
{
// Returning ranks refunds the corresponding share of the price originally paid.
// New ranks use the current configuration's cost, as in the OpenGL client.
template<class Snapshot, std::size_t N>
std::int64_t Cost(const Snapshot& data, const std::array<int, N>& draft)
{
    std::int64_t result = 0;
    for (std::size_t i = 0; i < N; ++i)
    {
        const auto paid = data.Paid[i];
        if (draft[i] >= data.Ranks[i])
            result += paid + std::int64_t(draft[i] - data.Ranks[i]) * data.Abilities[i].Cost;
        else if (draft[i] > 0 && data.Ranks[i] > 0)
            result += paid - paid * (data.Ranks[i] - draft[i]) / data.Ranks[i];
    }
    return result;
}

template<class Snapshot, std::size_t N>
bool Change(const Snapshot& data, std::array<int, N>& draft, int index, int delta)
{
    if (!data.Enabled || index < 0 || index >= static_cast<int>(N) || delta == 0) return false;
    const auto& rule = data.Abilities[index];
    if (delta > 0 && (!rule.Enabled || draft[index] >= rule.MaximumRank)) return false;
    if (delta < 0 && !data.AllowReallocation) return false;
    const int old = draft[index];
    draft[index] = std::clamp(old + delta, 0, std::max(data.Ranks[index], rule.MaximumRank));
    if (delta > 0 && Cost(data, draft) > data.Total) draft[index] = old;
    return draft[index] != old;
}
}
