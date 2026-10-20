#include "stdafx.h"
#include "Network/Paris/ParisRequests.h"
#include "Network/Server/SocketSystem.h"
#include "Network/Server/WSclient.h"
#include <cstring>

namespace Network::Paris
{
namespace
{
constexpr BYTE ShortPacket = 0xC1;
constexpr BYTE StatsHead = 0xED, StatsRequest = 0x20;
constexpr BYTE VipHead = 0xFA, VipRequest = 0xB2, VipVersion = 1;
constexpr int VipPacketSize = 16, QuoteOffset = 8;

bool Connected()
{
    return SocketClient != nullptr && SocketClient->IsConnected();
}
} // namespace

bool RequestJewelBank(const wchar_t* character, const wchar_t* command)
{
    if (!Connected() || character == nullptr || command == nullptr)
        return false;
    SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(character), MU_C16(command));
    return true;
}

bool RequestVip(int plan, bool purchase, std::uint64_t quote)
{
    constexpr int FirstPlan = 1, LastPlan = 3;
    if (!Connected() || plan < FirstPlan || plan > LastPlan || (purchase && quote == 0))
        return false;
    BYTE packet[VipPacketSize] = {ShortPacket,
                                  VipPacketSize,
                                  VipHead,
                                  VipRequest,
                                  VipVersion,
                                  static_cast<BYTE>(purchase),
                                  static_cast<BYTE>(plan),
                                  0};
    std::memcpy(packet + QuoteOffset, &quote, sizeof(quote));
    SocketClient->Send(packet, sizeof(packet));
    return true;
}

bool RequestReset(bool reset, std::uint64_t token)
{
    if (!Connected() || (reset && token == 0))
        return false;
    constexpr BYTE PacketSize = 14, Head = 0xFA, Subcode = 0xC6, Version = 1;
    constexpr int TokenOffset = 6;
    BYTE packet[PacketSize] = {ShortPacket, PacketSize, Head, Subcode, Version, static_cast<BYTE>(reset)};
    std::memcpy(packet + TokenOffset, &token, sizeof(token));
    SocketClient->Send(packet, sizeof(packet));
    return true;
}

bool RequestAdvancedStats()
{
    if (!Connected())
        return false;
    constexpr BYTE PacketSize = 4;
    BYTE packet[PacketSize] = {ShortPacket, PacketSize, StatsHead, StatsRequest};
    SocketClient->Send(packet, sizeof(packet));
    return true;
}
} // namespace Network::Paris
