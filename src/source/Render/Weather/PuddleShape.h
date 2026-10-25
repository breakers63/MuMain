#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace Render::Weather::Puddles {
inline constexpr int Segments = 20;
inline constexpr float Tau = 6.28318530718f;
inline constexpr std::array<float,4> Rings = {0.f,.52f,.8f,1.f};

// Shared ring vertices are sampled once per patch, including one common center.
template<class Sampler>
auto SamplePoints(Sampler sample)
{
    using Point = decltype(sample(0.f,0.f));
    std::array<std::array<Point,Segments>,Rings.size()> points;
    points[0].fill(sample(0.f,0.f));
    for(std::size_t ring=1;ring<Rings.size();++ring)
        for(int i=0;i<Segments;++i)points[ring][i]=sample(i*Tau/Segments,Rings[ring]);
    return points;
}

inline float Hash(std::uint32_t value)
{
    value ^= value >> 16; value *= 0x7feb352dU; value ^= value >> 15;
    value *= 0x846ca68bU; value ^= value >> 16;
    return (value & 0xFFFFFFU) / 16777215.f;
}
inline std::uint32_t Seed(int x,int y,int map)
{
    return static_cast<std::uint32_t>(x)*73856093U ^ static_cast<std::uint32_t>(y)*19349663U ^ static_cast<std::uint32_t>(map)*83492791U;
}
inline float EdgeRadius(float angle,std::uint32_t seed)
{
    constexpr float Tau=6.28318530718f;
    const float phase=Hash(seed)*Tau;
    return .83f+.10f*std::sin(angle*3+phase)+.07f*std::cos(angle*5-phase);
}
inline float Opacity(float wetness,float radius)
{
    const float fill=std::clamp((wetness-.12f)/.65f,0.f,1.f);
    const float edge=std::clamp((1.f-radius)/.24f,0.f,1.f);
    return fill*edge*.43f;
}
}
