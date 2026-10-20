#include "stdafx.h"
#include "Network/Paris/RankingInspectionProtocol.h"
#include "Network/Server/ExtendedItemData.h"
#include "Network/Server/WSclient.h"
#include <algorithm>
#include <cstring>
#include <span>

namespace Network::Paris::RankingInspection
{
namespace
{
constexpr std::uint8_t Head = 0xED;
constexpr int NameBytes = 10, MaximumMembers = 1000, MaximumMissions = 512;
int Read(const std::uint8_t* bytes) { int value; std::memcpy(&value, bytes, sizeof(value)); return value; }
std::wstring Text(const std::uint8_t* bytes, int capacity)
{
    int count = 0; while (count < capacity && bytes[count]) ++count;
    const int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(bytes), count, nullptr, 0);
    if (needed <= 0) return {};
    std::wstring result(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(bytes), count, result.data(), needed);
    return result;
}
bool EncodeName(std::uint8_t* out, const std::wstring& name)
{
    char utf8[41]{};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name.data(), static_cast<int>(name.size()), utf8, sizeof(utf8), nullptr, nullptr);
    if (size <= 0 || size > NameBytes) return false;
    std::memcpy(out, utf8, size); return true;
}
bool Send(const std::uint8_t* packet, int size)
{
    if (!SocketClient || !SocketClient->IsConnected()) return false;
    SocketClient->Send(packet, size); return true;
}
bool ReceiveProfile(const std::uint8_t* payload, int length, int tab)
{
    if (length < (tab == 1 ? 89 : 93) || payload[0] != 1 || payload[1] > 1 || tab != Data.tab) return false;
    const auto* base = payload + 2;
    const auto key = Text(base, NameBytes);
    if (key != Data.requestedName) return false;
    Profile profile;
    if (payload[1])
    {
        profile.name = tab == 1 ? Text(base + 10, NameBytes) : key;
        profile.guild = tab == 1 ? key : Text(base + 10, NameBytes);
        std::copy_n(base + 20, 32, profile.mark.begin());
        if (tab != 1)
        {
            profile.level = Read(base + (tab == 2 ? 60 : 52));
            profile.resets = tab == 2 ? 0 : Read(base + 56);
            profile.stats = tab == 2 ? 0 : Read(base + 60);
            if (profile.level < 0 || profile.resets < 0 || profile.stats < 0) return false;
            std::copy_n(base + 64, 27, profile.appearance.begin());
        }
        profile.available = true;
    }
    Data.profile = std::move(profile); ++Data.revision; ++Data.profileRevision;
    if (Data.profile.available)
    {
        if (tab == 1) RequestGuild(Data.profile.guild);
        else RequestEquipment(Data.profile.name);
    }
    return true;
}
bool ReceiveEquipment(const std::uint8_t* packet, int size)
{
    constexpr int PacketSize = 427, RecordSize = 34;
    if (size != PacketSize || packet[0] != 0xC2 || packet[5] != 1 || packet[6] > 1 || packet[7] > 1 || packet[18] > 1) return false;
    std::array<std::vector<std::uint8_t>, 12> equipment;
    for (std::size_t i = 0; i < equipment.size(); ++i)
    {
        const auto* record = packet + 19 + i * RecordSize;
        if (record[0] > 1 || (record[0] && (record[1] < 5 || record[1] > 32))) return false;
        if (!record[0]) continue;
        const auto bytes = std::span<const std::uint8_t>(record + 2, record[1]);
        if (!Network::Items::ExtendedItemLength(bytes)) return false;
        equipment[i].assign(bytes.begin(), bytes.end());
    }
    Data.ownPublic = packet[7] != 0; Data.preferenceReady = true;
    if (packet[18] && Text(packet + 8, NameBytes) == Data.equipmentName)
    {
        Data.publicEquipment = packet[6] != 0;
        Data.equipment = std::move(equipment); ++Data.equipmentRevision;
    }
    ++Data.revision; return true;
}
bool ReceiveGuild(const std::uint8_t* packet, int size)
{
    constexpr int Header = 18;
    if (size < Header || packet[0] != 0xC2 || (packet[5] != 1 && packet[5] != 2)) return false;
    const int count = packet[16] | (packet[17] << 8), recordSize = packet[5] == 2 ? 50 : 28;
    if (count > MaximumMembers || size != Header + count * recordSize || Text(packet + 6, NameBytes) != Data.guildName) return false;
    std::vector<Member> members; members.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        const auto* row = packet + Header + i * recordSize;
        Member member;
        member.name = Text(row, NameBytes); member.characterClass = row[10]; member.position = row[11];
        member.level = Read(row + 12); member.resets = Read(row + 16); member.stats = Read(row + 20); member.rank = Read(row + 24);
        if (member.name.empty() || member.level < 0 || member.resets < 0 || member.stats < 0 || member.rank < 0) return false;
        if (packet[5] == 2)
        {
            if (row[48] > 1 || row[49] > 1) return false;
            member.online = row[48] != 0; member.details = row[49] != 0;
            if (member.details) for (int a = 0; a < 5; ++a) { member.attributes[a] = Read(row + 28 + a * 4); if (member.attributes[a] < 0) return false; }
        }
        members.push_back(std::move(member));
    }
    Data.members = std::move(members); ++Data.revision; return true;
}
bool ReceiveMission(const std::uint8_t* packet, int size)
{
    if (size != 669 || packet[0] != 0xC2 || packet[5] != 2 || !Data.catalogLoading) return false;
    const auto* p = packet + 5;
    Mission mission;
    mission.id = Read(p + 1); mission.order = p[8] | (p[9] << 8);
    mission.current = Read(p + 14); mission.required = Read(p + 18); mission.monster = p[22] | (p[23] << 8);
    mission.name = Text(p + 29, 64); mission.objective = Text(p + 285, 64);
    if (mission.id < 0 || mission.current < 0 || mission.required <= 0 || mission.name.empty() || p[28] > 5) return false;
    if (p[28] > 0) { mission.reward = Read(p + 350); mission.rewardName = Text(p + 364, 48); }
    if (mission.reward < 0) return false;
    auto found = std::find_if(Data.missions.begin(), Data.missions.end(), [&](const auto& row) { return row.id == mission.id; });
    if (found != Data.missions.end()) *found = std::move(mission);
    else if (Data.missions.size() < MaximumMissions) Data.missions.push_back(std::move(mission));
    else return false;
    ++Data.revision; return true;
}
bool ReceiveMissionRanks(const std::uint8_t* packet, int size)
{
    if (size < 11 || packet[0] != 0xC2 || packet[5] != 1 || Read(packet + 6) != Data.missionId) return false;
    const int count = packet[10];
    if (count > 50 || size != 11 + count * 18) return false;
    std::vector<MissionRank> ranks; ranks.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        const auto* row = packet + 11 + i * 18;
        MissionRank rank{Text(row, NameBytes), Read(row + 10), Read(row + 14)};
        if (rank.name.empty() || rank.progress < 0 || rank.elapsed < 0) return false;
        ranks.push_back(std::move(rank));
    }
    Data.missionRanks = std::move(ranks); ++Data.revision; return true;
}
}
Snapshot Data;
bool RequestProfile(int tab, const std::wstring& name)
{
    if (tab < 0 || tab > 2) return false;
    constexpr std::uint8_t Subcodes[]{0x54, 0x56, 0x5A};
    std::uint8_t packet[15]{0xC1, 15, Head, Subcodes[tab], 1};
    if (!EncodeName(packet + 5, name) || !Send(packet, sizeof(packet))) return false;
    Data.tab = tab; Data.requestedName = name; Data.profile = {}; Data.members.clear();
    Data.equipment = {}; Data.publicEquipment = false; ++Data.revision; ++Data.equipmentRevision; return true;
}
bool RequestEquipment(const std::wstring& name, int mode, bool publicDetails)
{
    if (mode < 0 || mode > 2) return false;
    std::uint8_t packet[17]{0xC1, 17, Head, 0x5C, 1, static_cast<std::uint8_t>(mode), static_cast<std::uint8_t>(publicDetails)};
    if (mode == 0 && !EncodeName(packet + 7, name)) return false;
    if (!Send(packet, sizeof(packet))) return false;
    if (mode == 0) Data.equipmentName = name;
    else { Data.preferenceReady = false; ++Data.revision; }
    return true;
}
bool RequestGuild(const std::wstring& name)
{
    std::uint8_t packet[15]{0xC1, 15, Head, 0x5E, 2};
    if (!EncodeName(packet + 5, name) || !Send(packet, sizeof(packet))) return false;
    Data.guildName = name; return true;
}
bool RequestCatalog()
{
    constexpr std::uint8_t packet[]{0xC1, 5, Head, 0x00, 0};
    if (!Send(packet, sizeof(packet))) return false;
    Data.catalogLoading = true; return true;
}
bool RequestMission(int id)
{
    if (id < 0) return false;
    std::uint8_t packet[9]{0xC1, 9, Head, 0x60, 1};
    std::memcpy(packet + 5, &id, sizeof(id));
    if (!Send(packet, sizeof(packet))) return false;
    Data.missionId = id; Data.missionRanks.clear(); ++Data.revision; return true;
}
bool Receive(const std::uint8_t* packet, int size)
{
    if (!packet || size < 5) return false;
    const bool longPacket = packet[0] == 0xC2 || packet[0] == 0xC4;
    if (!longPacket && packet[0] != 0xC1 && packet[0] != 0xC3) return false;
    const int head = longPacket ? 3 : 2;
    const int declared = longPacket ? (packet[1] << 8) | packet[2] : packet[1];
    if (size != declared || size <= head + 1 || packet[head] != Head) return false;
    const int payload = head + 2;
    switch (packet[head + 1])
    {
    case 0x55: return ReceiveProfile(packet + payload, size - payload, 0);
    case 0x57: return ReceiveProfile(packet + payload, size - payload, 1);
    case 0x5B: return ReceiveProfile(packet + payload, size - payload, 2);
    case 0x5D: return ReceiveEquipment(packet, size);
    case 0x5F: return ReceiveGuild(packet, size);
    case 0x61: return ReceiveMissionRanks(packet, size);
    case 0x10:
        if (size < 8 || longPacket || packet[4] != 2 || (packet[5] | (packet[6] << 8)) > MaximumMissions) return false;
        Data.missions.clear(); Data.catalogLoading = true; ++Data.revision; return true;
    case 0x11: return ReceiveMission(packet, size);
    case 0x13:
        if (longPacket || size != 22 || packet[4] != 2) return false;
        for (auto& mission : Data.missions)
            if (mission.id == Read(packet + 5))
            {
                const int current = Read(packet + 10), required = Read(packet + 14);
                if (current < 0 || required <= 0) return false;
                mission.current = current; mission.required = required; ++Data.revision; return true;
            }
        return false;
    case 0x12:
        if (longPacket || size < 5 || packet[4] != 2) return false;
        std::stable_sort(Data.missions.begin(), Data.missions.end(), [](const auto& a, const auto& b) { return a.order == b.order ? a.id < b.id : a.order < b.order; });
        Data.catalogLoading = false; ++Data.revision; return true;
    default: return false;
    }
}
}
