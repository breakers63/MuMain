#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "Render/Items/ItemPreviewFit.h"
#include "UI/Dialogs/ParisMarketWindow.h"

#include "Network/Paris/ParisPacketDispatch.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/ItemMng.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Dialogs/GenericConfirmDialog.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/Tooltip/LegacyTextListTooltip.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <string>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x504D4B54u;
constexpr int PageSize = 5;

Rml::String FormatAmount(std::int64_t amount)
{
    Rml::String text = std::to_string(amount);
    constexpr int GroupDigits = 3;
    for (int position = static_cast<int>(text.size()) - GroupDigits; position > 0; position -= GroupDigits)
        text.insert(position, ",");
    return text;
}

Rml::String CategoryName(int type)
{
    constexpr const char* Names[]{"Espadas", "Hachas", "Mazas / cetros", "Lanzas", "Arcos", "Bastones", "Escudos", "Cascos", "Armaduras", "Pantalones", "Guantes", "Botas", "Alas / orbes", "Accesorios", "Joyas", "Scrolls"};
    const int group = type / MAX_ITEM_INDEX;
    return group >= 0 && group < static_cast<int>(std::size(Names)) ? Names[group] : "Articulo";
}

Rml::String CurrencyName(std::uint8_t currency)
{
    switch (currency)
    {
    case 0: return "Zen";
    case 1: return "Paris Coins";
    case 2: return "Jewel of Bless";
    case 3: return "Jewel of Soul";
    case 4: return "Jewel of Life";
    case 5: return "Jewel of Creation";
    case 9: return "Jewel of Chaos";
    default: return "Moneda";
    }
}
} // namespace

CParisMarketWindow* g_pParisMarketWindow = nullptr;

CParisMarketWindow::~CParisMarketWindow()
{
    Release();
}

bool CParisMarketWindow::Create(CManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    m_capabilityHandler = Network::Paris::RegisterHandler(0xFA, 0xE2,
        [this](const std::uint8_t* packet, int size) { ReceivePacket(packet, size); });
    m_resultsHandler = Network::Paris::RegisterHandler(0xFA, 0xE5,
        [this](const std::uint8_t* packet, int size) { ReceivePacket(packet, size); });
    m_buyHandler = Network::Paris::RegisterHandler(0xFA, 0xE7,
        [this](const std::uint8_t* packet, int size) { ReceivePacket(packet, size); });
    m_errorHandler = Network::Paris::RegisterHandler(0xFA, 0xEB,
        [this](const std::uint8_t* packet, int size) { ReceivePacket(packet, size); });
    m_priceHandler = Network::Paris::RegisterHandler(0xFA, 0xE9,
        [](const std::uint8_t* packet, int size) { Network::Paris::Market::Receive(packet, size); });
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisMarketWindow::Release()
{
    Network::Paris::UnregisterHandler(m_capabilityHandler);
    Network::Paris::UnregisterHandler(m_resultsHandler);
    Network::Paris::UnregisterHandler(m_buyHandler);
    Network::Paris::UnregisterHandler(m_errorHandler);
    Network::Paris::UnregisterHandler(m_priceHandler);
    m_priceHandler = 0;
    m_capabilityHandler = m_resultsHandler = m_buyHandler = m_errorHandler = 0;
    for (auto& target : m_itemTargets) target->Disable();
    ClearTooltipItems();
    m_RmlView.Release();
    m_itemTargets.clear();
    if (m_manager)
    {
        m_manager->RemoveUIObj(this);
        m_manager = nullptr;
    }
}

void CParisMarketWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("seller", &Row::seller);
    row.RegisterMember("name", &Row::name);
    row.RegisterMember("price", &Row::price);
    row.RegisterMember("currency", &Row::currency);
    row.RegisterMember("index", &Row::index);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("status", &model.status);
    c.Bind("query", &model.query);
    c.Bind("category", &model.category);
    c.Bind("currency", &model.currency);
    c.Bind("sort", &model.sort);
    c.Bind("selected", &model.selected);
    c.Bind("has_selection", &model.hasSelection);
    c.Bind("selected_name", &model.selectedName);
    c.Bind("selected_seller", &model.selectedSeller);
    c.Bind("selected_price", &model.selectedPrice);
    c.Bind("selected_detail", &model.selectedDetail);
    c.Bind("seller_initial", &model.sellerInitial);
    c.Bind("listings", &model.listings);
    c.Bind("empty", &model.empty);
    c.Bind("offer_count", &model.offerCount);
    c.Bind("page", &model.page);
    c.Bind("page_count", &model.pageCount);
    BindActions(c);
}

void CParisMarketWindow::BindActions(Rml::DataModelConstructor& c)
{
    c.BindEventCallback("paris_market_filter",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { m_filtersDirty = true; });
    c.BindEventCallback("paris_market_refresh",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RequestRefresh(); });
    c.BindEventCallback("paris_market_prev",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            const auto& data = Network::Paris::Market::Data;
            if (data.page > 0)
                RequestPage(data.page - 1);
        });
    c.BindEventCallback("paris_market_next",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            const auto& data = Network::Paris::Market::Data;
            const int pageCount = std::max(1, (data.total + PageSize - 1) / PageSize);
            if (data.page + 1 < pageCount)
                RequestPage(data.page + 1);
        });
    c.BindEventCallback("paris_market_select",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        { if (!args.empty()) SelectListing(args[0].Get<int>(-1)); });
    c.BindEventCallback("paris_market_selected_buy",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        { RequestBuy(m_RmlView.GetModel().selected); });
    c.BindEventCallback("paris_market_publish",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            CloseWindow();
            if (g_pNewUISystem) g_pNewUISystem->Show(INTERFACE_MYSHOP_INVENTORY);
        });
    c.BindEventCallback("paris_market_buy",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            if (!args.empty())
                RequestBuy(args[0].Get<int>(-1));
        });
    c.BindEventCallback("paris_market_close",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisMarketWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document())
        return;
    if (show)
        m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
    {
        m_RmlView.Document()->Hide();
        if (m_tooltipShown) UI::Tooltip::HideLegacyTextList();
        m_tooltipShown = false;
        for (auto& target : m_itemTargets) target->Disable();
    }
}

void CParisMarketWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure())
        return;
    if (m_itemTargets.empty())
        for (std::size_t index = 0; index <= PageSize; ++index)
            m_itemTargets.push_back(std::make_unique<UI::Items::ItemCameraTarget>(
                [this, index](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(index, offset, size); }));
    m_rowsDirty = true;
    Show(true);
    Network::Paris::Market::RequestCapability();
}

void CParisMarketWindow::CloseWindow()
{
    Show(false);
}

bool CParisMarketWindow::UpdateMouseEvent()
{
    return !IsVisible();
}

bool CParisMarketWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE))
    {
        CloseWindow();
        return false;
    }
    return !IsVisible();
}

bool CParisMarketWindow::Update()
{
    return true;
}

bool CParisMarketWindow::Render()
{
    if (!IsVisible()) return true;
    if (m_filtersDirty)
    {
        m_filtersDirty = false;
        RequestRefresh();
    }
    SyncModel();
    UpdateTooltip();
    auto* document = m_RmlView.Document();
    if (!document) return true;
    static thread_local Rml::ElementList icons;
    icons.clear();
    document->QuerySelectorAll(icons, ".market-row");
    std::erase_if(icons, [](auto* element) { return !element->IsVisible(); });
    for (std::size_t index = 0; index < m_itemTargets.size(); ++index)
    {
        const bool preview = index == PageSize;
        auto* image = preview ? document->GetElementById("market-preview-item") : index < icons.size() ? icons[index]->QuerySelector(".market-item-icon") : nullptr;
        const int row = preview ? m_RmlView.GetModel().selected : static_cast<int>(index);
        m_itemTargets[index]->Sync(image, row >= 0 && row < static_cast<int>(m_previews.size()) && m_previews[row].type >= 0);
    }
    return true;
}

void CParisMarketWindow::ClearTooltipItems()
{
    if (m_tooltipShown) UI::Tooltip::HideLegacyTextList();
    m_tooltipShown = false;
    if (g_pNewItemMng)
        for (auto* item : m_tooltipItems) if (item) g_pNewItemMng->DeleteItem(item);
    m_tooltipItems.clear();
}

void CParisMarketWindow::UpdateTooltip()
{
    auto* document = m_RmlView.Document();
    int index = -1;
    if (document)
    {
        static thread_local Rml::ElementList rows;
        rows.clear();
        document->QuerySelectorAll(rows, ".market-row");
        for (auto* row : rows)
            if (UI::RmlBridge::IsPointerOver(row)) index = row->GetAttribute<int>("data-index", -1);
        if (UI::RmlBridge::IsPointerWithin(document->GetElementById("market-preview-item")))
            index = m_RmlView.GetModel().selected;
    }
    if (index >= 0 && index < static_cast<int>(m_tooltipItems.size()) && m_tooltipItems[index])
    {
        RenderItemInfoAtPx(g_fWindowMouseX, g_fWindowMouseY, m_tooltipItems[index], false);
        m_tooltipShown = true;
    }
    else if (m_tooltipShown)
    {
        UI::Tooltip::HideLegacyTextList();
        m_tooltipShown = false;
    }
}

void CParisMarketWindow::DrawItem(std::size_t index, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    const int row = index == PageSize ? m_RmlView.GetModel().selected : static_cast<int>(index);
    if (row < 0 || row >= static_cast<int>(m_previews.size())) return;
    const auto& item = m_previews[row];
    const Render::Items::Preview::ScopedFit fit(offset.x, offset.y, size.x, size.y);
    if (item.type >= 0) RenderItem3D(offset.x, offset.y, size.x, size.y, item.type, item.level, item.excellent, item.ancient, false);
}

void CParisMarketWindow::SelectListing(int index)
{
    auto& model = m_RmlView.GetModel();
    model.selected = index >= 0 && index < static_cast<int>(model.listings.size()) ? index : -1;
    model.hasSelection = model.selected >= 0;
    model.selectedName = model.hasSelection ? model.listings[index].name : "";
    model.selectedSeller = model.hasSelection ? model.listings[index].seller : "";
    model.selectedPrice = model.hasSelection ? model.listings[index].price + " " + model.listings[index].currency : "";
    model.selectedDetail.clear();
    model.sellerInitial.clear();
    if (model.hasSelection && index < static_cast<int>(m_previews.size()))
    {
        const auto& preview = m_previews[index];
        model.selectedDetail = CategoryName(preview.type) + " - +" + std::to_string(preview.level);
        const auto& seller = Network::Paris::Market::Data.listings[index].seller;
        model.sellerInitial = seller.empty() ? "?" : StringUtils::WideToNarrow(seller.substr(0, 1).c_str());
    }
    for (const char* field : {"selected", "has_selection", "selected_name", "selected_seller", "selected_price", "selected_detail", "seller_initial"}) m_RmlView.MarkDirty(field);
}

void CParisMarketWindow::SyncModel()
{
    if (!m_RmlView.Document())
        return;
    const auto& data = Network::Paris::Market::Data;
    auto& model = m_RmlView.GetModel();
    const Rml::String status = StringUtils::WideToNarrow(data.status.c_str());
    if (model.status != status)
    {
        model.status = status;
        m_RmlView.MarkDirty("status");
    }
    if (!m_rowsDirty) return;
    m_rowsDirty = false;
    std::vector<Row> rows;
    m_previews.clear();
    ClearTooltipItems();
    rows.reserve(data.listings.size());
    for (int i = 0; i < static_cast<int>(data.listings.size()); ++i)
    {
        const auto& listing = data.listings[i];
        const std::wstring seller = listing.seller.empty() ? L"Vendedor" : listing.seller;
        ItemPreview preview;
        Rml::String name = "Articulo";
        if (!listing.itemData.empty())
        {
            const auto item = ParseItemData(listing.itemData);
            const int type = item.Group * MAX_ITEM_INDEX + item.Number;
            if (type >= 0 && type < MAX_ITEM)
            {
                preview = {type, item.Level, item.ExcellentFlags, item.AncientDiscriminator};
                name = StringUtils::WideToNarrow(ItemAttribute[type].Name) + " +" + std::to_string(item.Level);
            }
        }
        m_previews.push_back(preview);
        m_tooltipItems.push_back(g_pNewItemMng && !listing.itemData.empty() ? g_pNewItemMng->CreateItem(listing.itemData) : nullptr);
        rows.push_back({StringUtils::WideToNarrow(seller.c_str()), name, FormatAmount(listing.amount),
            CurrencyName(listing.currency), i});
    }
    if (model.listings != rows)
    {
        model.listings = std::move(rows);
        m_RmlView.MarkDirty("listings");
    }
    model.offerCount = data.total;
    m_RmlView.MarkDirty("offer_count");
    SelectListing(-1);
    const bool empty = model.listings.empty();
    if (model.empty != empty)
    {
        model.empty = empty;
        m_RmlView.MarkDirty("empty");
    }
    const int pageCount = std::max(1, (data.total + PageSize - 1) / PageSize);
    const int page = static_cast<int>(data.page) + 1;
    if (model.page != page)
    {
        model.page = page;
        m_RmlView.MarkDirty("page");
    }
    if (model.pageCount != pageCount)
    {
        model.pageCount = pageCount;
        m_RmlView.MarkDirty("page_count");
    }
}

void CParisMarketWindow::RequestRefresh()
{
    if (Network::Paris::Market::Data.supported)
        RequestPage(0);
    else
        Network::Paris::Market::RequestCapability();
}

void CParisMarketWindow::RequestPage(int page)
{
    const auto& model = m_RmlView.GetModel();
    if (page < 0 || page > 65535) return;
    Network::Paris::Market::RequestSearch(static_cast<std::uint16_t>(page),
        static_cast<std::uint8_t>(model.category), static_cast<std::uint8_t>(model.currency),
        StringUtils::NarrowToWide(model.query), static_cast<std::uint8_t>(model.sort));
}

void CParisMarketWindow::RequestBuy(int index)
{
    const auto& listings = Network::Paris::Market::Data.listings;
    if (index < 0 || index >= static_cast<int>(listings.size()) || !g_pGenericConfirmDialog)
        return;
    const auto listing = listings[static_cast<std::size_t>(index)];
    GenericDialogConfig confirm;
    confirm.title = L"Confirmar compra";
    confirm.lines.push_back({L"El servidor volverá a validar la publicación y el saldo antes de comprar.", false});
    confirm.primaryLabel = L"Comprar";
    confirm.showCancel = true;
    confirm.cancelLabel = L"Cancelar";
    confirm.onPrimary = [listing]
    {
        Network::Paris::Market::RequestBuy(listing);
    };
    g_pGenericConfirmDialog->Show(std::move(confirm));
}

bool CParisMarketWindow::ReceivePacket(const std::uint8_t* packet, int size)
{
    const bool received = Network::Paris::Market::Receive(packet, size);
    if (received)
    {
        m_rowsDirty = true;
        const auto& model = m_RmlView.GetModel();
        if (size >= 4 && packet[3] == 0xE2 && Network::Paris::Market::Data.supported
            && (model.category != 255 || model.currency != 255 || model.sort != 0 || !model.query.empty()))
            m_filtersDirty = true;
    }
    return received;
}
} // namespace mu::ui::window
