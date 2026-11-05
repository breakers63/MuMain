#pragma once

#include <istream>
#include <sstream>
#include <string>
#include <vector>

namespace Data::MiniMap
{
inline constexpr int Npc = 1;
inline constexpr int Portal = 2;
inline constexpr int Spot = 3;
inline constexpr int Individual = 4;
inline constexpr int MarkerCapacity = 2048;
inline constexpr int MaximumNameBytes = 95;

struct MiniMapMarkerRecord
{
    int kind = 0;
    int x = 0;
    int y = 0;
    std::string name;
};

// Parse the entire export before replacing the legacy markers. A malformed file leaves
// the original map available, instead of publishing a partial marker list.
inline bool ReadMarkers(std::istream& input, std::vector<MiniMapMarkerRecord>& result)
{
    std::vector<MiniMapMarkerRecord> parsed;
    std::string line;
    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream fields(line);
        MiniMapMarkerRecord marker;
        if (!(fields >> marker.kind >> marker.x >> marker.y))
            return false;
        std::getline(fields >> std::ws, marker.name);
        if (!marker.name.empty() && marker.name.back() == '\r')
            marker.name.pop_back();
        if ((marker.kind != Npc && marker.kind != Spot && marker.kind != Individual)
            || marker.x < 0 || marker.x > 255 || marker.y < 0 || marker.y > 255
            || marker.name.empty() || marker.name.size() > MaximumNameBytes
            || parsed.size() >= MarkerCapacity)
            return false;
        parsed.push_back(std::move(marker));
    }
    if (input.bad())
        return false;
    result = std::move(parsed);
    return true;
}
}
