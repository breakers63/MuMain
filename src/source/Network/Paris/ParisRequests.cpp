#include "stdafx.h"
#include "Network/Paris/ParisRequests.h"
#include "Network/Paris/ParisBattlePassProtocol.h"
#include "Network/Server/WSclient.h"

#include <cstring>
#include <algorithm>

namespace Network::Paris
{
namespace
{
constexpr BYTE ShortPacket = 0xC1;
constexpr BYTE StatsHead = 0xED, StatsRequest = 0x20;
constexpr BYTE VipHead = 0xFA, VipRequest = 0xB2, VipVersion = 1;

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

bool RequestStatLimits(const wchar_t* character, std::uint32_t request)
{
    wchar_t command[48]{};
    swprintf_s(command, L"/parislimits %u", request);
    return RequestJewelBank(character, command);
}

bool RequestStatAllocation(const wchar_t* character, int stat, int amount)
{
    if (stat < 0 || stat >= 5 || amount <= 0 || amount > 65535) return false;
    wchar_t command[48]{};
    swprintf_s(command, L"/parisalloc %d %d", stat, amount);
    return RequestJewelBank(character, command);
}

bool RequestVip(int plan, bool purchase, std::uint64_t quote)
{
    if (!Connected() || plan < 1 || plan > 3 || (purchase && quote == 0))
        return false;

    constexpr int PacketSize = 16, QuoteOffset = 8;
    BYTE packet[PacketSize] = {ShortPacket, PacketSize, VipHead, VipRequest, VipVersion,
                               static_cast<BYTE>(purchase), static_cast<BYTE>(plan), 0};
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
    BYTE packet[PacketSize] = {ShortPacket, PacketSize, Head, Subcode, Version,
                               static_cast<BYTE>(reset)};
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

bool RequestBattlePass(std::uint8_t operation, int level, bool premium)
{
    const auto& data = BattlePass::Data;
    if (!Connected() || operation > 2 || level < 0 || level > 65535
        || (operation != 0 && data.ConfigurationToken == 0))
        return false;

    constexpr BYTE PacketSize = 33;
    BYTE packet[PacketSize] = {ShortPacket, PacketSize, 0xFA, 0xB0, 1, operation};
    std::copy(data.Season.begin(), data.Season.end(), packet + 6);
    packet[22] = static_cast<BYTE>(level & 0xFF);
    packet[23] = static_cast<BYTE>((level >> 8) & 0xFF);
    packet[24] = static_cast<BYTE>(premium);
    std::memcpy(packet + 25, &data.ConfigurationToken, sizeof(data.ConfigurationToken));
    SocketClient->Send(packet, sizeof(packet));
    return true;
}
} // namespace Network::Paris
