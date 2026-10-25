#pragma once

namespace UI::HUD::Performance
{
inline constexpr const char* Unavailable = "#666666";
inline constexpr const char* Good = "#33ff40";
inline constexpr const char* Fair = "#ff8c0d";
inline constexpr const char* Poor = "#ff261f";

inline const char* FpsColor(double fps)
{
    constexpr double GoodFps = 50, FairFps = 25;
    if (fps < 0) return Unavailable;
    if (fps >= GoodFps) return Good;
    return fps >= FairFps ? Fair : Poor;
}

inline const char* PingColor(int milliseconds)
{
    constexpr int GoodPingLimit = 100, FairPingLimit = 250;
    if (milliseconds < 0) return Unavailable;
    if (milliseconds < GoodPingLimit) return Good;
    return milliseconds <= FairPingLimit ? Fair : Poor;
}
}
