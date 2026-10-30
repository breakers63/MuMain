#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <chrono>

namespace Network::Paris::BossDamage
{
struct Entry { std::string name; std::uint64_t damage = 0; };
struct Snapshot
{
    int entity = 0, monster = 0;
    std::uint64_t health = 0, maximum = 0;
    std::string name;
    std::vector<Entry> entries;
    std::chrono::steady_clock::time_point received;
};
inline std::unordered_map<int, Snapshot> snapshots;
inline bool Receive(const std::uint8_t* packet, int size)
{
    if (!packet || size < 29 || size > 4096 || packet[0] != 0xC2 || packet[3] != 0xFA
        || packet[4] != 0xEC || packet[5] != 1 || ((packet[1] << 8) | packet[2]) != size) return false;
    std::size_t cursor = 6;
    const auto number = [&](int count, std::uint64_t& result)
    {
        if (cursor + count > static_cast<std::size_t>(size)) return false;
        result = 0;
        for (int index = 0; index < count; ++index) result |= static_cast<std::uint64_t>(packet[cursor++]) << (8 * index);
        return true;
    };
    const auto text = [&](std::string& result, unsigned int limit)
    {
        if (cursor >= static_cast<std::size_t>(size)) return false;
        const unsigned int length = packet[cursor++];
        if (length > limit || cursor + length > static_cast<std::size_t>(size)) return false;
        result.assign(reinterpret_cast<const char*>(packet + cursor), length); cursor += length;
        return true;
    };
    Snapshot value; std::uint64_t entity, monster;
    if (!number(2, entity) || !number(2, monster) || !number(8, value.health) || !number(8, value.maximum)
        || value.maximum == 0 || value.health > value.maximum || !text(value.name, 96)
        || cursor >= static_cast<std::size_t>(size)) return false;
    const int count = packet[cursor++];
    if (count > 20) return false;
    for (int index = 0; index < count; ++index)
    {
        Entry entry;
        if (!text(entry.name, 40) || !number(8, entry.damage)) return false;
        value.entries.push_back(std::move(entry));
    }
    if (cursor != static_cast<std::size_t>(size)) return false;
    value.entity = static_cast<int>(entity); value.monster = static_cast<int>(monster);
    value.received = std::chrono::steady_clock::now();
    if (snapshots.size() >= 512) snapshots.clear();
    snapshots[value.entity] = std::move(value);
    return true;
}
inline const Snapshot* Find(int entity, int monster)
{
    const auto found = snapshots.find(entity);
    if (found == snapshots.end() || found->second.monster != monster
        || std::chrono::steady_clock::now() - found->second.received > std::chrono::seconds(15)) return nullptr;
    return &found->second;
}
inline bool IsBoss(int monster, int map)
{
    if ((monster >= 901 && monster <= 907) || (monster >= 950 && monster <= 964)) return true;
    static const auto rules = []
    {
        std::vector<std::pair<int,int>> result;
        std::ifstream file("Data/Custom/BossDamageRank.txt");
        for (std::string line; std::getline(file, line);)
        {
            line = line.substr(0,line.find("//"));
            std::istringstream stream(line); int number, show; std::string map;
            if (!(stream >> number >> show >> map)) continue;
            if (map == "*") result.emplace_back(number, -1);
            else { try { result.emplace_back(number, std::stoi(map)); } catch (...) {} }
        }
        return result;
    }();
    for (const auto& rule : rules) if (rule.first == monster && (rule.second == -1 || rule.second == map)) return true;
    return false;
}
inline std::string Escape(std::string_view text)
{
    std::string result;
    for (char character : text)
        switch(character) { case '&':result += "&amp;";break;case '<':result += "&lt;";break;case '>':result += "&gt;";break;default:result += character; }
    return result;
}
inline std::string Markup(const Snapshot& value)
{
    std::string text;
    for (std::size_t index=0; index<value.entries.size(); ++index)
    {
        if (index) text += "<br/>";
        text += std::to_string(index+1) + ". " + Escape(value.entries[index].name) + " — " + std::to_string(value.entries[index].damage);
    }
    return text;
}
}
