#include "stdafx.h"
#include "Network/Paris/ParisPacketDispatch.h"
#include "Network/Paris/EventTimerState.h"
#include "Network/Paris/AuctionProtocol.h"
#include "Network/Paris/GmEventsProtocol.h"
#include "Network/Paris/MentorProtocol.h"
#include "Network/Paris/ParisBattlePassProtocol.h"
#include "Network/Paris/ResetProtocol.h"
#include "Network/Paris/RankUserProtocol.h"
#include "Network/Paris/CustomRankingProtocol.h"
#include "Network/Paris/RankingInspectionProtocol.h"
#include "Network/Paris/SoulProtocol.h"
#include "Network/Paris/VipProtocol.h"
#include "Network/Paris/ItemRarityProtocol.h"

#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Network::Paris
{
namespace
{
struct Registration
{
    std::uint8_t head = 0;
    std::uint8_t subcode = 0;
    PacketHandler handler;
};

std::mutex g_handlerMutex;
std::unordered_map<std::uint64_t, Registration> g_handlers;
std::unordered_map<std::uint64_t, TextHandler> g_textHandlers;
std::uint64_t g_nextRegistration = 1;
} // namespace

std::uint64_t RegisterHandler(std::uint8_t head, std::uint8_t subcode, PacketHandler handler)
{
    if (!handler || (head != 0xED && head != 0xFA))
        return 0;

    std::lock_guard lock(g_handlerMutex);
    const std::uint64_t registration = g_nextRegistration++;
    g_handlers.emplace(registration, Registration{head, subcode, std::move(handler)});
    return registration;
}

void UnregisterHandler(std::uint64_t registration)
{
    if (registration == 0)
        return;

    std::lock_guard lock(g_handlerMutex);
    g_handlers.erase(registration);
}

std::uint64_t RegisterTextHandler(TextHandler handler)
{
    if (!handler)
        return 0;

    std::lock_guard lock(g_handlerMutex);
    const std::uint64_t registration = g_nextRegistration++;
    g_textHandlers.emplace(registration, std::move(handler));
    return registration;
}

void UnregisterTextHandler(std::uint64_t registration)
{
    if (registration == 0)
        return;

    std::lock_guard lock(g_handlerMutex);
    g_textHandlers.erase(registration);
}

bool DispatchText(std::wstring_view text)
{
    if (EventTimers.Receive(std::wstring(text).c_str(), timeGetTime()))
        return true;
    std::vector<TextHandler> handlers;
    {
        std::lock_guard lock(g_handlerMutex);
        handlers.reserve(g_textHandlers.size());
        for (const auto& [id, handler] : g_textHandlers)
        {
            (void)id;
            handlers.push_back(handler);
        }
    }

    for (const TextHandler& handler : handlers)
        if (handler(text))
            return true;
    return false;
}

void Dispatch(const std::uint8_t* packet, int size)
{
    if (packet == nullptr || size < 4)
        return;

    const bool shortPacket = packet[0] == 0xC1 || packet[0] == 0xC3;
    const bool longPacket = packet[0] == 0xC2 || packet[0] == 0xC4;
    if (!shortPacket && !longPacket)
        return;

    const int declaredSize = shortPacket ? packet[1] : (packet[1] << 8) | packet[2];
    const int headOffset = shortPacket ? 2 : 3;
    const int subcodeOffset = headOffset + 1;
    if (declaredSize != size || size <= subcodeOffset)
        return;

    const std::uint8_t head = packet[headOffset];
    if (head != 0xED && head != 0xFA)
        return;

    const std::uint8_t subcode = packet[subcodeOffset];
    if (head == 0xED && subcode == 0x67)
        (void)ItemRarity::Receive(packet, size);
    if (head == 0xFA)
    {
        // Keep the data snapshots current even when a custom panel is closed. RmlUi views can
        // then reopen without losing the response that arrived while the player was elsewhere.
        switch (subcode)
        {
        case 0xB1:
            if (auto snapshot = BattlePass::Decode(std::span<const BYTE>(packet, static_cast<std::size_t>(size))))
            {
                BattlePass::Data = std::move(*snapshot);
                BattlePass::Received = timeGetTime();
            }
            break;
        case 0xB3:
            (void)Vip::Receive(packet, size);
            break;
        case 0xC3:
            (void)RankUser::Receive(packet, size);
            break;
        case 0xD1:
            (void)GmEvents::Receive(packet, size);
            break;
        case 0xD3:
            (void)Mentor::Receive(packet, size);
            break;
        case 0xD6:
            (void)Auctions::Receive(packet, size);
            break;
        case 0xD8:
            (void)Soul::Receive(packet, size);
            break;
        case 0xC7:
            (void)Reset::Receive(packet, size);
            break;
        default:
            break;
        }
    }
    else if (head == 0xED && (subcode == 0x51 || subcode == 0x53 || subcode == 0x59))
    {
        (void)CustomRanking::Receive(packet, size);
    }
    if (head == 0xED) (void)RankingInspection::Receive(packet, size);
    std::vector<PacketHandler> handlers;
    {
        std::lock_guard lock(g_handlerMutex);
        for (const auto& [id, registration] : g_handlers)
        {
            (void)id;
            if (registration.head == head && registration.subcode == subcode)
                handlers.push_back(registration.handler);
        }
    }

    for (const PacketHandler& handler : handlers)
        handler(packet, size);
}
} // namespace Network::Paris
