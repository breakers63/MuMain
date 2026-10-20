#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/NewUI3DRenderMng.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class CUITextInputBox;
struct tagITEM;
typedef struct tagITEM ITEM;

namespace SEASON3B
{
    constexpr DWORD ETRO_MARKET_INTERFACE_KEY = 0x4D4B5457; // MKTW

    class CNewUIMarketWindow : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        enum
        {
            WINDOW_WIDTH = 400,
            WINDOW_HEIGHT = 260,
            PAGE_SIZE = 5,
        };

        CNewUIMarketWindow();
        ~CNewUIMarketWindow() override;

        bool Create(CNewUIManager* manager);
        void Release();
        void AnnounceCapability();
        void AnnounceCapability(bool force);
        void OpenWindow();
        void CloseWindow(bool playSound = true);
        bool IsWindowOpen() const;
        void ReceivePacket(const BYTE* packet, int size);
        bool CanUseExtendedMarket() const;
        BYTE SelectedPriceCurrency() const;
        void SetSelectedPriceCurrency(BYTE currency);
        void SendSetPriceRequest(BYTE slot, int amount, BYTE currency);

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool CloseForMenuHotKey();
        bool Update() override;
        bool Render() override;
        void Render3D() override;
        CNewUIObj* GetLayoutOwner() const override { return const_cast<CNewUIMarketWindow*>(this); }
        bool IsVisible() const override;
        float GetLayerDepth() override;
        float GetKeyEventOrder() override;

    private:
        enum class MarketView { Sellers, SellerItems, LegacyItems };

        struct Listing
        {
            std::array<BYTE, 16> ListingId{};
            std::uint32_t Revision{};
            std::array<BYTE, 16> ItemId{};
            std::wstring Seller;
            BYTE Slot{};
            BYTE Category{};
            BYTE Currency{};
            std::int32_t Amount{};
            std::vector<BYTE> ItemData;
        };

        struct Rect
        {
            int X;
            int Y;
            int Width;
            int Height;
        };

        void LoadImages();
        void UnloadImages();
        void ApplySearchBoxLayout();
        Rect LayoutRect(int x, int y, int width, int height) const;
        Rect DebugRect(const Rect& rect, int target) const;
        Rect ListingRowRect(int row) const;
        Rect SellerRowRect(int row) const;
        Rect RowBuyRect(int row) const;
        void SendOpenRequest();
        void SendSearchRequest();
        void SendSellerSearchRequest();
        void SendSellerItemsRequest();
        void SendActiveRequest();
        void EnterSeller(const std::wstring& seller);
        void ReturnToSellers();
        void SendBuyRequest(const Listing& listing);
        void ParseSearchResults(const BYTE* payload, int payloadSize);
        void ParseSellerSearchResults(const BYTE* payload, int payloadSize);
        void ParseSellerItemResults(const BYTE* payload, int payloadSize);
        void SetStatus(const wchar_t* value);
        void RenderButton(const Rect& rect, const wchar_t* text, bool active = false, int image = -1, bool enabled = true, int textTarget = -1) const;
        Rect FilterRect(int filter) const;
        Rect FilterOptionRect(int filter, int option) const;
        void RenderFilterMenu();
        void RenderSelectedPreview();
        void RenderHoveredItemTooltip();
        void ClearTooltipItem();
        bool IsMouseOver(const Rect& rect) const;
        bool CanGoPrevious() const;
        bool CanGoNext() const;
        bool CanBuy() const;
        static const wchar_t* CurrencyName(BYTE currency);

    private:
        CNewUIManager* m_Manager;
        CNewUI3DRenderMng* m_3DRenderManager;
        CUITextInputBox* m_SearchBox;
        POINT m_Position;
        bool m_WindowOpen;
        bool m_CapabilitySent;
        bool m_CapabilityAccepted;
        bool m_CurrencyPricingAccepted = false;
        bool m_WaitingForResults;
        bool m_SellerCapability = false;
        MarketView m_View = MarketView::LegacyItems;
        std::vector<std::wstring> m_Sellers;
        std::wstring m_SelectedSeller;
        std::wstring m_SavedSearch;
        int m_SavedPage = 0;
        BYTE m_SavedCategory = 0xFF;
        BYTE m_SavedCurrency = 0xFF;
        BYTE m_SavedSort = 0;
        std::uint32_t m_NextRequestId = 0;
        std::uint32_t m_PendingRequestId = 0;
        DWORD m_PendingTick = 0;
        MarketView m_PendingView = MarketView::LegacyItems;
        int m_Page;
        int m_TotalCount;
        BYTE m_Category;
        BYTE m_Currency;
        BYTE m_SelectedPriceCurrency = 0;
        BYTE m_Sort;
        int m_SelectedRow;
        ITEM* m_TooltipItem;
        int m_TooltipRow;
        int m_ConfirmRow;
        int m_OpenFilter = -1;
        bool m_PendingBuy = false;
        std::array<BYTE, 16> m_PendingBuyRequestId{};
        std::array<BYTE, 16> m_PendingBuyListingId{};
        BYTE m_PendingBuyCurrency = 0;
        int m_PendingBuyAmount = 0;
        DWORD m_LastRequestTick;
        std::wstring m_Status;
        std::vector<Listing> m_Listings;
    };
}

extern SEASON3B::CNewUIMarketWindow* g_pEtroMarketWindow;
