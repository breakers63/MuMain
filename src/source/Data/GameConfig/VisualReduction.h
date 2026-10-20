#pragma once
#include <array>
#include <cstddef>

namespace Data::Graphics
{
enum class VisualReduction : std::size_t
{
    Particles,
    Environment,
    Animals,
    Shadows,
    EquipmentGlow,
    Count
};
inline constexpr auto VisualReductionCount = static_cast<std::size_t>(VisualReduction::Count);
inline constexpr std::array<const wchar_t*, VisualReductionCount> VisualReductionKeys{
    L"ParticlesSprites", L"Environment", L"Boids", L"Shadows", L"ItemGlow"};
inline constexpr const wchar_t* VisualReductionSection = L"NativeAntilag";
}
