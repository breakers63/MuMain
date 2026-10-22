#pragma once
#include <cmath>
namespace UI::Gestures
{
constexpr int Count = 25;
constexpr double Tau = 6.283185307179586;
constexpr double HubRadius = 0.17;
constexpr double MiddleRadius = 0.40;
inline int Pick(double x, double y)
{
    const double radius = std::hypot(x, y);
    if (!std::isfinite(radius) || radius < HubRadius || radius > 1.0) return -1;
    const int count = radius < .40 ? 9 : radius < .64 ? 16 : radius < .83 ? 7 : 5;
    const int first = radius < .40 ? 0 : radius < .64 ? 9 : radius < .83 ? 25 : 32;
    double angle = std::atan2(x, -y) + Tau / (2 * count);
    if (angle < 0) angle += Tau;
    if (angle >= Tau) angle -= Tau;
    return first + static_cast<int>(angle * count / Tau);
}
}
