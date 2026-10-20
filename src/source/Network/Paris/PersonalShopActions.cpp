#include "stdafx.h"
#include "Network/Paris/PersonalShopActions.h"
#include "Network/Server/SocketSystem.h"
#include "UI/NewUI/Inventory/NewUIMyShopInventory.h"
#include "UI/NewUI/NewUISystem.h"

namespace Network::Paris::PersonalShopActions
{
    bool RequestOfflineStore()
    {
        if (!SocketClient || !SocketClient->IsConnected() || !Hero || !g_pMyShopInventory ||
            !g_pMyShopInventory->IsEnablePersonalShop())
            return false;
        constexpr wchar_t OfflineStoreCommand[] = L"/offstore";
        SocketClient->ToGameServer()->SendPublicChatMessage(MU_C16(Hero->ID), MU_C16(OfflineStoreCommand));
        return true;
    }
} // namespace Network::Paris::PersonalShopActions
