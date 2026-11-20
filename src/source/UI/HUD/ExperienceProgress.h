#pragma once
#include <algorithm>
#include <cstdint>

namespace UI::HUD
{
struct ExperienceProgress
{
    float levelFraction;
    float segmentFraction;
    int segment;
};

inline ExperienceProgress MakeExperienceProgress(std::int64_t current, std::int64_t lower, std::int64_t upper)
{
    constexpr int SegmentsPerLevel = 10;
    if (upper <= lower) return {0.f,0.f,0};
    const double progress = static_cast<double>(std::clamp(current,lower,upper)-lower) / static_cast<double>(upper-lower);
    const double scaled = progress * SegmentsPerLevel;
    const int segment = std::min(static_cast<int>(scaled),SegmentsPerLevel-1);
    return {static_cast<float>(progress),static_cast<float>(scaled-segment),segment};
}
}
