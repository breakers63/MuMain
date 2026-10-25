#include "stdafx.h"
#include "UI/Dialogs/ParisStorePriceWindow.h"
#include "GameLogic/Items/MarketPrice.h"
#include "Network/Paris/MarketProtocol.h"
#include "Render/Items/ItemPreviewFit.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/Inventory/ItemMng.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowCommon.h"
#include <RmlUi/Core/Event.h>
#include <array>
#include <limits>

namespace mu::ui::window
{
namespace { constexpr DWORD InterfaceId = 0x50535052u; }
CParisStorePriceWindow* g_pParisStorePriceWindow = nullptr;
CParisStorePriceWindow::~CParisStorePriceWindow() { Release(); }
bool CParisStorePriceWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}
void CParisStorePriceWindow::ClearItem()
{
    if (m_target) m_target->Disable();
    for (auto& target : m_currencyTargets) if (target) target->Disable();
    if (m_item && g_pNewItemMng) g_pNewItemMng->DeleteItem(m_item);
    m_item = nullptr;
}
void CParisStorePriceWindow::Release()
{
    // Scene teardown must not execute callbacks into inventories already released.
    m_apply = {}; m_cancel = {};
    ClearItem(); m_target.reset(); m_RmlView.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisStorePriceWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name); row.RegisterMember("amount", &Row::amount);
    row.RegisterMember("index", &Row::index); row.RegisterMember("enabled", &Row::enabled);
    c.RegisterArray<std::vector<Row>>(); c.Bind("rows", &model.rows); c.Bind("status", &model.status);
    c.BindEventCallback("price_adjust", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (args.size() == 2) Adjust(args[0].Get<int>(), args[1].Get<int>()); });
    c.BindEventCallback("price_apply", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Resolve(true); });
    c.BindEventCallback("price_cancel", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Resolve(false); });
}
bool CParisStorePriceWindow::OpenWindow(ITEM* item, std::function<void(std::uint8_t, int)> apply, std::function<void()> cancel)
{
    if (!item || !g_pNewItemMng || !m_RmlView.Ensure()) return false;
    if (IsVisible()) Resolve(false);
    ClearItem();
    m_item = g_pNewItemMng->DuplicateItem(item);
    if (!m_item) return false;
    if (!m_target) m_target = std::make_unique<UI::Items::ItemCameraTarget>(
        [this](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(offset, size); });
    constexpr int CurrencyItems[]{14 * MAX_ITEM_INDEX + 13, 14 * MAX_ITEM_INDEX + 14, 14 * MAX_ITEM_INDEX + 16, 14 * MAX_ITEM_INDEX + 22, 12 * MAX_ITEM_INDEX + 15};
    for (std::size_t i = 0; i < m_currencyTargets.size(); ++i)
        if (!m_currencyTargets[i]) m_currencyTargets[i] = std::make_unique<UI::Items::ItemCameraTarget>(
            [type = CurrencyItems[i]](const Rml::Vector2f& offset, const Rml::Vector2f& size)
            {
                RenderItem3D(offset.x, offset.y, size.x, size.y, type, 0, 0, 0, false);
            });
    m_apply = std::move(apply); m_cancel = std::move(cancel);
    m_extended = Network::Paris::Market::Data.supported && Network::Paris::Market::Data.currencyPricing;
    auto& model = m_RmlView.GetModel(); model.rows.clear(); model.status.clear();
    constexpr const char* Names[]{"Zen:", "Paris Coins:", "Bless:", "Soul:", "Life:", "Creation:", "Chaos:"};
    for (int i = 0; i < 7; ++i) model.rows.push_back({Names[i], "0", i, i == 0 || m_extended});
    m_RmlView.MarkDirty("rows"); m_RmlView.MarkDirty("status");
    Show(true);
    if (!Network::Paris::Market::Data.supported) Network::Paris::Market::RequestCapability();
    return true;
}
void CParisStorePriceWindow::Show(bool show)
{
    if (!show && IsVisible() && m_cancel) { Resolve(false); return; }
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else { m_RmlView.Document()->Hide(); if (m_target) m_target->Disable(); for (auto& target : m_currencyTargets) if (target) target->Disable(); }
}
bool CParisStorePriceWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { Resolve(false); return false; }
    return !IsVisible();
}
bool CParisStorePriceWindow::Update()
{
    if (!IsVisible()) return true;
    const bool extended = Network::Paris::Market::Data.supported && Network::Paris::Market::Data.currencyPricing;
    if (m_extended == extended) return true;
    m_extended = extended;
    auto& rows = m_RmlView.GetModel().rows;
    for (auto& row : rows) row.enabled = row.index == 0 || extended;
    m_RmlView.MarkDirty("rows");
    return true;
}
bool CParisStorePriceWindow::Render()
{
    if (IsVisible() && m_target && m_RmlView.Document())
        m_target->Sync(m_RmlView.Document()->GetElementById("price-item"), m_item != nullptr);
    if (IsVisible() && m_RmlView.Document())
    {
        static thread_local Rml::ElementList icons;
        icons.clear();
        m_RmlView.Document()->QuerySelectorAll(icons, ".price-row");
        std::erase_if(icons, [](auto* element) { return !element->IsVisible(); });
        for (std::size_t i = 0; i < m_currencyTargets.size(); ++i)
            if (m_currencyTargets[i]) m_currencyTargets[i]->Sync(i + 2 < icons.size() ? icons[i + 2]->QuerySelector(".price-jewel-icon") : nullptr, true);
    }
    return true;
}
void CParisStorePriceWindow::DrawItem(const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    if (!m_item) return;
    const Render::Items::Preview::ScopedFit fit(offset.x, offset.y, size.x, size.y);
    RenderItem3D(offset.x, offset.y, size.x, size.y, m_item->Type, m_item->Level, m_item->ExcellentFlags, m_item->AncientDiscriminator, false);
}
void CParisStorePriceWindow::Adjust(int index, int delta)
{
    auto& rows = m_RmlView.GetModel().rows;
    if (index < 0 || index >= static_cast<int>(rows.size()) || !rows[index].enabled) return;
    const auto amount = GameLogic::Items::MarketPrice::Amount(rows[index].amount);
    if (!amount) return;
    const long long value = std::clamp(static_cast<long long>(*amount) + delta, 0LL, static_cast<long long>(std::numeric_limits<int>::max()));
    rows[index].amount = std::to_string(value);
    m_RmlView.MarkDirty("rows");
}
void CParisStorePriceWindow::Resolve(bool apply)
{
    std::optional<GameLogic::Items::MarketPrice::Selection> price;
    if (apply)
    {
        std::array<std::string, 7> amounts;
        const auto& rows = m_RmlView.GetModel().rows;
        if (rows.size() != amounts.size()) return;
        for (std::size_t i = 0; i < amounts.size(); ++i) amounts[i] = rows[i].amount;
        const bool extended = m_extended && Network::Paris::Market::Data.supported && Network::Paris::Market::Data.currencyPricing;
        price = GameLogic::Items::MarketPrice::Select(amounts, extended);
        if (!price)
        {
            m_RmlView.GetModel().status = "Indica una cantidad válida en una sola moneda.";
            m_RmlView.MarkDirty("status"); return;
        }
    }
    auto onApply = std::move(m_apply); auto onCancel = std::move(m_cancel);
    m_apply = {}; m_cancel = {};
    Show(false); ClearItem();
    if (price && onApply) onApply(price->currency, price->amount);
    else if (onCancel) onCancel();
}
}
