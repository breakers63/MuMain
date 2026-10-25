#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace Network::Paris::PartyFinder
{
constexpr int PageSize = 4;
struct Entry { std::string id, leader; int map = 0, members = 0, capacity = 0; };
struct MemberStatus { std::string name; int level = 0, resets = 0, health = 0, maximumHealth = 0, mana = 0, maximumMana = 0; std::vector<int> buffs; };
struct Snapshot
{
    int request = 0, status = 0;
    bool isPublic = false, hasTarget = false, followEnabled = true, autoAcceptParty = false;
    bool showBuffs = true, blockSummon = false;
    std::vector<MemberStatus> members;
    std::string targetName;
    int targetMap = 0, targetX = 0, targetY = 0;
    std::vector<Entry> parties;
};
extern Snapshot Data;
extern std::vector<Entry> Listings;
extern std::uint32_t Received;
bool Receive(const std::uint8_t* packet, int size);
int Request(std::uint8_t operation, const std::string& identity = {});
const MemberStatus* FindMember(const std::string& name);
void Reset();
}
