#include "stdafx.h"
#include "Network/Paris/PartyFinderProtocol.h"
#include "Network/Server/WSclient.h"
#include "../../../ThirdParty/json.hpp"
#include <cstring>
#include <limits>
namespace Network::Paris::PartyFinder
{
Snapshot Data;
std::vector<Entry> Listings;
std::uint32_t Received = 0;
namespace { int sequence = 0, listRequest = 0; constexpr int RequestSize = 43, MaximumSnapshot = 60006; }
int Request(std::uint8_t operation, const std::string& identity)
{
    if (!SocketClient || !SocketClient->IsConnected() || operation > 15 || identity.size() > 32) return 0;
    sequence = sequence == std::numeric_limits<int>::max() ? 1 : sequence + 1;
    if (operation == 0) listRequest = sequence;
    BYTE packet[RequestSize] = {0xC1, RequestSize, 0xFA, 0xE0, 1, operation};
    std::memcpy(packet + 6, &sequence, sizeof(sequence));
    std::memcpy(packet + 10, identity.data(), identity.size());
    SocketClient->Send(packet, sizeof(packet));
    return sequence;
}
bool Receive(const std::uint8_t* packet, int size)
{
    if (!packet || size < 8 || size > MaximumSnapshot || packet[0] != 0xC2
        || ((packet[1] << 8) | packet[2]) != size || packet[3] != 0xFA || packet[4] != 0xE1 || packet[5] != 1) return false;
    try
    {
        const auto json = nlohmann::json::parse(packet + 6, packet + size);
        Snapshot next;
        next.request = json.at("request"); next.status = json.at("status"); next.isPublic = json.at("isPublic");
        next.followEnabled = json.value("followEnabled", true);
        next.autoAcceptParty = json.value("autoAcceptParty", false);
        next.showBuffs = json.value("showBuffs", true);
        next.blockSummon = json.value("blockSummon", false);
        if (json.contains("members") && !json["members"].is_null())
        {
            const auto& members = json["members"];
            if (!members.is_array() || members.size() > 16) return false;
            for (const auto& value : members)
            {
                MemberStatus member{value.at("name"), value.at("level"), value.at("resets"), value.at("health"),
                    value.at("maximumHealth"), value.at("mana"), value.at("maximumMana"), value.at("buffs").get<std::vector<int>>()};
                if (member.name.empty() || member.name.size() > 32 || member.level < 0 || member.resets < 0
                    || member.health < 0 || member.maximumHealth < 0 || member.mana < 0 || member.maximumMana < 0
                    || member.buffs.size() > 8) return false;
                for (int id : member.buffs) if (id < 1 || id > 160) return false;
                for (const auto& previous : next.members) if (previous.name == member.name) return false;
                next.members.push_back(std::move(member));
            }
        }
        const auto& parties = json.at("parties");
        if (next.request <= 0 || next.status < 0 || next.status > 6 || !parties.is_array() || parties.size() > 128) return false;
        for (const auto& row : parties)
        {
            Entry entry{row.at("id"), row.at("leader"), row.at("map"), row.at("members"), row.at("capacity")};
            if (entry.id.size() != 32 || entry.leader.empty() || entry.leader.size() > 32
                || entry.map < 0 || entry.map > 65535 || entry.members < 1 || entry.capacity < entry.members || entry.capacity > 255) return false;
            next.parties.push_back(std::move(entry));
        }
        if (!json.at("target").is_null())
        {
            const auto& target = json.at("target");
            next.hasTarget = true; next.targetName = target.at("name"); next.targetMap = target.at("map");
            next.targetX = target.at("x"); next.targetY = target.at("y");
            if (next.targetName.empty() || next.targetName.size() > 32 || next.targetMap < 0 || next.targetMap > 65535
                || next.targetX < 0 || next.targetX > 255 || next.targetY < 0 || next.targetY > 255) return false;
        }
        if (next.request == listRequest) Listings = next.parties;
        Data = std::move(next); Received = timeGetTime(); return true;
    }
    catch (const std::exception&) { return false; }
}
const MemberStatus* FindMember(const std::string& name)
{
    for (const auto& member : Data.members) if (member.name == name) return &member;
    return nullptr;
}
void Reset() { Data = {}; Listings.clear(); listRequest = 0; Received = 0; }
}
