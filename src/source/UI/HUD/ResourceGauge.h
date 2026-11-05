#pragma once

#include <algorithm>
#include <cstdint>

namespace UI::HUD
{
struct ResourceGauge
{
    std::uint32_t current;
    std::uint32_t maximum;
    float fraction;
};

inline ResourceGauge MakeResourceGauge(std::uint32_t current, std::uint32_t maximum)
{
    const auto value = std::min(current, maximum);
    return {value, maximum, maximum ? static_cast<float>(static_cast<double>(value) / maximum) : 0.f};
}
}
