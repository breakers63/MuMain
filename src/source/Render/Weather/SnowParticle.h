#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Render::Weather
{
struct SnowParticle
{
    float x, y, z, size, rotation, opacity;
};

inline float SnowHash(unsigned value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return (value & 0xFFFFFFU) / 16777215.f;
}

// OpenGL Paris snow trajectories, sampled from elapsed time rather than frame count.
inline SnowParticle SampleSnowParticle(unsigned index, std::uint64_t elapsedMs, bool blizzard, float amount, float wind)
{
    constexpr float Tau = 6.2831853f, VerticalRange = 1200.f, SnowSize = 11.f;
    const double seconds = elapsedMs * .001;
    const float wave = SnowHash(index * 29U + 151U) * Tau;
    const float variation = SnowHash(index * 47U + 193U);
    const float radius = blizzard ? 1350.f : 1150.f;
    const float speed = blizzard ? 380.f : 170.f;
    const float travel = static_cast<float>(std::fmod(seconds * speed + SnowHash(index * 23U + 97U) * VerticalRange, VerticalRange));
    const float x = (SnowHash(index * 11U + 13U) * 2.f - 1.f) * radius +
                    static_cast<float>(std::sin(seconds * (blizzard ? 1.10 : .85) + wave)) * (blizzard ? 62.f : 42.f) +
                    wind * travel * (blizzard ? .64f : .20f);
    const float y = (SnowHash(index * 17U + 43U) * 2.f - 1.f) * radius +
                    static_cast<float>(std::cos(seconds * (blizzard ? .88 : .65) + wave)) * (blizzard ? 40.f : 28.f);
    return {x, y, VerticalRange - travel - (blizzard ? 100.f : 80.f),
            SnowSize * (blizzard ? (.65f + variation * .70f) : (.75f + variation * .95f)),
            static_cast<float>(std::fmod(seconds * (.35f + variation * 1.10f) + wave, Tau)),
            std::clamp((blizzard ? .82f : .72f) * amount, 0.f, .92f) * (.68f + variation * .32f)};
}
}
