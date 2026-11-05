#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <istream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace World::Monsters::Custom
{
    using GoldenColor = std::array<float, 3>;

    // The classic encoder stores golden RGB colours in section 1. Parsing is
    // a startup operation; rendering only reads the cached result.
    inline std::unordered_map<int, GoldenColor> ReadGoldenColors(std::istream& input)
    {
        std::unordered_map<int, GoldenColor> colors;
        std::string line;
        int section = -1;
        while (std::getline(input, line))
        {
            line = line.substr(0, line.find("//"));
            std::istringstream row(line);
            std::string first;
            if (!(row >> first)) continue;
            if (first == "end") { section = -1; continue; }
            if (section < 0)
            {
                if ((first == "0" || first == "1" || first == "2") && (row >> std::ws).eof())
                    section = first[0] - '0';
                continue;
            }
            if (section != 1) continue;
            row.clear();
            row.str(line);
            int id = -1;
            GoldenColor color{};
            if (!(row >> id >> color[0] >> color[1] >> color[2]) || !(row >> std::ws).eof()
                || id < 0 || id > std::numeric_limits<std::int16_t>::max())
                throw std::invalid_argument("Invalid custom monster golden colour row.");
            for (float channel : color)
                if (!std::isfinite(channel) || channel < 0.0f || channel > 1.0f)
                    throw std::invalid_argument("Custom monster RGB values must be between zero and one.");
            colors.insert_or_assign(id, color);
        }
        return colors;
    }
}
