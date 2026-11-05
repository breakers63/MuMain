#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace Network::Paris::RankingInspection
{
struct Profile
{
    std::wstring name, guild;
    int level = 0, resets = 0, stats = 0;
    std::array<std::uint8_t, 27> appearance{};
    std::array<std::uint8_t, 32> mark{};
    bool available = false;
};
struct Member
{
    std::wstring name;
    int characterClass = 0, position = 0, level = 0, resets = 0, stats = 0, rank = 0;
    std::array<int, 5> attributes{};
    bool online = false, details = false;
};
struct Mission
{
    int id = 0, order = 0, monster = 0, current = 0, required = 0, reward = 0;
    std::wstring name, objective, rewardName;
};
struct MissionRank { std::wstring name; int progress = 0, elapsed = 0; };
struct Snapshot
{
    Profile profile;
    std::wstring requestedName, equipmentName, guildName;
    std::array<std::vector<std::uint8_t>, 12> equipment;
    std::vector<Member> members;
    std::vector<Mission> missions;
    std::vector<MissionRank> missionRanks;
    int tab = 0, missionId = -1;
    bool publicEquipment = false, ownPublic = false, preferenceReady = false, catalogLoading = false;
    std::uint64_t revision = 0, profileRevision = 0, equipmentRevision = 0;
};
extern Snapshot Data;
bool RequestProfile(int tab, const std::wstring& name);
bool RequestEquipment(const std::wstring& name, int mode = 0, bool publicDetails = false);
bool RequestGuild(const std::wstring& name);
bool RequestCatalog();
bool RequestMission(int id);
bool Receive(const std::uint8_t* packet, int size);
}
