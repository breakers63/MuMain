#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include "UI/Inventory/ItemMng.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/Tooltip/LegacyTextListTooltip.h"
#include "Render/Items/ItemPreviewFit.h"
#include "Engine/Object/ZzzInventory.h"
#include <cstdio>
#include "UI/Dialogs/ParisAuctionWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "Network/Paris/AuctionProtocol.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Dialogs/GenericConfirmDialog.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <charconv>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50415543u;

Rml::String CurrencyName(int currency)
{
    const auto& labels = Network::Paris::Auctions::Data.Labels;
    const auto found = labels.find("Currency" + std::to_string(currency));
    if (found != labels.end()) return StringUtils::WideToNarrow(found->second.c_str());
    constexpr const char* names[] = {"Zen", "WCoins", "WPoints", "Chaos", "Bless", "Soul", "Life", "Creation"};
    return currency >= 0 && currency < 8 ? names[currency] : "Moneda";
}

Rml::String PriceText(int amount, int currency)
{
    return std::to_string(amount) + " " + CurrencyName(currency);
}

Rml::String DurationText(int minutes)
{
    if (minutes % 1440 == 0) return std::to_string(minutes / 1440) + " d";
    if (minutes % 60 == 0) return std::to_string(minutes / 60) + " h";
    return std::to_string(minutes) + " min";
}

Rml::String ListingState(const Network::Paris::Auctions::Row& row)
{
    if (row.Status == 1) return "Vendida";
    if (row.Status == 2) return "Vencida";
    if (row.Status == 3) return "Cancelada";
    return "Activa";
}
} // namespace

CParisAuctionWindow* g_pParisAuctionWindow = nullptr;
CParisAuctionWindow::CParisAuctionWindow() = default;

CParisAuctionWindow::~CParisAuctionWindow()
{
    Release();
}

bool CParisAuctionWindow::Create(CManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisAuctionWindow::Release()
{
    ClearItems();
    for (auto& target : m_targets) target.reset();
    m_RmlView.Release();
    if (m_manager)
    {
        m_manager->RemoveUIObj(this);
        m_manager = nullptr;
    }
}

void CParisAuctionWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name);
    row.RegisterMember("options", &Row::options); row.RegisterMember("time", &Row::time);
    row.RegisterMember("seller", &Row::seller);
    row.RegisterMember("price", &Row::price);
    row.RegisterMember("state", &Row::state);
    row.RegisterMember("action", &Row::action);
    row.RegisterMember("index", &Row::index);
    row.RegisterMember("enabled", &Row::enabled);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("status", &model.status);
    c.Bind("query", &model.query); c.Bind("group", &model.group); c.Bind("subtab", &model.subtab);
    c.BindEventCallback("auction_filter", [this](auto, auto&, const auto&) { RequestPage(Network::Paris::Auctions::Data.Tab, 0); });
    c.BindEventCallback("auction_subtab", [this](auto, auto&, const Rml::VariantList& args)
    {
        if (args.empty()) return;
        m_RmlView.GetModel().subtab = std::clamp(args[0].Get<int>(), 0, 1); m_RmlView.MarkDirty("subtab");
        RequestPage(Network::Paris::Auctions::Data.Tab, 0);
    });
    c.Bind("price_input", &model.priceInput);
    c.Bind("buy_now_input", &model.buyNowInput);
    c.Bind("duration_label", &model.durationLabel);
    c.Bind("currency_label", &model.currencyLabel);
    c.Bind("rows", &model.rows);
    c.Bind("tab", &model.tab);
    c.Bind("page", &model.page);
    c.Bind("page_count", &model.pageCount);
    c.Bind("empty", &model.empty);
    c.BindEventCallback("paris_auction_tab", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (!args.empty()) RequestPage(args[0].Get<int>(-1), 0);
    });
    c.BindEventCallback("paris_auction_refresh", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        const auto& data = Network::Paris::Auctions::Data;
        RequestPage(data.Tab, data.Page);
    });
    c.BindEventCallback("paris_auction_previous", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        const auto& data = Network::Paris::Auctions::Data;
        if (data.Page > 0) RequestPage(data.Tab, data.Page - 1);
    });
    c.BindEventCallback("paris_auction_next", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        const auto& data = Network::Paris::Auctions::Data;
        const int pages = data.More ? data.Page + 2 : data.Page + 1;
        if (data.Page + 1 < pages) RequestPage(data.Tab, data.Page + 1);
    });
    c.BindEventCallback("paris_auction_action", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (!args.empty()) HandleRowAction(args[0].Get<int>(-1));
    });
    c.BindEventCallback("paris_auction_duration", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        const auto& data = Network::Paris::Auctions::Data;
        if (data.Durations.empty()) return;
        m_durationIndex = (m_durationIndex + 1) % static_cast<int>(data.Durations.size());
        SyncModel();
    });
    c.BindEventCallback("paris_auction_currency", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        const auto& data = Network::Paris::Auctions::Data;
        if (data.Currencies.empty()) return;
        const auto found = std::find(data.Currencies.begin(), data.Currencies.end(), m_currency);
        const int currentIndex = found == data.Currencies.end() ? -1 : static_cast<int>(found - data.Currencies.begin());
        m_currency = data.Currencies[static_cast<std::size_t>((currentIndex + 1) % static_cast<int>(data.Currencies.size()))];
        SyncModel();
    });
    c.BindEventCallback("paris_auction_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        CloseWindow();
    });
}

void CParisAuctionWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
    {
        m_RmlView.Document()->Hide();
        for (auto& target : m_targets) if (target) target->Disable();
        if (m_tooltipShown) UI::Tooltip::HideLegacyTextList(); m_tooltipShown = false;
    }
}

void CParisAuctionWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    for (int i = 0; i < 16; ++i) if (!m_targets[i])
    {
        m_targets[i] = std::make_unique<UI::Items::ItemCameraTarget>([this, i](const auto& offset, const auto& size) { DrawItem(i, offset, size); });
        m_targets[i]->SetContinuous(false);
    }
    m_nextSync = 0;
    Show(true);
    RequestPage(0, 0);
}

void CParisAuctionWindow::CloseWindow()
{
    Show(false);
}

bool CParisAuctionWindow::UpdateMouseEvent()
{
    return !IsVisible();
}

bool CParisAuctionWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE))
    {
        CloseWindow();
        return false;
    }
    return !IsVisible();
}

bool CParisAuctionWindow::Update()
{
    return true;
}

bool CParisAuctionWindow::Render()
{
    if (!IsVisible()) return true;
    if (timeGetTime() >= m_nextSync || m_itemsReceived != Network::Paris::Auctions::Received)
    {
        m_nextSync = timeGetTime() + 1000; SyncModel();
    }
    SyncItems();
    return true;
}

void CParisAuctionWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    const auto& data = Network::Paris::Auctions::Data;
    auto& model = m_RmlView.GetModel();
    const auto status = data.Enabled ? Rml::String("Subastas activas") : Rml::String("Sistema no habilitado por el servidor");
    if (model.status != status) { model.status = status; m_RmlView.MarkDirty("status"); }
    if (m_durationIndex >= static_cast<int>(data.Durations.size())) m_durationIndex = 0;
    if (!data.Currencies.empty() && std::find(data.Currencies.begin(), data.Currencies.end(), m_currency) == data.Currencies.end())
        m_currency = data.Currencies.front();
    const Rml::String durationLabel = data.Durations.empty() ? "Duración no disponible"
        : DurationText(data.Durations[static_cast<std::size_t>(m_durationIndex)]);
    if (model.durationLabel != durationLabel) { model.durationLabel = durationLabel; m_RmlView.MarkDirty("duration_label"); }
    const Rml::String currencyLabel = data.Currencies.empty() ? "Moneda no disponible" : CurrencyName(m_currency);
    if (model.currencyLabel != currencyLabel) { model.currencyLabel = currencyLabel; m_RmlView.MarkDirty("currency_label"); }

    std::vector<Row> rows;
    rows.reserve(data.Rows.size());
    for (int index = 0; index < static_cast<int>(data.Rows.size()); ++index)
    {
        const auto& source = data.Rows[static_cast<std::size_t>(index)];
        Row row;
        row.name = StringUtils::WideToNarrow(source.Name.c_str());
        row.seller = StringUtils::WideToNarrow(source.Seller.c_str());
        row.index = index;
        row.options = "+" + std::to_string(source.Level);
        const long long seconds = std::max(0LL, source.Ends - data.ServerTime - static_cast<long long>((timeGetTime() - Network::Paris::Auctions::Received) / 1000));
        char remaining[32]{}; std::snprintf(remaining, sizeof(remaining), "%02lld:%02lld:%02lld", seconds / 3600, seconds / 60 % 60, seconds % 60);
        row.time = source.Ends > 0 ? remaining : "—";
        if (data.Tab == 1)
        {
            row.price = "Casilla " + std::to_string(source.Slot + 1);
            row.action = "Publicar";
            row.enabled = data.Enabled && !data.Currencies.empty() && !data.Durations.empty();
        }
        else if (data.Tab == 3)
        {
            row.price = source.ItemDelivery ? "Objeto" : PriceText(source.Amount, source.Currency);
            row.action = "Recoger";
            row.enabled = !source.DeliveryId.empty();
        }
        else if (data.Tab == 4)
        {
            row.price = PriceText(source.Price, source.Currency);
            row.state = ListingState(source);
            row.action = "Cancelar";
            row.enabled = source.CanCancel;
        }
        else if (data.Tab == 2)
        {
            row.price = PriceText(source.Amount, source.Currency);
            row.state = source.State.empty() ? ListingState(source) : source.State;
        }
        else
        {
            row.price = PriceText(source.Price, source.Currency);
            row.state = ListingState(source);
            row.action = source.BuyNow > 0 ? "Comprar" : source.NextBid > 0 ? "Ofertar" : "Ver";
            row.enabled = !source.Owned && (source.BuyNow > 0 || source.NextBid > 0) && data.Enabled;
        }
        rows.push_back(std::move(row));
    }
    if (model.rows != rows) { model.rows = std::move(rows); m_RmlView.MarkDirty("rows"); }
    const bool empty = model.rows.empty();
    if (model.empty != empty) { model.empty = empty; m_RmlView.MarkDirty("empty"); }
    if (model.tab != data.Tab) { model.tab = data.Tab; m_RmlView.MarkDirty("tab"); }
    const int page = data.Page + 1;
    if (model.page != page) { model.page = page; m_RmlView.MarkDirty("page"); }
    const int pageCount = std::max(1, data.More ? data.Page + 2 : data.Page + 1);
    if (model.pageCount != pageCount) { model.pageCount = pageCount; m_RmlView.MarkDirty("page_count"); }
}

void CParisAuctionWindow::RequestPage(int tab, int page)
{
    if (tab < 0 || tab > 4 || page < 0 || page > 100) return;
    auto& model = m_RmlView.GetModel();
    if (tab != Network::Paris::Auctions::Data.Tab) { model.subtab = 0; m_RmlView.MarkDirty("subtab"); }
    Network::Paris::Auctions::Request(0, static_cast<BYTE>(tab), static_cast<BYTE>(page), static_cast<BYTE>(model.subtab),
        StringUtils::NarrowToWide(model.query), static_cast<BYTE>(model.group));
}

void CParisAuctionWindow::HandleRowAction(int index)
{
    const auto& data = Network::Paris::Auctions::Data;
    if (index < 0 || index >= static_cast<int>(data.Rows.size())) return;
    const auto row = data.Rows[static_cast<std::size_t>(index)];
    if (data.Tab == 1)
    {
        PublishItem(row);
        return;
    }
    if (data.Tab == 3 && !row.DeliveryId.empty())
    {
        Network::Paris::Auctions::Request(5, 3, static_cast<BYTE>(data.Page), static_cast<BYTE>(data.SubTab), L"", 255, &row);
        return;
    }
    if (data.Tab == 4 && row.CanCancel)
    {
        GenericDialogConfig confirm;
        confirm.title = L"Cancelar subasta";
        confirm.lines.push_back({row.Name, false});
        confirm.primaryLabel = L"Cancelar publicación";
        confirm.showCancel = true;
        confirm.cancelLabel = L"Volver";
        confirm.onPrimary = [row, page = data.Page, subTab = data.SubTab]
        {
            Network::Paris::Auctions::Request(4, 4, static_cast<BYTE>(page), static_cast<BYTE>(subTab), L"", 255, &row);
        };
        if (g_pGenericConfirmDialog) g_pGenericConfirmDialog->Show(std::move(confirm));
        return;
    }
    if (data.Tab == 0 && data.Enabled && !row.Owned && row.BuyNow > 0)
    {
        GenericDialogConfig confirm;
        confirm.title = L"Confirmar compra";
        confirm.lines.push_back({row.Name + L" — " + std::to_wstring(row.BuyNow), false});
        confirm.primaryLabel = L"Comprar ahora";
        confirm.showCancel = true;
        confirm.cancelLabel = L"Volver";
        confirm.onPrimary = [row, page = data.Page, subTab = data.SubTab]
        {
            Network::Paris::Auctions::Request(3, 0, static_cast<BYTE>(page), static_cast<BYTE>(subTab), L"", 255, &row);
        };
        if (g_pGenericConfirmDialog) g_pGenericConfirmDialog->Show(std::move(confirm));
        return;
    }
    if (data.Tab == 0 && data.Enabled && !row.Owned && row.NextBid > 0)
    {
        PlaceBid(row);
    }
}

bool CParisAuctionWindow::ReadAmount(const Rml::String& value, int& amount) const
{
    if (value.empty()) return false;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto parsed = std::from_chars(begin, end, amount);
    return parsed.ec == std::errc{} && parsed.ptr == end && amount >= 0;
}

void CParisAuctionWindow::PublishItem(const Network::Paris::Auctions::Row& row)
{
    const auto& data = Network::Paris::Auctions::Data;
    auto& model = m_RmlView.GetModel();
    int price = 0;
    int buyNow = 0;
    if (!ReadAmount(model.priceInput, price) || price < 1 || !ReadAmount(model.buyNowInput, buyNow)
        || (buyNow != 0 && buyNow < price) || m_durationIndex >= static_cast<int>(data.Durations.size())
        || std::find(data.Currencies.begin(), data.Currencies.end(), m_currency) == data.Currencies.end())
        return;

    GenericDialogConfig confirm;
    confirm.title = L"Publicar objeto";
    confirm.lines.push_back({row.Name + L" — " + std::to_wstring(price), false});
    confirm.lines.push_back({buyNow > 0 ? std::wstring(L"Compra inmediata: ") + std::to_wstring(buyNow) : L"Sin compra inmediata", false});
    confirm.primaryLabel = L"Publicar";
    confirm.showCancel = true;
    confirm.cancelLabel = L"Volver";
    confirm.onPrimary = [row, price, buyNow, currency = m_currency, duration = data.Durations[static_cast<std::size_t>(m_durationIndex)], page = data.Page]
    {
        Network::Paris::Auctions::Request(1, 1, static_cast<BYTE>(page), 0, L"", 255, &row, price, buyNow, duration,
            static_cast<BYTE>(currency));
    };
    if (g_pGenericConfirmDialog) g_pGenericConfirmDialog->Show(std::move(confirm));
}

void CParisAuctionWindow::PlaceBid(const Network::Paris::Auctions::Row& row)
{
    GenericDialogConfig confirm;
    confirm.title = L"Confirmar oferta";
    confirm.lines.push_back({row.Name + L" — mínimo " + std::to_wstring(row.NextBid), false});
    GenericDialogConfig::InputField input; input.mode = GenericDialogConfig::InputField::Mode::Text;
    input.numericOnly = true; input.maxLength = 10; input.initialText = std::to_wstring(row.NextBid); confirm.input = input;
    confirm.primaryLabel = L"Ofertar"; confirm.showCancel = true; confirm.cancelLabel = L"Volver";
    confirm.onPrimary = [this, row, page = Network::Paris::Auctions::Data.Page, subTab = Network::Paris::Auctions::Data.SubTab]
    {
        if (!g_pGenericConfirmDialog) return;
        int amount = 0;
        if (!ReadAmount(StringUtils::WideToNarrow(g_pGenericConfirmDialog->GetInputText().c_str()), amount) || amount < row.NextBid) return;
        Network::Paris::Auctions::Request(2, 0, static_cast<BYTE>(page), static_cast<BYTE>(subTab), L"", 255, &row,
            amount, 0, 0, static_cast<BYTE>(row.Currency));
    };
    if (g_pGenericConfirmDialog) g_pGenericConfirmDialog->Show(std::move(confirm));
}
void CParisAuctionWindow::ClearItems()
{
    for (auto*& item : m_items) { if (item && g_pNewItemMng) g_pNewItemMng->DeleteItem(item); item = nullptr; }
}
void CParisAuctionWindow::SyncItems()
{
    if (m_itemsReceived != Network::Paris::Auctions::Received)
    {
        m_itemsReceived = Network::Paris::Auctions::Received; ClearItems();
        const auto& rows = Network::Paris::Auctions::Data.Rows;
        for (int i = 0; i < 16; ++i)
        {
            if (m_targets[i]) m_targets[i]->Invalidate();
            if (i < static_cast<int>(rows.size()) && rows[i].ItemData.size() >= 5 && g_pNewItemMng)
                m_items[i] = g_pNewItemMng->CreateItem(rows[i].ItemData);
        }
    }
    static thread_local Rml::ElementList rows; rows.clear(); m_RmlView.Document()->QuerySelectorAll(rows, ".auction-row");
    int hovered = -1;
    for (int i = 0; i < 16; ++i)
    {
        auto* row = i < static_cast<int>(rows.size()) && rows[i]->IsVisible() ? rows[i] : nullptr;
        if (m_targets[i]) m_targets[i]->Sync(row ? row->QuerySelector("img") : nullptr, row && m_items[i]);
        if (row && UI::RmlBridge::IsPointerWithin(row)) hovered = i;
    }
    if (hovered >= 0 && m_items[hovered]) { RenderItemInfoAtPx(g_fWindowMouseX, g_fWindowMouseY, m_items[hovered], false); m_tooltipShown = true; }
    else if (m_tooltipShown) { UI::Tooltip::HideLegacyTextList(); m_tooltipShown = false; }
}
void CParisAuctionWindow::DrawItem(int index, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    if (!m_items[index]) return;
    const auto& item = *m_items[index]; const Render::Items::Preview::ScopedFit fit(offset.x, offset.y, size.x, size.y);
    RenderItem3D(offset.x, offset.y, size.x, size.y, item.Type, item.Level, item.ExcellentFlags, item.AncientDiscriminator, false);
}
} // namespace mu::ui::window
