#include "Network/Paris/EventGuideProtocol.h"
#include <algorithm>
#include <climits>

namespace Network::Paris::EventGuide
{
namespace
{
bool Integer(const std::string& value, int& result, int minimum = 0, int maximum = INT_MAX)
{
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return !value.empty() && parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() && result >= minimum && result <= maximum;
}
bool Key(const std::string& value)
{
    return !value.empty() && value.size() <= 64 && std::all_of(value.begin(), value.end(), [](char c) { return c >= 'a' && c <= 'z' || c >= '0' && c <= '9' || c == '-'; });
}
bool Text(const std::string& value, std::size_t maximum = 192)
{
    return value.size() <= maximum && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == '<' || c == '>'; });
}
bool Phase(const std::string& value)
{
    return value == "idle" || value == "open" || value == "registration" || value == "prepare" || value == "running" || value == "disabled" || value == "available" || value == "unknown";
}
std::vector<std::string> Split(std::string_view text)
{
    std::vector<std::string> fields;
    while (true)
    {
        const auto position = text.find('|');
        fields.emplace_back(text.substr(0, position));
        if (position == std::string_view::npos) break;
        text.remove_prefix(position + 1);
    }
    return fields;
}
}

std::optional<Snapshot> Decoder::Receive(std::string_view line, std::uint32_t now)
{
    if (!line.starts_with("[PG]") || line.size() > 480) return {};
    auto fields = Split(line.substr(4));
    int sequence = 0;
    if (fields.size() < 2 || !Integer(fields[1], sequence, 1) || static_cast<std::uint32_t>(sequence) != m_sequence) return {};
    if (fields[0] == "B") { m_staging = Snapshot{}; m_staging->at = now; m_invalid = false; return {}; }
    if (!m_staging || m_invalid || now - m_staging->at > 10000) return {};
    if (fields[0] == "Z")
    {
        auto result = std::move(m_staging); m_staging.reset();
        if (result->events.empty()) return {};
        if (result->detail && std::none_of(result->events.begin(), result->events.end(), [&](const Event& entry) { return entry.key == result->detail->key; })) return {};
        result->at = now; return result;
    }
    m_invalid = !ReadRow(fields);
    return {};
}

bool Decoder::ReadRow(const std::vector<std::string>& f)
{
    if (f[0] == "D") return ReadDetail(f);
    if (f[0] == "L")
    {
        int seconds = 0;
        if (f.size() != 7 || !Key(f[2]) || !Text(f[3]) || !Phase(f[5]) || !Integer(f[6], seconds, -1) || m_staging->events.size() >= 128) return false;
        if (f[4] != "classic" && f[4] != "invasion" && f[4] != "boss" && f[4] != "bonus" && f[4] != "custom") return false;
        if (std::any_of(m_staging->events.begin(), m_staging->events.end(), [&](const Event& e) { return e.key == f[2]; })) return false;
        m_staging->events.push_back({f[2], f[3], f[4], f[5], seconds}); return true;
    }
    if (!m_staging->detail) return false;
    auto& detail = *m_staging->detail;
    int number = 0;
    if (f[0] == "R" && f.size() == 5 && Text(f[2], 80) && Text(f[3], 240) && Integer(f[4], number, -1, 1) && detail.requirements.size() < 32)
    { detail.requirements.push_back({f[2], f[3], number}); return true; }
    if (f[0] == "M" && f.size() == 5 && Integer(f[2], number, 0, 32767) && Text(f[3], 128) && (f[4] == "0" || f[4] == "1") && detail.monsters.size() < 64)
    { detail.monsters.push_back({number, f[3], f[4] == "1"}); return true; }
    if (f[0] == "W" && f.size() == 3 && Text(f[2], 240) && detail.rewards.size() < 32)
    { detail.rewards.push_back(f[2]); return true; }
    return false;
}

bool Decoder::ReadDetail(const std::vector<std::string>& f)
{
    Detail detail;
    if (f.size() != 9 || m_staging->detail || !Key(f[2]) || !Text(f[3]) || !Phase(f[4])
        || !Integer(f[5], detail.seconds, -1) || !Integer(f[6], detail.players, -1, 10000)
        || !Integer(f[7], detail.maximum, -1, 10000) || (f[8] != "0" && f[8] != "1")) return false;
    detail.key = f[2]; detail.name = f[3]; detail.phase = f[4]; detail.canJoin = f[8] == "1";
    m_staging->detail = std::move(detail); return true;
}
} // namespace Network::Paris::EventGuide
