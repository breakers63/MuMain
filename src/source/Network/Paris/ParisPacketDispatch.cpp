#include "stdafx.h"
#include "Network/Paris/ParisPacketDispatch.h"
#include "UI/NewUI/Character/NewUICustomRanking.h"
#include "UI/NewUI/Inventory/NewUIMarketWindow.h"
#include "UI/NewUI/HUD/ParisBattlePassWindow.h"
#include "UI/NewUI/Character/NewUIAdvancedStatsWindow.h"
#include "UI/NewUI/Character/ParisVipWindow.h"
#include "UI/NewUI/Character/ParisResetWindow.h"
#include "UI/NewUI/Quests/NewUICustomQuestCenter.h"
#include "UI/NewUI/Character/ParisRankUser.h"
#include "Network/Paris/MentorProtocol.h"
#include "Network/Paris/SoulProtocol.h"
#include "Network/Paris/AuctionProtocol.h"
#include "Network/Paris/GmEventsProtocol.h"

namespace Network::Paris
{
void Dispatch(const std::uint8_t* packet, int size)
{
    constexpr int ShortHeadOffset = 2, LongHeadOffset = 3;
    constexpr BYTE ShortPacket = 0xC1, LongPacket = 0xC2;
    constexpr BYTE EncryptedShortPacket = 0xC3, EncryptedLongPacket = 0xC4;
    constexpr BYTE QuestAndStatsHead = 0xED, VipHead = 0xFA;
    if (packet == nullptr || size < LongHeadOffset + 1)
        return;
    const bool shortPacket = packet[0] == ShortPacket || packet[0] == EncryptedShortPacket;
    if (!shortPacket && packet[0] != LongPacket && packet[0] != EncryptedLongPacket)
        return;
    const BYTE head = packet[shortPacket ? ShortHeadOffset : LongHeadOffset];
    if (head == VipHead)
    {
        if (UI::Character::RankUser::Receive(packet, size)) return;
        if (Mentor::Receive(packet, size) || Soul::Receive(packet, size)
            || Auctions::Receive(packet, size) || GmEvents::Receive(packet, size)) return;
        if (g_pEtroMarketWindow) g_pEtroMarketWindow->ReceivePacket(packet, size);
        SEASON3B::CParisBattlePassWindow::ReceivePacket(packet, size);
        SEASON3B::CParisVipWindow::ReceivePacket(packet, size);
        SEASON3B::CParisResetWindow::ReceivePacket(packet, size);
        return;
    }
    if (head != QuestAndStatsHead)
        return;
    if (g_pCustomRankingWindow) g_pCustomRankingWindow->ReceivePacket(packet, size);
    if (g_pCustomQuestCenter) g_pCustomQuestCenter->ReceivePacket(packet, size);
    if (g_pAdvancedStatsWindow != nullptr)
        g_pAdvancedStatsWindow->ReceivePacket(packet, size);
}
} // namespace Network::Paris
