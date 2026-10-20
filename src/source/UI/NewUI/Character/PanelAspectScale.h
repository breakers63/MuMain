#pragma once
#include <algorithm>
#include <cmath>

namespace UI::Character::PanelAspect
{
    inline float Fit(float screenWidth, float screenHeight, float designWidth, float designHeight)
    {
        constexpr float ScreenCoverage = .96f;
        return std::max(.01f, std::min(screenWidth * ScreenCoverage / designWidth, screenHeight * ScreenCoverage / designHeight));
    }
    inline int Axis(int value, float scale, float screenRate)
    {
        return static_cast<int>(std::lround(value * scale / std::max(.01f, screenRate)));
    }
}
