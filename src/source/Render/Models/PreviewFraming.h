#pragma once
#include <algorithm>
#include <cmath>

namespace Render::MonsterPreviews::Framing
{
inline constexpr float Distance = 10000.f;
inline constexpr float FieldOfView = 1.f;
inline constexpr float Occupancy = .90f;

inline float Scale(float depth, float horizontal, float vertical, float aspect)
{
    if (!std::isfinite(depth) || !std::isfinite(horizontal) || !std::isfinite(vertical) ||
        !std::isfinite(aspect) || depth < 0 || horizontal <= 0 || vertical <= 0 || aspect <= 0) return 0;
    constexpr float DegreesToRadians = 0.01745329252f;
    const float verticalLimit = std::tan(FieldOfView * .5f * DegreesToRadians) * Occupancy;
    const float horizontalLimit = verticalLimit * aspect;
    // Fit the nearest corners too, so deep models cannot cross the image edges.
    return std::min(Distance * verticalLimit / (vertical + depth * verticalLimit),
                    Distance * horizontalLimit / (horizontal + depth * horizontalLimit));
}
}
