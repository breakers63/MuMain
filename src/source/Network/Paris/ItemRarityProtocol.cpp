#include "stdafx.h"
#include "ItemRarityProtocol.h"
#include "GameLogic/Items/ItemRarityConfig.h"
#include "Network/Server/WSclient.h"

namespace Network::Paris::ItemRarity
{
namespace
{
constexpr DWORD RefreshMilliseconds = 300000;
DWORD lastRequest = 0, lastResponse = 0;
bool requested = false, received = false;
}

void Refresh()
{
    const DWORD now = GetTickCount();
    if (!SocketClient || (received && now - lastResponse < RefreshMilliseconds) ||
        (requested && now - lastRequest < RefreshMilliseconds)) return;
    BYTE packet[] = {0xC1, 5, 0xED, 0x66, 1};
    SocketClient->Send(packet, sizeof(packet));
    lastRequest = now;
    requested = true;
}

bool Receive(const unsigned char* packet, int size)
{
    constexpr int HeaderBytes = 6;
    if (!packet || size < HeaderBytes || size > GameLogic::Items::Rarity::MaximumConfigurationBytes + HeaderBytes ||
        packet[0] != 0xC2 || ((packet[1] << 8) | packet[2]) != size || packet[3] != 0xED || packet[4] != 0x67 || packet[5] != 1)
        return false;
    if (!GameLogic::Items::Rarity::Replace(reinterpret_cast<const char*>(packet + HeaderBytes), size - HeaderBytes)) return false;
    received = true;
    lastResponse = GetTickCount();
    return true;
}
}
