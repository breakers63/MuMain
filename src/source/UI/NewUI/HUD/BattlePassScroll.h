#pragma once
#include <algorithm>
namespace UI::BattlePass::Scroll
{
inline int MaximumOffset(int count, int visible) { return std::max(0, count - visible); }
inline int ThumbWidth(int track, int count, int visible)
{
    return std::min(track, std::max(20, track * visible / std::max(visible, count)));
}
inline int OffsetAt(int cursor, int track, int thumb, int maximum)
{
    if (maximum <= 0 || track <= thumb) return 0;
    return std::clamp((cursor - thumb / 2) * maximum / (track - thumb), 0, maximum);
}
}
