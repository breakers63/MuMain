#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Network::Paris::EventGuide
{
struct Event
{
    std::string key, name, category, phase;
    int seconds = -1;
};
struct Requirement { std::string label, value; int state = -1; };
struct Monster { int id = 0; std::string name; bool boss = false; };
struct Detail : Event
{
    int players = -1, maximum = -1;
    bool canJoin = false;
    std::vector<Requirement> requirements;
    std::vector<Monster> monsters;
    std::vector<std::string> rewards;
};
struct Snapshot
{
    std::vector<Event> events;
    std::optional<Detail> detail;
    std::uint32_t at = 0;
};

class Decoder
{
public:
    void Expect(std::uint32_t sequence) { m_sequence = sequence; m_staging.reset(); m_invalid = false; }
    std::optional<Snapshot> Receive(std::string_view line, std::uint32_t now);
private:
    bool ReadRow(const std::vector<std::string>& fields);
    bool ReadDetail(const std::vector<std::string>& fields);
    std::uint32_t m_sequence = 0;
    bool m_invalid = false;
    std::optional<Snapshot> m_staging;
};
} // namespace Network::Paris::EventGuide
