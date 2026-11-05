#pragma once

#include <cmath>
#include <cstdint>

namespace Render::Weather
{
struct PrecipitationFall
{
    float height;
    std::uint64_t cycle;
};

// Cycle zero is reserved for a particle that has not been observed yet.
inline PrecipitationFall SamplePrecipitationFall(std::uint64_t elapsedMs, float initialFraction,
                                                float speed, float columnHeight)
{
    const double distance = initialFraction * columnHeight + static_cast<double>(elapsedMs) * speed / 1000.0;
    const double cycle = std::floor(distance / columnHeight);
    return {static_cast<float>(columnHeight - (distance - cycle * columnHeight)),
            static_cast<std::uint64_t>(cycle) + 1};
}

inline bool HasLanded(std::uint64_t previousCycle, std::uint64_t cycle)
{
    return previousCycle != 0 && cycle > previousCycle;
}
} // namespace Render::Weather
