#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisActivityLogWindow.h"
#include "Core/Utilities/StringUtils.h"
#include "GameLogic/Social/ActivityHistory.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/ItemMng.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/Tooltip/LegacyTextListTooltip.h"
#include "World/MapInfra/MapManager.h"
#include "Engine/Object/ZzzInventory.h"
#include "Render/Items/ItemPreviewFit.h"
#include <RmlUi/Core/Event.h>
#include <algorithm>
#include <cwctype>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50414354u;
constexpr int VisibleRows = 4;
Rml::String Text(const std::wstring& value) { return StringUtils::WideToNarrow(value.c_str()); }
std::wstring Lower(std::wstring text)
{
    for (auto& c : text) c = static_cast<wchar_t>(std::towlower(c));
    return text;
}
auto& History(bool kills) { return kills ? GameLogic::Social::ActivityHistory::Kills : GameLogic::Social::ActivityHistory::Posts; }
}
CParisActivityLogWindow* g_pParisActivityLogWindow = nullptr;
CParisActivityLogWindow::~CParisActivityLogWindow() { Release(); }
bool CParisActivityLogWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}
void CParisActivityLogWindow::ClearItems()
{
    if (m_tooltipShown) UI::Tooltip::HideLegacyTextList();
    m_tooltipShown = false;
    for (auto*& item : m_items)
    {
        if (item && g_pNewItemMng) g_pNewItemMng->DeleteItem(item);
        item = nullptr;
    }
}
void CParisActivityLogWindow::Release()
{
    ClearItems();
    for (auto& target : m_targets) if (target) target->Disable();
    m_RmlView.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisActivityLogWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("time", &Row::time); row.RegisterMember("actor", &Row::actor);
    row.RegisterMember("description", &Row::description); row.RegisterMember("detail", &Row::detail);
    row.RegisterMember("map", &Row::map); row.RegisterMember("winner", &Row::winner);
    row.RegisterMember("slot", &Row::slot); row.RegisterMember("map_tile", &Row::mapTile);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("title", &model.title); c.Bind("status", &model.status); c.Bind("rows", &model.rows);
    c.Bind("empty", &model.empty); c.Bind("kills", &model.kills); c.Bind("all", &model.all);
    c.Bind("first", &model.first); c.Bind("maximum", &model.maximum); c.Bind("count", &model.count);
    c.Bind("query", &model.query); c.Bind("thumb", &model.thumb);
    c.BindEventCallback("paris_activity_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
    c.BindEventCallback("activity_filter", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) { m_RmlView.GetModel().all = args[0].Get<bool>(); m_dirty = true; } });
    c.BindEventCallback("activity_step", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) Scroll(args[0].Get<int>()); });
    c.BindEventCallback("activity_page", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) Scroll(args[0].Get<int>() * VisibleRows); });
    c.BindEventCallback("activity_wheel", [this](Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList&)
    {
        const float delta = event.GetParameter<float>("wheel_delta_y", 0.f);
        if (delta != 0) Scroll(delta > 0 ? 1 : -1);
        event.StopPropagation();
    });
}
void CParisActivityLogWindow::ToggleWindow(bool kills)
{
    if (IsVisible() && m_kills == kills) CloseWindow();
    else OpenWindow(kills);
}

void CParisActivityLogWindow::OpenWindow(bool kills)
{
    if (!m_RmlView.Ensure()) return;
    m_kills = kills; m_dirty = true; m_revision = ~0ull; m_first = -1;
    auto& model = m_RmlView.GetModel();
    model.title = kills ? "KILL LOG" : "POST ITEM LOG";
    model.kills = kills; model.all = true; model.first = 0; model.query.clear();
    for (int i = 0; i < VisibleRows; ++i)
        if (!m_targets[i]) m_targets[i] = std::make_unique<UI::Items::ItemCameraTarget>(
            [this, i](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(i, offset, size); });
    for (const char* field : {"kills", "all", "first", "query", "title"}) m_RmlView.MarkDirty(field);
    Show(true); SyncModel();
}
void CParisActivityLogWindow::CloseWindow() { Show(false); }
void CParisActivityLogWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
    {
        m_RmlView.Document()->Hide();
        if (m_tooltipShown) UI::Tooltip::HideLegacyTextList();
        m_tooltipShown = false;
        for (auto& target : m_targets) if (target) target->Disable();
    }
}
bool CParisActivityLogWindow::UpdateMouseEvent() { return !IsVisible(); }
bool CParisActivityLogWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}
bool CParisActivityLogWindow::Update() { if (IsVisible()) SyncModel(); return true; }
bool CParisActivityLogWindow::Render()
{
    if (!IsVisible()) return true;
    SyncModel(); UpdateItems();
    return true;
}
void CParisActivityLogWindow::Scroll(int delta)
{
    auto& model = m_RmlView.GetModel();
    model.first = std::clamp(model.first + delta, 0, model.maximum);
    m_RmlView.MarkDirty("first");
}
void CParisActivityLogWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    auto& model = m_RmlView.GetModel();
    const auto& history = History(m_kills);
    if (m_dirty || m_revision != history.Revision() || m_query != model.query || m_all != model.all)
    {
        const bool changedFilter = m_query != model.query || m_all != model.all;
        m_dirty = false; m_revision = history.Revision(); m_query = model.query; m_all = model.all;
        if (changedFilter) model.first = 0;
        const auto query = Lower(StringUtils::NarrowToWide(model.query));
        m_filtered.clear();
        for (int i = 0; i < history.Count(); ++i)
        {
            const auto* record = history.At(i);
            if (!record || (m_kills && !model.all && record->Winner)) continue;
            const std::wstring map = m_kills && record->Map >= 0 ? gMapManager.GetMapName(record->Map) : L"";
            if (Lower(record->Actor + L" " + record->Description + L" " + map).find(query) != std::wstring::npos) m_filtered.push_back(i);
        }
        model.count = static_cast<int>(m_filtered.size()); model.empty = model.count == 0;
        model.maximum = std::max(0, model.count - VisibleRows);
        model.first = std::clamp(model.first, 0, model.maximum);
        model.thumb = std::to_string(std::max(11.1111, 100. * VisibleRows / std::max(VisibleRows, model.count))) + "%";
        for (const char* field : {"count", "empty", "maximum", "first", "all", "thumb"}) m_RmlView.MarkDirty(field);
        m_first = -1;
    }
    if (m_first != model.first) { m_first = model.first; BuildRows(); }
}
void CParisActivityLogWindow::BuildRows()
{
    ClearItems();
    auto& model = m_RmlView.GetModel(); model.rows.clear();
    const auto& history = History(m_kills);
    for (int slot = 0; slot < VisibleRows && m_first + slot < static_cast<int>(m_filtered.size()); ++slot)
    {
        const auto* record = history.At(m_filtered[m_first + slot]);
        if (!record) continue;
        const Rml::String map = record->Map >= 0 ? Text(gMapManager.GetMapName(record->Map)) : "Desconocido";
        model.rows.push_back({Text(record->Time), Text(record->Actor), Text(record->Description), Text(record->Detail), map, record->Winner, slot,
            record->Map == 2 ? 1 : record->Map == 3 ? 2 : record->Map == 1 ? 3 : 0});
        if (m_kills || !g_pNewItemMng || record->ItemType < 0 || record->ItemType >= MAX_ITEM) continue;
        ItemCreationParams params{};
        params.Group = record->ItemType / MAX_ITEM_INDEX; params.Number = record->ItemType % MAX_ITEM_INDEX;
        params.Level = static_cast<BYTE>(record->ItemLevel); params.Durability = 255;
        params.WithLuck = record->Luck != 0; params.WithOption = record->AdditionalOptionLevel > 0;
        params.OptionLevel = static_cast<BYTE>(std::clamp(record->AdditionalOptionLevel, 0, 15));
        params.HasExcellentOption = record->Excellent != 0; params.ExcellentFlags = static_cast<BYTE>(record->Excellent);
        params.IsAncient = record->Ancient != 0; params.AncientDiscriminator = static_cast<BYTE>(record->Ancient);
        std::fill(std::begin(params.SocketOptions), std::end(params.SocketOptions), SOCKET_EMPTY);
        m_items[slot] = g_pNewItemMng->CreateItemByParameters(&params);
    }
    model.status = std::to_string(model.empty ? 0 : model.first + 1) + " - " + std::to_string(std::min(model.first + VisibleRows, model.count)) + " de " + std::to_string(model.count) + " registros";
    m_RmlView.MarkDirty("rows"); m_RmlView.MarkDirty("status");
}
void CParisActivityLogWindow::UpdateItems()
{
    auto* document = m_RmlView.Document();
    if (!document) return;
    static thread_local Rml::ElementList rows;
    rows.clear();
    document->QuerySelectorAll(rows, ".activity-row");
    std::erase_if(rows, [](auto* element) { return !element->IsVisible(); });
    int hovered = -1;
    for (int slot = 0; slot < VisibleRows; ++slot)
    {
        auto* row = slot < static_cast<int>(rows.size()) ? rows[slot] : nullptr;
        auto* icon = row ? row->QuerySelector(".activity-item") : nullptr;
        // Sync owns visibility while a texture is being prepared. A temporarily hidden
        // image must keep receiving Sync calls or it can never become visible again.
        if (m_targets[slot]) m_targets[slot]->Sync(icon, !m_kills && m_items[slot]);
        if (!m_kills && slot < static_cast<int>(rows.size()) && UI::RmlBridge::IsPointerWithin(rows[slot])) hovered = slot;
    }
    if (hovered >= 0 && m_items[hovered])
    { RenderItemInfoAtPx(g_fWindowMouseX, g_fWindowMouseY, m_items[hovered], false); m_tooltipShown = true; }
    else if (m_tooltipShown) { UI::Tooltip::HideLegacyTextList(); m_tooltipShown = false; }
}
void CParisActivityLogWindow::DrawItem(int row, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    if (row < 0 || row >= VisibleRows || !m_items[row]) return;
    const auto& item = *m_items[row];
    const Render::Items::Preview::ScopedFit fit(offset.x, offset.y, size.x, size.y);
    RenderItem3D(offset.x, offset.y, size.x, size.y, item.Type, item.Level, item.ExcellentFlags, item.AncientDiscriminator, false);
}
}
