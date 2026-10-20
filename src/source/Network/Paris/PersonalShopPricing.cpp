#include "stdafx.h"
#include "Network/Paris/PersonalShopPricing.h"
#include "GameLogic/Items/StorePriceInput.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Inventory/NewUIMarketWindow.h"
#include "Network/Server/SocketSystem.h"

namespace Network::Paris::PersonalShopPricing
{
    bool CanUse(std::uint8_t currency)
    {
        const auto& currencies = GameLogic::Items::StorePrice::Currencies;
        if (std::find(currencies.begin(), currencies.end(), currency) == currencies.end()) return false;
        return currency == 0 || (g_pEtroMarketWindow && g_pEtroMarketWindow->CanUseExtendedMarket());
    }

    bool Send(int slot, int amount, std::uint8_t currency)
    {
        if (slot < 0 || slot > UINT8_MAX || amount <= 0 || !CanUse(currency)
            || !SocketClient || !SocketClient->IsConnected()) return false;
        if (g_pEtroMarketWindow && g_pEtroMarketWindow->CanUseExtendedMarket())
            g_pEtroMarketWindow->SendSetPriceRequest(static_cast<BYTE>(slot), amount, currency);
        else
            SocketClient->ToGameServer()->SendPlayerShopSetItemPrice(slot, amount);
        return true;
    }

    const wchar_t* CurrencyName(std::uint8_t currency)
    {
        constexpr const wchar_t* names[]{L"Zen", L"Paris Coins", L"Bless", L"Soul", L"Life", L"Creation",
            L"Guardian", L"Gemstone", L"Harmony", L"Chaos", L"LowerRefine", L"HigherRefine"};
        return currency < std::size(names) ? names[currency] : L"Desconocida";
    }
}
