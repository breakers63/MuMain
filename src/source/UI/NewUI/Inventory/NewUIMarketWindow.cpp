#include "stdafx.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Dialogs/ParisCustomMenuWindow.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"
#include "UI/NewUI/Character/ParisMarketAssets.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Inventory/NewUIMarketWindow.h"


#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInventory.h"
#include "Network/Server/SocketSystem.h"
#include "Network/Server/WSclient.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Textures/ZzzTexture.h"
#include "Scenes/SceneCore.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "UI/NewUI/NewUISystem.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>

using namespace SEASON3B;

SEASON3B::CNewUIMarketWindow* g_pEtroMarketWindow = nullptr;

namespace
{
    enum : int { BITMAP_ETRO_MARKET_PANEL = 32800, BITMAP_ETRO_MARKET_ROW = 32801, BITMAP_ETRO_MARKET_CLOSE = 32802, BITMAP_ETRO_MARKET_BUTTON = 32803, BITMAP_ETRO_MARKET_BUTTON_ACTION = 32804, BITMAP_ETRO_MARKET_BUTTON_PAGE = 32805, BITMAP_ETRO_MARKET_INPUT = 32806, BITMAP_ETRO_MARKET_DROPDOWN = 32807, BITMAP_ETRO_MARKET_ITEM_SLOT = 32808 };
    HFONT g_MarketFont=nullptr, g_MarketTitleFont=nullptr;
    GLuint g_ParisMarketBuy = BITMAP_UNKNOWN;
    constexpr BYTE HeadCode = 0xFA;
    constexpr BYTE Version = 1;
    constexpr BYTE Capability = 0xE2;
    constexpr BYTE Open = 0xE3;
    constexpr BYTE Search = 0xE4;
    constexpr BYTE SearchResults = 0xE5;
    constexpr BYTE Buy = 0xE6;
    constexpr BYTE BuyResult = 0xE7;
    constexpr BYTE SetPrice = 0xE8;
    constexpr BYTE SetPriceResult = 0xE9;
    constexpr BYTE Balances = 0xEA;
    constexpr BYTE Error = 0xEB;
    constexpr BYTE SellerSearch = 0xEC;
    constexpr BYTE SellerSearchResults = 0xED;
    constexpr BYTE SellerItems = 0xEE;
    constexpr BYTE SellerItemResults = 0xEF;
    constexpr BYTE AnyFilter = 0xFF;
    constexpr const wchar_t* MarketCategories[] = {
        L"Todas", L"Espadas", L"Hachas", L"Mazas / cetros", L"Lanzas",
        L"Arcos / ballestas", L"Bastones", L"Escudos", L"Cascos",
        L"Armaduras", L"Pantalones", L"Guantes", L"Botas",
        L"Alas / orbes", L"Accesorios", L"Joyas / consumibles", L"Scrolls / skills"
    };

    UI::Theme::ParisFrameAssets marketArt;
    UI::Theme::ParisMarketAssets referenceArt;
    constexpr DWORD RequestCooldown = 200;
    constexpr int MarketDesignWidth = 800;
    constexpr int MarketDesignHeight = 548;

    // ETRO MARKET DUAL BACKGROUND V1
    // Layout design space remains 800x520; only source texture resolution changes.
    int g_MarketPanelSourceWidth = 800;
    int g_MarketPanelSourceHeight = 520;
    bool g_MarketPanelUsingHd = false;
    bool g_MarketPanelSelectionInitialized = false;
    bool g_MarketPanelLastPreferHd = false;

    // ETRO MARKET FINAL FROZEN LAYOUT.
    // Final calibration integrated on 2026-09-26; runtime debug is disabled.
    // ETRO MARKET LAYOUT V2 ----------------------------------------------------
    // 800x520 remains the DESIGN space of the artwork/layout.
    // The NewUI client still uses its virtual 640x480 coordinate system
    // internally. Physical 640x480 can be removed from the game options later
    // without changing this virtual coordinate system.
    constexpr float g_MarketLayoutScale = 0.70f;
    constexpr int g_MarketOffsetX = 0;
    constexpr int g_MarketOffsetY = 0;


    enum MarketDebugGroup
    {
        DebugGroupWindow = 0,
        DebugGroupHeader,
        DebugGroupFilters,
        DebugGroupResults,
        DebugGroupPreview,
        DebugGroupFooter,
        DebugGroupCount,
    };

    // Stable layout target IDs retained to apply the final per-element calibration.
    enum MarketDebugTarget
    {
        DebugBackground = 0,

        DebugTopClose,
        DebugTitle,
        DebugBackButton,
        DebugTopCloseText,
        DebugBackText,

        DebugSearchInput,
        DebugCategory,
        DebugCurrency,
        DebugSort,
        DebugSearchButton,
        DebugRefreshButton,
        DebugSearchLabel,
        DebugCategoryText,
        DebugCurrencyText,
        DebugSortText,
        DebugSearchButtonText,
        DebugRefreshButtonText,

        DebugRowBuy,
        DebugSellerHeader,
        DebugSellerRow,
        DebugSellerName,
        DebugItemHeader,
        DebugItemSellerHeader,
        DebugPriceHeader,
        DebugCurrencyHeader,
        DebugItemRow,
        DebugItemName,
        DebugItemSeller,
        DebugItemPrice,
        DebugItemCurrency,
        DebugRowBuyText,
        DebugListEmpty,
        DebugSellerEmpty,

        DebugPreviewBuy,
        DebugPreview3D,
        DebugPreviewTitle,
        DebugPreviewEmpty,
        DebugPreviewSlot,
        DebugPreviewName,
        DebugPreviewFieldLabel,
        DebugPreviewFieldValue,
        DebugPreviewFieldLast,
        DebugPreviewBuyText,
        DebugTooltip,

        DebugPrevious,
        DebugNext,
        DebugBottomClose,
        DebugPageText,
        DebugTotalText,
        DebugStatusText,
        DebugPreviousText,
        DebugNextText,
        DebugBottomCloseText,

        DebugTargetCount,
    };

    int MarketTextureAllocationSize(int imageSize)
    {
        if (imageSize <= 0)
        {
            return 1;
        }

        int allocationSize = 1;
        while (allocationSize < imageSize)
        {
            allocationSize *= 2;
        }

        return allocationSize;
    }

    float MarketTextureUvStart(int imageSize)
    {
        const int allocationSize = MarketTextureAllocationSize(imageSize);
        return 0.5f / static_cast<float>(allocationSize);
    }

    float MarketTextureUvSpan(int imageSize)
    {
        const int allocationSize = MarketTextureAllocationSize(imageSize);
        if (imageSize <= 1)
        {
            return 0.0f;
        }

        // Stay on valid texel centers. Ending exactly at source/allocation
        // makes GL_LINEAR mix the last valid source pixel with padded memory.
        return static_cast<float>(imageSize - 1) / static_cast<float>(allocationSize);
    }

    bool EnsureMarketPanelTextureForCurrentResolution(bool forceReload)
    {
        // WindowWidth/WindowHeight are updated by WM_SIZE after the real game
        // window reaches its physical resolution. LoadImages() can run earlier,
        // so selection must also be checked at runtime.
        const bool preferHdPanel = (WindowWidth >= 1280);

        if (!forceReload
            && g_MarketPanelSelectionInitialized
            && g_MarketPanelLastPreferHd == preferHdPanel)
        {
            return true;
        }

        if (g_MarketPanelSelectionInitialized)
        {
            DeleteBitmap(BITMAP_ETRO_MARKET_PANEL);
        }

        g_MarketPanelSelectionInitialized = true;
        g_MarketPanelLastPreferHd = preferHdPanel;
        g_MarketPanelSourceWidth = 800;
        g_MarketPanelSourceHeight = 520;
        g_MarketPanelUsingHd = false;

        auto tryLoadPanel = [&](const wchar_t* path, int sourceWidth, int sourceHeight) -> bool
        {
            if (LoadBitmap(path, BITMAP_ETRO_MARKET_PANEL, GL_LINEAR, GL_CLAMP_TO_EDGE, false))
            {
                g_MarketPanelSourceWidth = sourceWidth;
                g_MarketPanelSourceHeight = sourceHeight;
                g_MarketPanelUsingHd = (sourceWidth == 1600 && sourceHeight == 1040);
                return true;
            }

            return false;
        };

        if (preferHdPanel)
        {
            if (tryLoadPanel(L"Interface\\EtroMarketPanelHD.tga", 1600, 1040))
                return true;

            return tryLoadPanel(L"Interface\\EtroMarketPanel.tga", 800, 520);
        }

        if (tryLoadPanel(L"Interface\\EtroMarketPanel.tga", 800, 520))
            return true;

        return tryLoadPanel(L"Interface\\EtroMarketPanelHD.tga", 1600, 1040);
    }

    void RenderMarketImage(GLuint image, float x, float y, float width, float height)
    {
        // Bitmaps[image].Width/Height report the padded OpenGL allocation in
        // this client. Keep the original TGA dimensions measured from the
        // shipped EtroMarket assets, so their empty padding is never sampled.
        int sourceWidth = 0;
        int sourceHeight = 0;
        switch (image)
        {
        case BITMAP_ETRO_MARKET_PANEL:         sourceWidth = g_MarketPanelSourceWidth; sourceHeight = g_MarketPanelSourceHeight; break;
        case BITMAP_ETRO_MARKET_ROW:           sourceWidth = 560; sourceHeight = 44; break;
        case BITMAP_ETRO_MARKET_CLOSE:         sourceWidth = 26; sourceHeight = 26; break;
        case BITMAP_ETRO_MARKET_BUTTON:        sourceWidth = 90; sourceHeight = 26; break;
        case BITMAP_ETRO_MARKET_BUTTON_ACTION: sourceWidth = 82; sourceHeight = 26; break;
        case BITMAP_ETRO_MARKET_BUTTON_PAGE:   sourceWidth = 80; sourceHeight = 26; break;
        case BITMAP_ETRO_MARKET_INPUT:         sourceWidth = 200; sourceHeight = 24; break;
        case BITMAP_ETRO_MARKET_DROPDOWN:      sourceWidth = 120; sourceHeight = 24; break;
        case BITMAP_ETRO_MARKET_ITEM_SLOT:     sourceWidth = 42; sourceHeight = 42; break;
        default: break;
        }

        const float u = MarketTextureUvStart(sourceWidth);
        const float v = MarketTextureUvStart(sourceHeight);
        const float uw = MarketTextureUvSpan(sourceWidth);
        const float vh = MarketTextureUvSpan(sourceHeight);

        if (image == BITMAP_ETRO_MARKET_PANEL)
        {
            // ETRO_MARKET_ALPHA_CUTOUT_V1
            // Same proven path used by transparent HUD panels:
            // alpha==0 exterior is discarded instead of becoming a rectangle.
            DisableAlphaBlend();
            EnableAlphaTest();
            mu::GetRenderer().SetAlphaFunc(GL_GREATER, 0.01f);
            UI::Paris::Color(1.0f, 1.0f, 1.0f, 1.0f);

            RenderBitmap(image, x, y, width, height, u, v, uw, vh);

            mu::GetRenderer().SetAlphaFunc(GL_GREATER, 0.0f);
            EnableAlphaTest();
            return;
        }

        RenderImage(
            image,
            x,
            y,
            width,
            height,
            u,
            v,
            uw,
            vh);
    }

    void RenderMarketInset(int x,int y,int width,int height)
    {
        constexpr int edge=5;
        const int sourceX[]={0,5,77,82},sourceY[]={0,5,21,26};
        const int targetX[]={x,x+edge,x+width-edge,x+width},targetY[]={y,y+edge,y+height-edge,y+height};
        UI::Character::ParisStyle::Solid(x,y,width,height,.025f,.025f,.025f);
        EnableAlphaTest();UI::Paris::Color(1,1,1,1);
        for(int row=0;row<3;++row)for(int col=0;col<3;++col)
        {
            if(row==1&&col==1)continue;
            RenderBitmap(BITMAP_ETRO_MARKET_BUTTON_ACTION,targetX[col],targetY[row],targetX[col+1]-targetX[col],targetY[row+1]-targetY[row],
                (sourceX[col]+.5f)/128.f,(sourceY[row]+.5f)/32.f,(sourceX[col+1]-sourceX[col]-1.f)/128.f,(sourceY[row+1]-sourceY[row]-1.f)/32.f);
        }
    }

    void RenderPublicationBackground(int x,int y,int width,int height,int row)
    {
        const float shade=row%2?.055f:.035f;
        UI::Character::ParisStyle::Solid(x,y,width,height,shade,shade,shade);
        UI::Character::ParisStyle::Solid(x,y+height-1,width,1,.18f,.16f,.12f);
    }

    int ScaleMarketUi(int value)
    {
        return static_cast<int>((static_cast<float>(value) * g_MarketLayoutScale) + 0.5f);
    }

    void RenderMarketTitle(int x, int y, int width)
    {
        constexpr int titleTop=20;
        g_pRenderText->SetBgColor(0,0,0,0);
        g_pRenderText->SetFont(g_MarketTitleFont?g_MarketTitleFont:g_hFontBold);
        g_pRenderText->SetTextColor(244,211,126,255);
        g_pRenderText->RenderText(x,y+ScaleMarketUi(titleTop),L"MU PARIS - MARKET",width,0,RT3_SORT_CENTER);
        g_pRenderText->SetFont(g_hFont);
    }

    int MarketPanelWidth()
    {
        return ScaleMarketUi(MarketDesignWidth);
    }

    int MarketPanelHeight()
    {
        return ScaleMarketUi(MarketDesignHeight);
    }

    void UpdateMarketPanelPosition(POINT& position)
    {
        position.x = ((640 - MarketPanelWidth()) / 2) + g_MarketOffsetX;
        position.y = ((480 - MarketPanelHeight()) / 2) + g_MarketOffsetY;
    }


    std::uint16_t ReadUInt16(const BYTE* source)
    {
        return static_cast<std::uint16_t>(source[0])
            | (static_cast<std::uint16_t>(source[1]) << 8);
    }

    std::uint32_t ReadUInt32(const BYTE* source)
    {
        return static_cast<std::uint32_t>(source[0])
            | (static_cast<std::uint32_t>(source[1]) << 8)
            | (static_cast<std::uint32_t>(source[2]) << 16)
            | (static_cast<std::uint32_t>(source[3]) << 24);
    }

    std::int32_t ReadInt32(const BYTE* source)
    {
        return static_cast<std::int32_t>(ReadUInt32(source));
    }

    void WriteUInt16(BYTE* target, std::uint16_t value)
    {
        target[0] = static_cast<BYTE>(value);
        target[1] = static_cast<BYTE>(value >> 8);
    }

    void WriteUInt32(BYTE* target, std::uint32_t value)
    {
        target[0] = static_cast<BYTE>(value);
        target[1] = static_cast<BYTE>(value >> 8);
        target[2] = static_cast<BYTE>(value >> 16);
        target[3] = static_cast<BYTE>(value >> 24);
    }

    std::vector<BYTE> ToUtf8(const wchar_t* value)
    {
        if (value == nullptr || value[0] == L'\0')
        {
            return {};
        }

        const int required = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
        if (required <= 1)
        {
            return {};
        }

        std::vector<BYTE> result(static_cast<std::size_t>(required));
        WideCharToMultiByte(CP_UTF8, 0, value, -1, reinterpret_cast<char*>(result.data()), required, nullptr, nullptr);
        result.pop_back();
        if (result.size() > 64)
        {
            result.resize(64);
        }

        return result;
    }

    std::wstring FromFixedUtf8(const BYTE* value, int length)
    {
        int actual = 0;
        while (actual < length && value[actual] != 0)
        {
            ++actual;
        }

        if (actual == 0)
        {
            return {};
        }

        const int required = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(value), actual, nullptr, 0);
        std::wstring result(static_cast<std::size_t>(std::max(required, 0)), L'\0');
        if (required > 0)
        {
            MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(value), actual, result.data(), required);
        }

        return result;
    }

    std::array<BYTE, 16> CreateRequestId()
    {
        static std::mt19937_64 generator(
            static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
            ^ static_cast<std::uint64_t>(GetTickCount64())
            ^ static_cast<std::uint64_t>(GetCurrentProcessId()));
        std::array<BYTE, 16> result{};
        const auto first = generator();
        const auto second = generator();
        std::memcpy(result.data(), &first, sizeof(first));
        std::memcpy(result.data() + sizeof(first), &second, sizeof(second));
        result[6] = static_cast<BYTE>((result[6] & 0x0F) | 0x40);
        result[8] = static_cast<BYTE>((result[8] & 0x3F) | 0x80);
        return result;
    }

    void DrawPanel(float x, float y, float width, float height, float red, float green, float blue, float alpha)
    {
        EnableAlphaTest();
        UI::Paris::Color(red, green, blue, alpha);
        RenderColor(x, y, width, height);
        EndRenderColor();
        UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
    }

}

CNewUIMarketWindow::CNewUIMarketWindow()
    : m_Manager(nullptr),
      m_3DRenderManager(nullptr),
      m_SearchBox(nullptr),
      m_Position{ ((640 - ScaleMarketUi(MarketDesignWidth)) / 2) + g_MarketOffsetX, ((480 - ScaleMarketUi(MarketDesignHeight)) / 2) + g_MarketOffsetY },
      m_WindowOpen(false),
      m_CapabilitySent(false),
      m_CapabilityAccepted(false),
      m_WaitingForResults(false),
      m_Page(0),
      m_TotalCount(0),
      m_Category(AnyFilter),
      m_Currency(AnyFilter),
      m_Sort(0),
      m_SelectedRow(-1),
      m_TooltipItem(nullptr),
      m_TooltipRow(-1),
      m_ConfirmRow(-1),
      m_LastRequestTick(0),
      m_Status(L"Mercado listo")
{
}

CNewUIMarketWindow::~CNewUIMarketWindow()
{
    Release();
}

bool CNewUIMarketWindow::Create(CNewUIManager* manager)
{
    if (manager == nullptr || g_pNewUI3DRenderMng == nullptr)
    {
        return false;
    }

    m_Manager = manager;
    m_Manager->AddUIObj(ETRO_MARKET_INTERFACE_KEY, this);

    // ETRO MARKET 3D RENDER FIX V1.5.3
    // RenderItem3D requires the dedicated NewUI 3D camera/projection pass.
    // The Market itself renders at layer 7.0, so use 7.1 to place the item
    // models above the panel instead of behind it.
    m_3DRenderManager = g_pNewUI3DRenderMng;
    m_3DRenderManager->Add3DRenderObj(this, 7.1f);

        LoadImages();

    UpdateMarketPanelPosition(m_Position);

    m_SearchBox = new CUITextInputBox;
    m_SearchBox->Init(g_hWnd, 142, 16, 64);
    ApplySearchBoxLayout();
    m_SearchBox->SetTextColor(245, 235, 205, 255);
    m_SearchBox->SetBackColor(0, 0, 0, 0);
    m_SearchBox->SetFont(g_hFont);
    m_SearchBox->SetState(UISTATE_HIDE);
    SetRelatedWnd(m_SearchBox->GetHandle());
    Show(true);
    Enable(true);
    g_pEtroMarketWindow = this;
    return true;
}

void CNewUIMarketWindow::Release()
{
    if (g_pEtroMarketWindow == this)
    {
        g_pEtroMarketWindow = nullptr;
    }

    ClearTooltipItem();
    SAFE_DELETE(m_SearchBox);
    UnloadImages();

    if (m_3DRenderManager != nullptr)
    {
        m_3DRenderManager->Remove3DRenderObj(this);
        m_3DRenderManager = nullptr;
    }

    if (m_Manager != nullptr)
    {
        m_Manager->RemoveUIObj(this);
        m_Manager = nullptr;
    }
}


void CNewUIMarketWindow::LoadImages()
{
    marketArt.Load(); referenceArt.Load();
    g_ParisMarketBuy = Bitmaps.LoadImage(L"Data\\Interface\\ParisMarketBuy.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    g_MarketFont = g_hFont;
    g_MarketTitleFont = g_hFontBold;
    // Initial load. The selected background will be revalidated in Render()
    // after WM_SIZE has supplied the real physical resolution.
    EnsureMarketPanelTextureForCurrentResolution(true);

    LoadBitmap(L"Interface\\EtroMarketRow.tga", BITMAP_ETRO_MARKET_ROW, GL_LINEAR);
    LoadBitmap(L"Interface\\EtroMarketClose.tga", BITMAP_ETRO_MARKET_CLOSE, GL_LINEAR);
    LoadBitmap(L"Interface\\EtroMarketButton.tga", BITMAP_ETRO_MARKET_BUTTON, GL_LINEAR);
    LoadBitmap(L"Interface\\EtroMarketButtonAction.tga", BITMAP_ETRO_MARKET_BUTTON_ACTION, GL_LINEAR);
    LoadBitmap(L"Interface\\EtroMarketButtonPage.tga", BITMAP_ETRO_MARKET_BUTTON_PAGE, GL_LINEAR);
    LoadBitmap(L"Interface\\EtroMarketInput.tga", BITMAP_ETRO_MARKET_INPUT, GL_LINEAR);
    LoadBitmap(L"Interface\\EtroMarketDropdown.tga", BITMAP_ETRO_MARKET_DROPDOWN, GL_LINEAR);
    LoadBitmap(L"Interface\\EtroMarketItemSlot.tga", BITMAP_ETRO_MARKET_ITEM_SLOT, GL_LINEAR);
}

void CNewUIMarketWindow::UnloadImages()
{
    marketArt.Unload();
    referenceArt.Unload();
    g_pRenderText->SetFont(g_hFont);


    g_MarketFont=g_MarketTitleFont=nullptr;
    if (g_ParisMarketBuy != BITMAP_UNKNOWN) DeleteBitmap(g_ParisMarketBuy);
    g_ParisMarketBuy = BITMAP_UNKNOWN;
    DeleteBitmap(BITMAP_ETRO_MARKET_ITEM_SLOT);
    DeleteBitmap(BITMAP_ETRO_MARKET_DROPDOWN);
    DeleteBitmap(BITMAP_ETRO_MARKET_INPUT);
    DeleteBitmap(BITMAP_ETRO_MARKET_BUTTON_PAGE);
    DeleteBitmap(BITMAP_ETRO_MARKET_BUTTON_ACTION);
    DeleteBitmap(BITMAP_ETRO_MARKET_BUTTON);
    DeleteBitmap(BITMAP_ETRO_MARKET_CLOSE);
    DeleteBitmap(BITMAP_ETRO_MARKET_ROW);
    DeleteBitmap(BITMAP_ETRO_MARKET_PANEL);

    g_MarketPanelSelectionInitialized = false;
    g_MarketPanelLastPreferHd = false;
    g_MarketPanelUsingHd = false;
    g_MarketPanelSourceWidth = 800;
    g_MarketPanelSourceHeight = 520;
}

CNewUIMarketWindow::Rect CNewUIMarketWindow::LayoutRect(int x, int y, int width, int height) const
{
    return
    {
        m_Position.x + ScaleMarketUi(x),
        m_Position.y + ScaleMarketUi(y),
        ScaleMarketUi(width),
        ScaleMarketUi(height),
    };
}

CNewUIMarketWindow::Rect CNewUIMarketWindow::DebugRect(const Rect& rect, int target) const
{
    return rect;
}

void CNewUIMarketWindow::ApplySearchBoxLayout()
{
    if (m_SearchBox == nullptr)
    {
        return;
    }

    const auto input = DebugRect(LayoutRect(119, 107, 313, 28), DebugSearchInput);
    m_SearchBox->SetPosition(input.X + 4, input.Y + std::max(2, (input.Height - 12) / 2));
    m_SearchBox->SetSize(std::max(40, input.Width - 8), 12);
}

CNewUIMarketWindow::Rect CNewUIMarketWindow::ListingRowRect(int row) const
{
    return DebugRect(LayoutRect(31, 227 + (row * 39), 556, 39), DebugItemRow);
}

CNewUIMarketWindow::Rect CNewUIMarketWindow::SellerRowRect(int row) const
{
    return DebugRect(LayoutRect(31, 227 + (row * 39), 556, 39), DebugSellerRow);
}

CNewUIMarketWindow::Rect CNewUIMarketWindow::RowBuyRect(int row) const
{
    return DebugRect(LayoutRect(498, 236 + (row * 39), 82, 25), DebugRowBuy);
}

void CNewUIMarketWindow::AnnounceCapability()
{
    AnnounceCapability(false);
}

void CNewUIMarketWindow::AnnounceCapability(bool force)
{
    if ((m_CapabilitySent && !force) || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        return;
    }

    BYTE packet[5] = { 0xC1, 5, HeadCode, Capability, Version };
    SocketClient->Send(packet, sizeof(packet));
    m_CapabilitySent = true;
}

void CNewUIMarketWindow::OpenWindow()
{
    if (SceneFlag != MAIN_SCENE || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        return;
    }
    if (g_pNewUISystem && g_pNewUISystem->GetUI_NewMainFrameWindow())
        CParisCustomMenuWindow::CloseAllOverlays(this);


    UpdateMarketPanelPosition(m_Position);
    Show(true);
    m_WindowOpen = true;
    m_SelectedRow = -1;
    m_ConfirmRow = -1;
    m_PendingBuy = false;
    m_View = MarketView::LegacyItems;
    m_SellerCapability = false;
    m_CurrencyPricingAccepted = false;
    m_SelectedPriceCurrency = 0;
    m_PendingRequestId = 0;
    m_SelectedSeller.clear();
    m_Sellers.clear();
    m_Page = 0;
    ApplySearchBoxLayout();
    m_SearchBox->SetState(UISTATE_NORMAL);

    // ETRO MARKET CLIENT HANDSHAKE FIX V1.5.2
    // Always renegotiate on window open. The server stores SupportsEtroMarket
    // per RemotePlayer/connection, so a stale client-side flag must never let
    // Open/Search race ahead of the capability acknowledgement.
    m_CapabilitySent = false;
    m_CapabilityAccepted = false;
    m_WaitingForResults = true;
    SetStatus(L"Negociando mercado...");
    AnnounceCapability();

    PlayBuffer(SOUND_CLICK01);
}

void CNewUIMarketWindow::CloseWindow(bool playSound)
{
    if (!m_WindowOpen)
    {
        return;
    }

    m_WindowOpen = false;
    m_OpenFilter = -1;
    m_SelectedRow = -1;
    m_ConfirmRow = -1;
    m_PendingBuy = false;
    ClearTooltipItem();
    m_SearchBox->SetState(UISTATE_HIDE);
    if (playSound)
    {
        PlayBuffer(SOUND_CLICK01);
    }
}

bool CNewUIMarketWindow::IsWindowOpen() const
{
    return m_WindowOpen;
}

bool CNewUIMarketWindow::CanUseExtendedMarket() const
{
    return m_CapabilityAccepted && m_CurrencyPricingAccepted;
}

BYTE CNewUIMarketWindow::SelectedPriceCurrency() const
{
    return CanUseExtendedMarket() ? m_SelectedPriceCurrency : 0;
}

void CNewUIMarketWindow::SetSelectedPriceCurrency(BYTE currency)
{
    if (CanUseExtendedMarket() && (currency == 0 || currency == 1 || currency == 2
        || currency == 3 || currency == 4 || currency == 5 || currency == 9))
    {
        m_SelectedPriceCurrency = currency;
    }
}

void CNewUIMarketWindow::SendSetPriceRequest(BYTE slot, int amount, BYTE currency)
{
    if (!CanUseExtendedMarket() || amount <= 0
        || !(currency == 0 || currency == 1 || currency == 2 || currency == 3
            || currency == 4 || currency == 5 || currency == 9)
        || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        return;
    }

    BYTE packet[11] = { 0xC1, 11, HeadCode, SetPrice, Version, slot, currency, 0, 0, 0, 0 };
    WriteUInt32(packet + 7, static_cast<std::uint32_t>(amount));
    SocketClient->Send(packet, sizeof(packet));
}

void CNewUIMarketWindow::SendOpenRequest()
{
    if (!m_CapabilityAccepted || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        return;
    }

    BYTE packet[5] = { 0xC1, 5, HeadCode, Open, Version };
    SocketClient->Send(packet, sizeof(packet));
    m_WaitingForResults = true;
    SetStatus(L"Cargando tiendas...");
}

void CNewUIMarketWindow::SendSearchRequest()
{
    if (!m_CapabilityAccepted || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        SetStatus(L"Negociando mercado...");
        return;
    }

    const DWORD now = GetTickCount();
    if (m_LastRequestTick != 0 && now - m_LastRequestTick < RequestCooldown)
    {
        return;
    }

    wchar_t searchText[65]{};
    m_SearchBox->GetText(searchText, 65);
    auto utf8 = ToUtf8(searchText);
    const int length = 12 + static_cast<int>(utf8.size());
    std::vector<BYTE> packet(static_cast<std::size_t>(length));
    packet[0] = 0xC1;
    packet[1] = static_cast<BYTE>(length);
    packet[2] = HeadCode;
    packet[3] = Search;
    packet[4] = Version;
    WriteUInt16(packet.data() + 5, static_cast<std::uint16_t>(m_Page));
    packet[7] = PAGE_SIZE;
    packet[8] = m_Category;
    packet[9] = m_Currency;
    packet[10] = m_Sort;
    packet[11] = static_cast<BYTE>(utf8.size());
    std::copy(utf8.begin(), utf8.end(), packet.begin() + 12);
    SocketClient->Send(packet.data(), length);
    m_LastRequestTick = now;
    m_ConfirmRow = -1;
    m_WaitingForResults = true;
    SetStatus(L"Buscando...");
}

void CNewUIMarketWindow::SendSellerSearchRequest()
{
    if (!m_CapabilityAccepted || !m_SellerCapability || SocketClient == nullptr || !SocketClient->IsConnected()) return;
    wchar_t searchText[65]{};
    m_SearchBox->GetText(searchText, 65);
    const auto utf8 = ToUtf8(searchText);
    std::vector<BYTE> packet(16 + utf8.size());
    packet[0] = 0xC1;
    packet[1] = static_cast<BYTE>(packet.size());
    packet[2] = HeadCode;
    packet[3] = SellerSearch;
    packet[4] = Version;
    WriteUInt16(packet.data() + 5, static_cast<std::uint16_t>(m_Page));
    packet[7] = PAGE_SIZE;
    packet[8] = m_Category;
    packet[9] = m_Currency;
    packet[10] = 0;
    packet[11] = static_cast<BYTE>(utf8.size());
    m_PendingRequestId = ++m_NextRequestId;
    if (m_PendingRequestId == 0) m_PendingRequestId = ++m_NextRequestId;
    WriteUInt32(packet.data() + 12, m_PendingRequestId);
    std::copy(utf8.begin(), utf8.end(), packet.begin() + 16);
    m_PendingView = MarketView::Sellers;
    m_PendingTick = GetTickCount();
    m_WaitingForResults = true;
    m_ConfirmRow = -1;
    SetStatus(L"Buscando vendedores...");
    SocketClient->Send(packet.data(), static_cast<int>(packet.size()));
}

void CNewUIMarketWindow::SendSellerItemsRequest()
{
    if (!m_CapabilityAccepted || !m_SellerCapability || m_SelectedSeller.empty()
        || SocketClient == nullptr || !SocketClient->IsConnected()) return;
    wchar_t searchText[65]{};
    m_SearchBox->GetText(searchText, 65);
    const auto utf8 = ToUtf8(searchText);
    const auto seller = ToUtf8(m_SelectedSeller.c_str());
    if (seller.empty() || seller.size() > 64) return;
    std::vector<BYTE> packet(17 + seller.size() + utf8.size());
    packet[0] = 0xC1;
    packet[1] = static_cast<BYTE>(packet.size());
    packet[2] = HeadCode;
    packet[3] = SellerItems;
    packet[4] = Version;
    WriteUInt16(packet.data() + 5, static_cast<std::uint16_t>(m_Page));
    packet[7] = PAGE_SIZE;
    packet[8] = m_Category;
    packet[9] = m_Currency;
    packet[10] = m_Sort;
    packet[11] = static_cast<BYTE>(utf8.size());
    m_PendingRequestId = ++m_NextRequestId;
    if (m_PendingRequestId == 0) m_PendingRequestId = ++m_NextRequestId;
    WriteUInt32(packet.data() + 12, m_PendingRequestId);
    packet[16] = static_cast<BYTE>(seller.size());
    std::copy(seller.begin(), seller.end(), packet.begin() + 17);
    std::copy(utf8.begin(), utf8.end(), packet.begin() + 17 + seller.size());
    m_PendingView = MarketView::SellerItems;
    m_PendingTick = GetTickCount();
    m_WaitingForResults = true;
    m_ConfirmRow = -1;
    SetStatus(L"Cargando tienda...");
    SocketClient->Send(packet.data(), static_cast<int>(packet.size()));
}

void CNewUIMarketWindow::EnterSeller(const std::wstring& seller)
{
    wchar_t savedText[65]{};
    m_SearchBox->GetText(savedText, 65);
    m_SavedSearch = savedText;
    m_SavedPage = m_Page;
    m_SavedCategory = m_Category;
    m_SavedCurrency = m_Currency;
    m_SavedSort = m_Sort;
    m_SearchBox->SetText(L"");
    m_SelectedSeller = seller;
    m_View = MarketView::SellerItems;
    m_Page = 0;
    m_Category = AnyFilter;
    m_Currency = AnyFilter;
    m_Sort = 0;
    m_Listings.clear();
    m_SelectedRow = -1;
    ClearTooltipItem();
    SendSellerItemsRequest();
}

void CNewUIMarketWindow::ReturnToSellers()
{
    m_View = MarketView::Sellers;
    m_SelectedSeller.clear();
    m_SelectedRow = -1;
    m_Listings.clear();
    ClearTooltipItem();
    m_Page = m_SavedPage;
    m_Category = m_SavedCategory;
    m_Currency = m_SavedCurrency;
    m_Sort = m_SavedSort;
    m_SearchBox->SetText(m_SavedSearch.c_str());
    SendSellerSearchRequest();
}

void CNewUIMarketWindow::SendActiveRequest()
{
    if (m_View == MarketView::Sellers) SendSellerSearchRequest();
    else if (m_View == MarketView::SellerItems) SendSellerItemsRequest();
    else SendSearchRequest();
}

void CNewUIMarketWindow::SendBuyRequest(const Listing& listing)
{
    if (m_PendingBuy || SocketClient == nullptr || !SocketClient->IsConnected()) return;
    BYTE packet[41] = { 0xC1, 41, HeadCode, Buy, Version };
    const auto requestId = CreateRequestId();
    m_PendingBuyRequestId = requestId;
    m_PendingBuyListingId = listing.ListingId;
    m_PendingBuyCurrency = listing.Currency;
    m_PendingBuyAmount = listing.Amount;
    m_PendingBuy = true;
    std::copy(requestId.begin(), requestId.end(), packet + 5);
    std::copy(listing.ListingId.begin(), listing.ListingId.end(), packet + 21);
    WriteUInt32(packet + 37, listing.Revision);
    SocketClient->Send(packet, sizeof(packet));
    SetStatus(L"Procesando compra...");
    m_ConfirmRow = -1;
}

void CNewUIMarketWindow::ReceivePacket(const BYTE* packet, int size)
{
    if (packet == nullptr || size < 5)
    {
        return;
    }

    const bool longHeader = packet[0] == 0xC2 || packet[0] == 0xC4;
    const int headOffset = longHeader ? 3 : 2;
    const int subOffset = longHeader ? 4 : 3;
    const int payloadOffset = longHeader ? 5 : 4;
    if (size <= payloadOffset || packet[headOffset] != HeadCode || packet[payloadOffset] != Version)
    {
        return;
    }

    switch (packet[subOffset])
    {
    case Capability:
        if (size >= payloadOffset + 2)
        {
            m_CapabilityAccepted = packet[payloadOffset + 1] != 0;
            m_SellerCapability = size >= payloadOffset + 3 && (packet[payloadOffset + 2] & 1) != 0;
            m_CurrencyPricingAccepted = m_CapabilityAccepted && size >= payloadOffset + 3
                && (packet[payloadOffset + 2] & 2) != 0;
            if (!m_CurrencyPricingAccepted) m_SelectedPriceCurrency = 0;
            if (m_CapabilityAccepted)
            {
                // The Open request is intentionally sent only after the server
                // has acknowledged capability support. This removes the race
                // where Open could be silently rejected before
                // RemotePlayer.SupportsEtroMarket became true.
                if (m_WindowOpen)
                {
                    m_View = MarketView::LegacyItems;
                    SendOpenRequest();
                }
            }
            else
            {
                m_WaitingForResults = false;
                SetStatus(L"Market inactivo");
            }
        }
        break;
    case SearchResults:
        if (m_WindowOpen && m_View == MarketView::LegacyItems)
            ParseSearchResults(packet + payloadOffset, size - payloadOffset);
        break;
    case SellerSearchResults:
        if (m_WindowOpen && m_View == MarketView::Sellers)
            ParseSellerSearchResults(packet + payloadOffset, size - payloadOffset);
        break;
    case SellerItemResults:
        if (m_WindowOpen && m_View == MarketView::SellerItems)
            ParseSellerItemResults(packet + payloadOffset, size - payloadOffset);
        break;
    case BuyResult:
        if (m_PendingBuy && size >= payloadOffset + 38
            && std::equal(m_PendingBuyRequestId.begin(), m_PendingBuyRequestId.end(), packet + payloadOffset + 2)
            && std::equal(m_PendingBuyListingId.begin(), m_PendingBuyListingId.end(), packet + payloadOffset + 18))
        {
            const BYTE result = packet[payloadOffset + 1];
            m_PendingBuy = false;
            if (result == 4 && m_PendingBuyCurrency >= 2)
            {
                int missing = m_PendingBuyAmount;
                if (size >= payloadOffset + 43 && packet[payloadOffset + 38] == m_PendingBuyCurrency)
                    missing = ReadInt32(packet + payloadOffset + 39);
                wchar_t message[128] = {};
                mu_swprintf(message, L"Deposita %d %ls en Jewel Bank para comprar este articulo",
                    missing, CurrencyName(m_PendingBuyCurrency));
                SetStatus(message);
            }
            else
            {
                SetStatus(result <= 1 ? L"Compra completada" : L"La compra fue rechazada o la publicacion cambio");
            }
            m_LastRequestTick = 0;
            if (m_View == MarketView::SellerItems) SendSellerItemsRequest();
            else if (m_View == MarketView::LegacyItems) SendSearchRequest();
        }
        break;
    case SetPriceResult:
        SetStatus(size >= payloadOffset + 4 && packet[payloadOffset + 1] == 1
            ? L"Precio y moneda guardados en la tienda personal"
            : L"El servidor rechazo el precio o la moneda elegida");
        break;
    case Balances:
        break;
    case Error:
        SetStatus(L"Solicitud de mercado invalida o no autorizada");
        m_WaitingForResults = false;
        break;
    default:
        break;
    }
}

void CNewUIMarketWindow::ParseSellerSearchResults(const BYTE* payload, int payloadSize)
{
    if (payload == nullptr || payloadSize < 13 || !m_WaitingForResults
        || m_PendingView != MarketView::Sellers || ReadUInt32(payload + 1) != m_PendingRequestId) return;
    const int count = payload[12];
    int offset = 13;
    std::vector<std::wstring> sellers;
    sellers.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
    {
        if (offset >= payloadSize) return;
        const int length = payload[offset++];
        if (length == 0 || length > 64 || offset + length > payloadSize) return;
        sellers.push_back(FromFixedUtf8(payload + offset, length));
        offset += length;
    }
    if (offset != payloadSize) return;
    m_Page = ReadUInt16(payload + 5);
    m_TotalCount = std::max(0, ReadInt32(payload + 8));
    m_Sellers = std::move(sellers);
    m_SelectedRow = -1;
    ClearTooltipItem();
    m_WaitingForResults = false;
    m_PendingTick = 0;
    SetStatus(m_Sellers.empty() ? L"No hay vendedores para esta busqueda" : L"Vendedores actualizados");
}

void CNewUIMarketWindow::ParseSellerItemResults(const BYTE* payload, int payloadSize)
{
    if (payload == nullptr || payloadSize < 13 || !m_WaitingForResults
        || m_PendingView != MarketView::SellerItems || ReadUInt32(payload + 1) != m_PendingRequestId) return;
    // New C2 header has a four-byte request ID. Reuse the validated E5 item decoder.
    std::vector<BYTE> legacy(static_cast<std::size_t>(payloadSize - 4));
    legacy[0] = Version;
    std::copy(payload + 5, payload + payloadSize, legacy.begin() + 1);
    ParseSearchResults(legacy.data(), static_cast<int>(legacy.size()));
    if (!m_WaitingForResults) m_PendingTick = 0;
    if (m_Listings.empty()) SetStatus(L"El vendedor no tiene articulos disponibles");
}

void CNewUIMarketWindow::ParseSearchResults(const BYTE* payload, int payloadSize)
{
    if (payload == nullptr || payloadSize < 9)
    {
        return;
    }

    m_Page = ReadUInt16(payload + 1);
    m_TotalCount = ReadInt32(payload + 4);
    const int count = payload[8];
    int offset = 9;
    std::vector<Listing> listings;
    listings.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index)
    {
        if (offset + 54 > payloadSize)
        {
            return;
        }

        Listing listing;
        std::copy(payload + offset, payload + offset + 16, listing.ListingId.begin());
        listing.Revision = ReadUInt32(payload + offset + 16);
        std::copy(payload + offset + 20, payload + offset + 36, listing.ItemId.begin());
        listing.Seller = FromFixedUtf8(payload + offset + 36, 10);
        listing.Slot = payload[offset + 46];
        listing.Category = payload[offset + 47];
        listing.Currency = payload[offset + 48];
        listing.Amount = ReadInt32(payload + offset + 49);
        const int itemLength = payload[offset + 53];
        offset += 54;
        if (itemLength < 5 || offset + itemLength > payloadSize)
        {
            return;
        }

        listing.ItemData.assign(payload + offset, payload + offset + itemLength);
        offset += itemLength;
        listings.push_back(std::move(listing));
    }

    ClearTooltipItem();
    m_Listings = std::move(listings);
    m_SelectedRow = m_Listings.empty() ? -1 : 0;
    m_ConfirmRow = -1;
    m_WaitingForResults = false;
    SetStatus(m_Listings.empty() ? L"No se encontraron publicaciones" : L"Publicaciones actualizadas");
}

bool CNewUIMarketWindow::UpdateMouseEvent()
{
    if (!m_WindowOpen)
    {
        return true;
    }

    // The initial LoadImages() can happen before WM_SIZE has applied the
    // selected physical resolution. Re-check here and swap STD/HD only when
    // the resolution bucket changes.
    EnsureMarketPanelTextureForCurrentResolution(false);

    // ETRO MARKET ALPHA FIX V1:
    // Transparent TGA/OZT UI textures require the NewUI alpha state before
    // RenderImage. Without it, RGB stored under alpha=0 becomes visible as
    // rectangular/striped outer borders.
    EnableAlphaTest();
    UI::Paris::Color(1.0f, 1.0f, 1.0f, 1.0f);

    UpdateMarketPanelPosition(m_Position);
    ApplySearchBoxLayout();
    if (m_OpenFilter >= 0 && SEASON3B::IsPress(VK_LBUTTON))
    {
        const int filter = m_OpenFilter;
        const int count = filter == 0 ? 17 : filter == 1 ? 8 : 2;
        for (int option = 0; option < count; ++option)
        {
            if (!IsMouseOver(FilterOptionRect(filter, option))) continue;
            static constexpr BYTE currencies[] = { AnyFilter, 0, 1, 2, 3, 4, 5, 9 };
            if (filter == 0) m_Category = option == 0 ? AnyFilter : static_cast<BYTE>(option - 1);
            else if (filter == 1) m_Currency = currencies[option];
            else m_Sort = static_cast<BYTE>(option);
            m_OpenFilter = -1;
            m_Page = 0;
            SendActiveRequest();
            return false;
        }
        // Clicking outside dismisses without clicking through into the list.
        if (!IsMouseOver(FilterRect(filter)))
        {
            m_OpenFilter = -1;
            return false;
        }
    }
    m_SearchBox->DoAction();
    if (!SEASON3B::IsPress(VK_LBUTTON))
    {
        return !CheckMouseIn(m_Position.x, m_Position.y, MarketPanelWidth(), MarketPanelHeight());
    }

    const Rect close = DebugRect(LayoutRect(754, 14, 24, 24), DebugTopClose);
    const Rect search = DebugRect(LayoutRect(451, 106, 136, 26), DebugSearchButton);
    const Rect refresh = DebugRect(LayoutRect(451, 140, 136, 26), DebugRefreshButton);
    const Rect category = DebugRect(FilterRect(0), DebugCategory);
    const Rect currency = DebugRect(FilterRect(1), DebugCurrency);
    const Rect sort = DebugRect(FilterRect(2), DebugSort);
    const Rect previous = DebugRect(LayoutRect(193, 490, 132, 32), DebugPrevious);
    const Rect next = DebugRect(LayoutRect(475, 490, 132, 32), DebugNext);
    const Rect bottomClose = DebugRect(LayoutRect(338, 490, 124, 32), DebugBottomClose);
    const Rect back = DebugRect(LayoutRect(24, 140, 116, 26), DebugBackButton);

    if (m_View == MarketView::SellerItems && IsMouseOver(back))
    {
        ReturnToSellers();
    }
    else if (IsMouseOver(close) || IsMouseOver(bottomClose))
    {
        UI::Paris::ConsumeMouse();
        CloseWindow();
    }
    else if (!m_WaitingForResults && IsMouseOver(search))
    {
        m_Page = 0;
        SendActiveRequest();
    }
    else if (!m_WaitingForResults && IsMouseOver(refresh))
    {
        SendActiveRequest();
    }
    else if (!m_WaitingForResults && IsMouseOver(category))
        m_OpenFilter = m_OpenFilter == 0 ? -1 : 0;
    else if (!m_WaitingForResults && IsMouseOver(currency))
        m_OpenFilter = m_OpenFilter == 1 ? -1 : 1;
    else if (!m_WaitingForResults && IsMouseOver(sort))
        m_OpenFilter = m_OpenFilter == 2 ? -1 : 2;
    else if (IsMouseOver(previous) && CanGoPrevious())
    {
        --m_Page;
        SendActiveRequest();
    }
    else if (IsMouseOver(next) && CanGoNext())
    {
        ++m_Page;
        SendActiveRequest();
    }
    else if (m_View == MarketView::Sellers)
    {
        if (!m_WaitingForResults)
        {
            for (int row = 0; row < std::min<int>(PAGE_SIZE, static_cast<int>(m_Sellers.size())); ++row)
            {
                if (IsMouseOver(SellerRowRect(row)))
                {
                    EnterSeller(m_Sellers[static_cast<std::size_t>(row)]);
                    break;
                }
            }
        }
    }
    else
    {
        for (int row = 0; row < std::min<int>(static_cast<int>(PAGE_SIZE), static_cast<int>(m_Listings.size())); ++row)
        {
            if (CanBuy() && IsMouseOver(RowBuyRect(row)))
            {
                m_SelectedRow = row;
                if (m_ConfirmRow == row)
                {
                    SendBuyRequest(m_Listings[static_cast<std::size_t>(row)]);
                }
                else
                {
                    m_ConfirmRow = row;
                    SetStatus(L"Pulse Comprar nuevamente para confirmar");
                }

                break;
            }

            if (IsMouseOver(ListingRowRect(row)))
            {
                m_SelectedRow = row;
                m_ConfirmRow = -1;
                SetStatus(L"Publicacion seleccionada");
                break;
            }
        }
    }

    return false;
}

bool CNewUIMarketWindow::CloseForMenuHotKey()
{
    if (!m_WindowOpen || m_SearchBox->HaveFocus() || g_pChatInputBox->HaveFocus()) return false;
    constexpr int menuKeys[] = {'C','V','I','T','P','G','U','D','M','X','B','Z','Y','L','O','H'};
    for (const int key : menuKeys)
    {
        if (!SEASON3B::IsPress(key)) continue;
        CloseWindow();
        return true;
    }
    return false;
}

bool CNewUIMarketWindow::UpdateKeyEvent()
{
    if (CloseForMenuHotKey()) return true;
    if (!m_WindowOpen)
    {
        return true;
    }

    if (SEASON3B::IsPress(VK_ESCAPE))
    {
        if (m_OpenFilter >= 0) m_OpenFilter = -1;
        else CloseWindow();
        return false;
    }

    if (SEASON3B::IsPress(VK_RETURN))
    {
        m_Page = 0;
        SendActiveRequest();
        return false;
    }

    return true;
}

bool CNewUIMarketWindow::Update()
{
    if (m_WindowOpen && !CNewUIObj::IsVisible()) CloseWindow(false);
    if (m_SellerCapability && m_WaitingForResults && m_PendingTick != 0
        && GetTickCount() - m_PendingTick > 10000)
    {
        m_WaitingForResults = false;
        m_PendingTick = 0;
        SetStatus(L"Tiempo de espera agotado; pulsa Refrescar");
    }
    if (SceneFlag != MAIN_SCENE || SocketClient == nullptr || !SocketClient->IsConnected())
    {
        m_CapabilitySent = false;
        m_CapabilityAccepted = false;
        m_CurrencyPricingAccepted = false;
        m_SelectedPriceCurrency = 0;
        m_WaitingForResults = false;
        CloseWindow(false);
    }
    else
    {
        AnnounceCapability();
    }

    return true;
}

CNewUIMarketWindow::Rect CNewUIMarketWindow::FilterRect(int filter) const
{
    return LayoutRect(25 + filter * 193, 62, 180, 29);
}

CNewUIMarketWindow::Rect CNewUIMarketWindow::FilterOptionRect(int filter, int option) const
{
    return LayoutRect(25 + filter * 193, 91 + option * 24, 180, 24);
}

void CNewUIMarketWindow::RenderFilterMenu()
{
    if (m_OpenFilter < 0) return;
    const int count = m_OpenFilter == 0 ? 17 : m_OpenFilter == 1 ? 8 : 2;
    static constexpr BYTE currencies[] = { AnyFilter, 0, 1, 2, 3, 4, 5, 9 };
        g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    for (int option = 0; option < count; ++option)
    {
        const Rect rect = FilterOptionRect(m_OpenFilter, option);
        const wchar_t* text = m_OpenFilter == 0 ? MarketCategories[option]
            : m_OpenFilter == 1 ? (option == 0 ? L"Todas" : CurrencyName(currencies[option]))
            : option == 0 ? L"Menor precio" : L"Mayor precio";
        const bool selected = m_OpenFilter == 0 ? m_Category == (option == 0 ? AnyFilter : option - 1)
            : m_OpenFilter == 1 ? m_Currency == currencies[option] : m_Sort == option;
        UI::Character::ParisStyle::Box(rect.X, rect.Y, rect.Width, rect.Height, selected || IsMouseOver(rect));
        g_pRenderText->SetTextColor(selected || IsMouseOver(rect) ? 255 : 235, selected ? 220 : 235, selected ? 130 : 235, 255);
        g_pRenderText->RenderText(rect.X + 5, rect.Y + 3, text, rect.Width - 10, 0, RT3_SORT_LEFT);
    }
}

bool CNewUIMarketWindow::Render()
{
    if (!m_WindowOpen) return true;
    UpdateMarketPanelPosition(m_Position);
    ApplySearchBoxLayout();
    EnableAlphaTest();
    UI::Character::ParisStyle::Solid(m_Position.x, m_Position.y, MarketPanelWidth(), MarketPanelHeight(), .025f, .025f, .025f);
    referenceArt.MarketFrame(m_Position.x, m_Position.y, MarketPanelWidth(), MarketPanelHeight());
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    referenceArt.MarketTitleDecoration(m_Position.x,m_Position.y,MarketPanelWidth());
    RenderMarketTitle(m_Position.x, m_Position.y, MarketPanelWidth());
    RenderButton(LayoutRect(754, 14, 24, 24), L"X");
    const wchar_t* categoryName = m_Category == AnyFilter ? MarketCategories[0]
        : m_Category < 16 ? MarketCategories[m_Category + 1] : L"Todas";
    const wchar_t* filterTexts[] = { categoryName, m_Currency == AnyFilter ? L"Todas" : CurrencyName(m_Currency), m_Sort == 0 ? L"Menor precio" : L"Mayor precio" };
    const wchar_t* filterNames[] = { L"Categoria", L"Moneda", L"Precio" };
    for (int filter = 0; filter < 3; ++filter)
    {
        const Rect rect = FilterRect(filter);
        g_pRenderText->SetTextColor(230, 204, 135, 255);
        g_pRenderText->RenderText(rect.X, rect.Y - 12, filterNames[filter], rect.Width, 0, RT3_SORT_LEFT);
        RenderButton(rect, L"", m_OpenFilter == filter, -1, !m_WaitingForResults);
        g_pRenderText->SetTextColor(245, 245, 245, 255);
        g_pRenderText->RenderText(rect.X + 5, rect.Y + (rect.Height - 11) / 2, filterTexts[filter], rect.Width - 22, 0, RT3_SORT_CENTER);
        referenceArt.Arrow(rect.X+rect.Width-15,rect.Y+rect.Height/2-2,9);
    }
    const Rect input = LayoutRect(119, 107, 313, 28);
    referenceArt.Button(input.X, input.Y, input.Width, input.Height);
    const Rect searchLabel = LayoutRect(29, 114, 88, 18);
    g_pRenderText->SetTextColor(235, 225, 200, 255);
    g_pRenderText->RenderText(searchLabel.X, searchLabel.Y, L"Buscar item:", searchLabel.Width, 0, RT3_SORT_LEFT);
    RenderButton(LayoutRect(451, 106, 136, 26), L"Buscar", false, -1, !m_WaitingForResults);
    RenderButton(LayoutRect(451, 140, 136, 26), L"Refrescar", false, -1, !m_WaitingForResults);
    if (m_View == MarketView::SellerItems) RenderButton(LayoutRect(24, 140, 116, 26), L"< Volver");
    m_SearchBox->Render();
    const Rect preview = LayoutRect(602, 47, 181, 395);
    referenceArt.PreviewFrame(preview.X, preview.Y, preview.Width, preview.Height);

    if (m_View != MarketView::Sellers) RenderSelectedPreview();
    const Rect table = LayoutRect(21, 173, 576, 269);
    referenceArt.InnerFrame(table.X, table.Y, table.Width, table.Height);
    const Rect section = LayoutRect(47, 182, 500, 18);
    g_pRenderText->SetFont(g_MarketTitleFont?g_MarketTitleFont:g_hFontBold);
    g_pRenderText->SetTextColor(244, 211, 126, 255);
    g_pRenderText->RenderText(section.X, section.Y, m_View == MarketView::Sellers ? L"VENDEDORES" : L"PUBLICACIONES", section.Width, 0, RT3_SORT_LEFT);
    g_pRenderText->SetFont(g_hFont);


    const Rect header=LayoutRect(30,202,557,25);
    referenceArt.MarketHeader(header.X,header.Y,header.Width,header.Height);
    const int columns[] = { 78, 232, 338, 424, 500 };
    const int widths[] = { 148, 102, 80, 70, 80 };
    const wchar_t* headers[] = { L"Item", L"Vendedor", L"Precio", L"Moneda", L"Accion" };
    g_pRenderText->SetTextColor(230, 206, 145, 255);
    for (int column = 0; column < 5; ++column)
    {
        if (m_View == MarketView::Sellers && column != 0 && column != 4) continue;
        const Rect label = LayoutRect(columns[column], 208, widths[column], 16);
        g_pRenderText->RenderText(label.X, label.Y, m_View == MarketView::Sellers && column == 0 ? L"Vendedor" : headers[column], label.Width, 0, RT3_SORT_CENTER);
    }
    const int count = m_View == MarketView::Sellers ? static_cast<int>(m_Sellers.size()) : static_cast<int>(m_Listings.size());
    for (int row = 0; row < std::min<int>(PAGE_SIZE, count); ++row)
    {
        const Rect rect = ListingRowRect(row);
        RenderPublicationBackground(rect.X,rect.Y,rect.Width,rect.Height,row);
        if (row == m_SelectedRow || IsMouseOver(rect))
            DrawPanel(rect.X + 1, rect.Y + 1, rect.Width - 2, rect.Height - 2, .40f, .28f, .06f, .30f);
        const int y = 242 + row * 39;
        if (m_View == MarketView::Sellers)
        {
            const Rect label = LayoutRect(68, y, 500, 22);
            g_pRenderText->SetTextColor(245, 245, 245, 255);
            g_pRenderText->RenderText(label.X, label.Y, m_Sellers[row].c_str(), label.Width, 0, RT3_SORT_CENTER);
            const Rect action = RowBuyRect(row);
            RenderButton(action, L"Ver items");
            continue;
        }
        const auto& listing = m_Listings[row];
        wchar_t itemName[160] = L"Item";
        if (listing.ItemData.size() >= 5)
        {
            const auto item = ParseItemData(std::span<const BYTE>(listing.ItemData.data(), listing.ItemData.size()));
            const int type = item.Group * MAX_ITEM_INDEX + item.Number;
            if (type >= 0 && type < MAX_ITEM) mu_swprintf(itemName, L"%ls +%u", ItemAttribute[type].Name, item.Level);
        }
        if (listing.Currency <= 1)
        {
            const Rect coin = LayoutRect(429, y - 1, 16, 18);
            referenceArt.Coin(coin.X,coin.Y,coin.Width,coin.Height);

        }
        wchar_t price[64]{}; mu_swprintf(price, L"%d", listing.Amount);
        const wchar_t* values[] = { itemName, listing.Seller.c_str(), price, CurrencyName(listing.Currency) };
        for (int column = 0; column < 4; ++column)
        {
            const Rect label = LayoutRect(columns[column] + (column == 3 ? 24 : 0), y, widths[column] - (column == 3 ? 24 : 0), 22);
            g_pRenderText->SetTextColor(255,255,255,255);
            g_pRenderText->RenderText(label.X, label.Y, values[column], label.Width, 0, RT3_SORT_CENTER);
        }
        RenderButton(RowBuyRect(row), m_ConfirmRow == row ? L"Confirmar" : L"Comprar", m_ConfirmRow == row, -1, CanBuy());
    }
    constexpr int columnDividers[] = { 224, 336, 422, 498 };
    for (const int boundary : columnDividers)
    {
        const Rect line = LayoutRect(boundary, 202, 2, 240);
        UI::Character::ParisStyle::Solid(line.X, line.Y, line.Width, line.Height, .27f, .23f, .16f);
    }
    if (count == 0)
    {
        const Rect label = LayoutRect(36, 314, 543, 20);
        g_pRenderText->SetTextColor(235, 215, 150, 255);
        g_pRenderText->RenderText(label.X, label.Y, m_WaitingForResults ? L"Cargando..." : L"No hay resultados para estos filtros", label.Width, 0, RT3_SORT_CENTER);
    }
    wchar_t page[100]{};
    mu_swprintf(page, L"%d %ls - Pagina %d / %d", m_TotalCount, m_View == MarketView::Sellers ? L"vendedores" : L"publicaciones", m_Page + 1, std::max(1, (m_TotalCount + PAGE_SIZE - 1) / PAGE_SIZE));
    const Rect pageLabel = LayoutRect(24, 464, 752, 20);
    g_pRenderText->SetTextColor(235, 215, 150, 255);
    g_pRenderText->RenderText(pageLabel.X, pageLabel.Y, page, pageLabel.Width, 0, RT3_SORT_CENTER);
    RenderButton(LayoutRect(193, 490, 132, 32), L"< Anterior", false, -1, CanGoPrevious());
    RenderButton(LayoutRect(338, 490, 124, 32), L"Cerrar");
    RenderButton(LayoutRect(475, 490, 132, 32), L"Siguiente >", false, -1, CanGoNext());
    const Rect status = LayoutRect(24, 529, 752, 13);
    g_pRenderText->SetTextColor(215, 205, 175, 255);
    g_pRenderText->RenderText(status.X, status.Y, m_Status.c_str(), status.Width, 0, RT3_SORT_CENTER);
    if (m_OpenFilter >= 0) RenderFilterMenu();
    else if (m_View != MarketView::Sellers) RenderHoveredItemTooltip();
    g_pRenderText->SetFont(g_hFont);
    return true;
}

void CNewUIMarketWindow::RenderButton(const Rect& rect, const wchar_t* text, bool active, int image, bool enabled, int textTarget) const
{
    const bool hovered = enabled && IsMouseOver(rect);

    referenceArt.Button(rect.X,rect.Y,rect.Width,rect.Height);
    const bool searchIcon=text && wcscmp(text,L"Buscar")==0;
    const bool refreshIcon=text && wcscmp(text,L"Refrescar")==0;
    const bool redAction = text && (wcscmp(text,L"Comprar")==0 || wcscmp(text,L"Confirmar")==0
        || wcscmp(text,L"< Anterior")==0);
    const bool greenAction = text && (wcscmp(text,L"Siguiente >")==0 || searchIcon);
    const bool blueAction = refreshIcon;
    if (redAction || greenAction || blueAction)
    {
        const float red = redAction ? .54f : .08f;
        const float green = redAction ? .07f : (greenAction ? .42f : .24f);
        const float blue = redAction ? .05f : (greenAction ? .10f : .62f);
        DrawPanel(static_cast<float>(rect.X + 3), static_cast<float>(rect.Y + 3),
            static_cast<float>(std::max(1, rect.Width - 6)), static_cast<float>(std::max(1, rect.Height - 6)),
            red, green, blue, .34f);
    }
    const int iconSize=12;
    if(searchIcon) referenceArt.SearchIcon(rect.X+10,rect.Y+(rect.Height-iconSize)/2,iconSize);
    if(refreshIcon) referenceArt.RefreshIcon(rect.X+10,rect.Y+(rect.Height-iconSize)/2,iconSize);


    if (!enabled)
    {
        DrawPanel(static_cast<float>(rect.X + 1), static_cast<float>(rect.Y + 1),
            static_cast<float>(std::max(1, rect.Width - 2)), static_cast<float>(std::max(1, rect.Height - 2)),
            0.02f, 0.02f, 0.02f, 0.56f);
    }
    else if (active || hovered)
    {
        DrawPanel(static_cast<float>(rect.X + 1), static_cast<float>(rect.Y + 1),
            static_cast<float>(rect.Width - 2), static_cast<float>(rect.Height - 2),
            0.45f, 0.30f, 0.06f, active ? 0.25f : 0.16f);
    }

    if (!enabled)
        g_pRenderText->SetTextColor(115, 110, 100, 255);
    else
        g_pRenderText->SetTextColor(245, 245, 245, 255);

    if (text == nullptr || text[0] == 0) return;
    const int labelInset=searchIcon || refreshIcon?18:0;
    const Rect labelRect = DebugRect({rect.X+labelInset, rect.Y + ((rect.Height - 11) / 2), rect.Width-labelInset, 11}, textTarget);
    g_pRenderText->RenderText(labelRect.X, labelRect.Y, text, labelRect.Width, 0, RT3_SORT_CENTER);
}

void CNewUIMarketWindow::RenderSelectedPreview()
{
    const Rect title = DebugRect(LayoutRect(608, 62, 170, 18), DebugPreviewTitle);

    g_pRenderText->SetFont(g_MarketTitleFont?g_MarketTitleFont:g_hFontBold);
    g_pRenderText->SetTextColor(241, 207, 115, 255);
    g_pRenderText->RenderText(title.X, title.Y, L"VISTA PREVIA", title.Width, 0, RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_MarketFont?g_MarketFont:g_hFont);

    if (m_SelectedRow < 0 || m_SelectedRow >= static_cast<int>(m_Listings.size()))
    {
        const Rect empty = DebugRect(LayoutRect(617, 109, 151, 22), DebugPreviewEmpty);
        g_pRenderText->SetTextColor(165, 165, 160, 255);
        g_pRenderText->RenderText(empty.X, empty.Y, L"Seleccione una publicacion", empty.Width, 0, RT3_SORT_CENTER);
        return;
    }

    const auto& listing = m_Listings[static_cast<std::size_t>(m_SelectedRow)];
    if (listing.ItemData.size() < 5) return;

    const auto item = ParseItemData(std::span<const BYTE>(listing.ItemData.data(), listing.ItemData.size()));
    const int type = item.Group * MAX_ITEM_INDEX + item.Number;
    if (type < 0 || type >= MAX_ITEM) return;
    const Rect slot = DebugRect(LayoutRect(619, 83, 146, 319), DebugPreviewSlot);
    const Rect nameRect = DebugRect(LayoutRect(610, 419, 166, 18), DebugPreviewName);

    g_pRenderText->SetTextColor(242, 215, 132, 255);
    g_pRenderText->RenderText(nameRect.X, nameRect.Y, ItemAttribute[type].Name, nameRect.Width, 0, RT3_SORT_CENTER);

}

void CNewUIMarketWindow::ClearTooltipItem()
{
    if (m_TooltipItem != nullptr && g_pNewItemMng != nullptr)
        g_pNewItemMng->DeleteItem(m_TooltipItem);
    m_TooltipItem = nullptr;
    m_TooltipRow = -1;
}

void CNewUIMarketWindow::RenderHoveredItemTooltip()
{
    int hoveredRow = -1;
    const int visibleRows = std::min<int>(static_cast<int>(PAGE_SIZE), static_cast<int>(m_Listings.size()));
    for (int row = 0; row < visibleRows; ++row)
    {
        if (IsMouseOver(ListingRowRect(row)) && !IsMouseOver(RowBuyRect(row)))
        {
            hoveredRow = row;
            break;
        }
    }

    const Rect previewSlot = DebugRect(LayoutRect(619, 83, 146, 319), DebugPreviewSlot);
    if (hoveredRow < 0 && m_SelectedRow >= 0 && m_SelectedRow < static_cast<int>(m_Listings.size())
        && IsMouseOver(previewSlot))
        hoveredRow = m_SelectedRow;

    if (hoveredRow < 0)
    {
        ClearTooltipItem();
        return;
    }

    if (hoveredRow != m_TooltipRow)
    {
        ClearTooltipItem();
        const auto& listing = m_Listings[static_cast<std::size_t>(hoveredRow)];
        if (g_pNewItemMng != nullptr && listing.ItemData.size() >= 5)
        {
            m_TooltipItem = g_pNewItemMng->CreateItem(
                std::span<const BYTE>(listing.ItemData.data(), listing.ItemData.size()));
            m_TooltipRow = m_TooltipItem == nullptr ? -1 : hoveredRow;
        }
    }

    if (m_TooltipItem == nullptr) return;

    // Keep every tooltip at the same position outside the publications list.
    // Debug target 66 moves the complete tooltip, independent of the row.
    const Rect background = DebugRect(
        { m_Position.x, m_Position.y, MarketPanelWidth(), MarketPanelHeight() },
        DebugBackground);
    const Rect tooltip = DebugRect(
        { std::clamp(MouseX + 12, 12, 400), std::clamp(MouseY + 12, 30, 240), 1, 1 },
        DebugTooltip);
    RenderMarketItemInfo(tooltip.X, tooltip.Y, m_TooltipItem);
}

bool CNewUIMarketWindow::IsMouseOver(const Rect& rect) const
{
    return CheckMouseIn(rect.X, rect.Y, rect.Width, rect.Height);
}

bool CNewUIMarketWindow::CanGoPrevious() const
{
    return !m_WaitingForResults && m_Page > 0;
}

bool CNewUIMarketWindow::CanGoNext() const
{
    return !m_WaitingForResults && (m_Page + 1) * PAGE_SIZE < m_TotalCount;
}

bool CNewUIMarketWindow::CanBuy() const
{
    return !m_WaitingForResults && m_CapabilityAccepted;
}

const wchar_t* CNewUIMarketWindow::CurrencyName(BYTE currency)
{
    static constexpr const wchar_t* names[] =
    {
        L"Zen", L"Paris Coins", L"Bless", L"Soul", L"Life", L"Creation",
        L"Guardian", L"Gemstone", L"Harmony", L"Chaos", L"LowerRefine", L"HigherRefine",
    };
    return currency < _countof(names) ? names[currency] : L"Desconocida";
}

void CNewUIMarketWindow::SetStatus(const wchar_t* value)
{
    m_Status = value == nullptr ? L"" : value;
}

void CNewUIMarketWindow::Render3D()
{
    if (!IsVisible() || m_View == MarketView::Sellers || m_OpenFilter >= 0) return;
    const auto renderItem = [&](const Listing& listing, const Rect& previewFrame, int currencyType = -1)
    {
        if (listing.ItemData.size() < 5) return;
        const auto item = ParseItemData(std::span<const BYTE>(listing.ItemData.data(), listing.ItemData.size()));
        const int type = currencyType >= 0 ? currencyType : item.Group * MAX_ITEM_INDEX + item.Number;
        if (type < 0 || type >= MAX_ITEM) return;
        const Rect slot = previewFrame;
        UI::Paris::Clip(slot.X, slot.Y, slot.Width, slot.Height);
                RenderMarketPreviewItem3D(static_cast<float>(slot.X), static_cast<float>(slot.Y),
                    static_cast<float>(slot.Width), static_cast<float>(slot.Height), type, currencyType >= 0 ? 0 : item.Level,
                    currencyType >= 0 ? 0 : item.ExcellentFlags, currencyType >= 0 ? 0 : item.AncientDiscriminator,
                    std::min(slot.Height>100?1.2f:.85f, std::min(slot.Width / (24.f * std::max(1, int(ItemAttribute[type].Width))),
                        slot.Height / (24.f * std::max(1, int(ItemAttribute[type].Height))))));
        UI::Paris::ResetClip();
    };
    for (int row = 0; row < std::min<int>(PAGE_SIZE, static_cast<int>(m_Listings.size())); ++row)
    {
        const auto& listing = m_Listings[row];
        renderItem(listing, LayoutRect(43, 229 + row * 39, 30, 36));
        constexpr int currencies[] = {14*512+15, 14*512+15, 14*512+13, 14*512+14,
            14*512+16, 14*512+22, 14*512+31, 14*512+41, 14*512+42, 12*512+15, 14*512+43, 14*512+44};
        if (listing.Currency > 1 && listing.Currency < std::size(currencies))
            renderItem(listing, LayoutRect(429, 239 + row * 39, 18, 20), currencies[listing.Currency]);
    }
    if (m_SelectedRow >= 0 && m_SelectedRow < static_cast<int>(m_Listings.size()))
        renderItem(m_Listings[m_SelectedRow], LayoutRect(627, 103, 130, 275));
}

bool CNewUIMarketWindow::IsVisible() const
{
    return m_WindowOpen && CNewUIObj::IsVisible();
}

float CNewUIMarketWindow::GetLayerDepth()
{
    return 7.0f;
}

float CNewUIMarketWindow::GetKeyEventOrder()
{
    return 6.0f;
}

