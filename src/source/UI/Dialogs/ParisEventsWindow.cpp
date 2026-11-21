#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisEventsWindow.h"
#include "Core/Utilities/StringUtils.h"
#include "Network/Paris/ParisPacketDispatch.h"
#include "Network/Paris/ParisRequests.h"
#include "UI/Core/WindowManager.h"
#include "UI/RmlBridge/RmlDraggable.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "Engine/Object/ZzzCharacter.h"
#include <RmlUi/Core/Event.h>
#include <algorithm>
#include <cstdio>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50455654u;
constexpr DWORD RefreshMilliseconds = 5000, StaleMilliseconds = 15000;
constexpr int EventsPerPage = 9, MonstersPerPage = 3;
const std::pair<const char*, const char*> Categories[] = {
    {"all","Todos"},{"classic","Clásicos"},{"invasion","Invasiones"},
    {"boss","Jefes"},{"bonus","Bonos"},{"custom","Customs"}};
std::string Phase(const std::string& phase)
{
    if (phase == "open" || phase == "registration") return "Inscripciones abiertas";
    if (phase == "running") return "En curso";
    if (phase == "prepare") return "Preparando";
    if (phase == "disabled") return "Inactivo";
    if (phase == "available") return "Disponible";
    return phase == "idle" ? "Próximo" : "Sin información";
}
std::string Countdown(int seconds, DWORD at, bool fresh)
{
    if (seconds < 0 || !fresh) return "--:--:--";
    const int remaining = std::max(0, seconds - static_cast<int>((timeGetTime() - at) / 1000));
    char text[32]{};
    std::snprintf(text, sizeof(text), "%02d:%02d:%02d", remaining / 3600, remaining / 60 % 60, remaining % 60);
    return text;
}
}
CParisEventsWindow* g_pParisEventsWindow = nullptr;
CParisEventsWindow::~CParisEventsWindow() { Release(); }

bool CParisEventsWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_RmlView.SetAfterBuild([this]
    {
        auto* doc = m_RmlView.Document();
        auto* panel = doc->GetElementById("panel");
        auto* title = doc->GetElementById("title");
        if (panel && title) UI::RmlBridge::MakeDraggable(title, panel, nullptr,
            [panel] { UI::RmlBridge::KeepInsideWindow(panel); });
    });
    m_handler = Network::Paris::RegisterTextHandler([this](std::wstring_view text) { return Receive(text); });
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}
void CParisEventsWindow::Release()
{
    Network::Paris::UnregisterTextHandler(m_handler); m_handler = 0;
    DisablePreviews(); m_RmlView.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
bool CParisEventsWindow::Receive(std::wstring_view text)
{
    if (!text.starts_with(L"[PG]")) return false;
    std::lock_guard lock(m_mutex);
    if (auto snapshot = m_decoder.Receive(StringUtils::WideToNarrow(std::wstring(text).c_str()), timeGetTime()))
        m_received = std::move(snapshot);
    return true;
}

void CParisEventsWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("key", &Row::key); row.RegisterMember("name", &Row::name);
    row.RegisterMember("value", &Row::value); row.RegisterMember("state", &Row::state);
    row.RegisterMember("selected", &Row::selected);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("status", &model.status); c.Bind("title", &model.title);
    c.Bind("phase", &model.phase); c.Bind("countdown", &model.countdown);
    c.Bind("players", &model.players); c.Bind("boss", &model.boss);
    c.Bind("page", &model.page); c.Bind("pages", &model.pages);
    c.Bind("monster_page", &model.monsterPage); c.Bind("monster_pages", &model.monsterPages);
    c.Bind("empty", &model.empty); c.Bind("has_detail", &model.hasDetail); c.Bind("can_join", &model.canJoin);
    c.Bind("tabs", &model.tabs); c.Bind("events", &model.events);
    c.Bind("requirements", &model.requirements); c.Bind("monsters", &model.monsters); c.Bind("rewards", &model.rewards);
    BindActions(c);
}
void CParisEventsWindow::BindActions(Rml::DataModelConstructor& c)
{
    c.BindEventCallback("paris_event_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
    c.BindEventCallback("paris_event_join", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Request(true); });
    c.BindEventCallback("paris_event_select", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (args.empty() || timeGetTime() - m_lastRequest < 850) return;
        m_selected = args[0].Get<Rml::String>(); m_monsterPage = 0; Request();
    });
    c.BindEventCallback("paris_event_tab", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) { m_category = args[0].Get<Rml::String>(); m_page = 0; SyncModel(); } });
    c.BindEventCallback("paris_event_page", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) { m_page = std::clamp(m_page + args[0].Get<int>(), 0, m_RmlView.GetModel().pages - 1); SyncModel(); } });
    c.BindEventCallback("paris_monster_page", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) { m_monsterPage = std::clamp(m_monsterPage + args[0].Get<int>(), 0, m_RmlView.GetModel().monsterPages - 1); SyncModel(); } });
}
void CParisEventsWindow::Request(bool join)
{
    if (!Hero || timeGetTime() - m_lastRequest < 850) return;
    if (m_character != Hero->ID)
    {
        m_character = Hero->ID; m_snapshot.reset(); m_selected = "-";
        m_pending = false; m_monsterPage = 0; m_page = 0;
    }
    if (join && m_pending) return;
    if (join && (!m_snapshot || !m_snapshot->detail || !m_RmlView.GetModel().canJoin)) return;
    m_sequence = m_sequence % INT_MAX + 1;
    { std::lock_guard lock(m_mutex); m_received.reset(); m_decoder.Expect(m_sequence); }
    wchar_t command[120]{};
    const auto key = StringUtils::NarrowToWide(m_selected);
    swprintf_s(command, L"/eventos %u %ls%ls", m_sequence, join ? L"join:" : L"", key.c_str());
    if (Network::Paris::RequestJewelBank(Hero->ID, command)) { m_lastRequest = timeGetTime(); m_pending = true; }
    if (join) { m_RmlView.GetModel().canJoin = false; m_RmlView.MarkDirty("can_join"); }
}
void CParisEventsWindow::OpenWindow()
{
    if (!m_RmlView.Ensure()) return;
    Show(true);
}
void CParisEventsWindow::CloseWindow() { Show(false); }
void CParisEventsWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) { Request(); SyncModel(); m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document); }
    else { m_RmlView.Document()->Hide(); DisablePreviews(); }
}
bool CParisEventsWindow::UpdateMouseEvent() { return !IsVisible() || !UI::RmlBridge::IsPointerOver(m_RmlView.Document()); }
bool CParisEventsWindow::UpdateKeyEvent()
{
    if (IsVisible() && (IsPress(VK_ESCAPE) || IsPress('H'))) { CloseWindow(); return false; }
    return !IsVisible();
}
bool CParisEventsWindow::Update()
{
    if (!IsVisible()) return true;
    if (timeGetTime() - m_lastRequest >= (m_pending ? 10000 : RefreshMilliseconds)) Request();
    if (timeGetTime() >= m_nextSync) { m_nextSync = timeGetTime() + 250; SyncModel(); }
    return true;
}
bool CParisEventsWindow::Render()
{
    if (IsVisible()) SyncPreviews();
    return true;
}
void CParisEventsWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    { std::lock_guard lock(m_mutex); if (m_received) { m_snapshot = std::move(m_received); m_received.reset(); m_pending = false; if (m_snapshot->detail) m_selected = m_snapshot->detail->key; } }
    auto& model = m_RmlView.GetModel();
    const bool fresh = m_snapshot && timeGetTime() - m_snapshot->at < StaleMilliseconds;
    model.status = fresh ? "Datos en vivo · Requisitos de tu personaje" : "Esperando información del servidor";
    model.tabs.clear();
    for (const auto& [key, name] : Categories) model.tabs.push_back({key, name, "", -1, m_category == key});
    std::vector<const Network::Paris::EventGuide::Event*> events;
    if (m_snapshot) for (const auto& event : m_snapshot->events)
        if (m_category == "all" || event.category == m_category) events.push_back(&event);
    model.pages = std::max(1, static_cast<int>((events.size() + EventsPerPage - 1) / EventsPerPage));
    m_page = std::clamp(m_page, 0, model.pages - 1); model.page = m_page + 1;
    model.events.clear(); model.empty = events.empty();
    for (int i = m_page * EventsPerPage; i < std::min(static_cast<int>(events.size()), (m_page + 1) * EventsPerPage); ++i)
    {
        const auto& e = *events[i];
        const bool active = e.phase == "running" || e.phase == "open" || e.phase == "available" || e.phase == "registration";
        model.events.push_back({e.key, e.name, !fresh ? "Sin conexión" : e.seconds < 0 ? Phase(e.phase) : Countdown(e.seconds, m_snapshot->at, fresh), active ? 1 : -1, e.key == m_selected});
    }
    SyncDetail(model, fresh);
    for (const auto* field : {"status","tabs","pages","page","events","empty","has_detail","title","phase","countdown","players","boss","requirements","monsters","rewards","can_join","monster_page","monster_pages"}) m_RmlView.MarkDirty(field);
}
void CParisEventsWindow::SyncDetail(Model& model, bool fresh)
{
    model.hasDetail = m_snapshot && m_snapshot->detail.has_value();
    model.requirements.clear(); model.monsters.clear(); model.rewards.clear(); model.canJoin = false;
    if (!model.hasDetail) return;
    const auto& detail = *m_snapshot->detail;
    model.title = detail.name; model.phase = fresh ? Phase(detail.phase) : "Actualizando...";
    model.countdown = Countdown(detail.seconds, m_snapshot->at, fresh);
    model.players = (detail.players < 0 ? "--" : std::to_string(detail.players)) + " / " + (detail.maximum < 0 ? "--" : std::to_string(detail.maximum));
    model.canJoin = fresh && !m_pending && detail.key == m_selected && detail.canJoin;
    for (const auto& row : detail.requirements) model.requirements.push_back({"", row.label, row.value, row.state});
    model.monsterPages = std::max(1, static_cast<int>((detail.monsters.size() + MonstersPerPage - 1) / MonstersPerPage));
    m_monsterPage = std::clamp(m_monsterPage, 0, model.monsterPages - 1); model.monsterPage = m_monsterPage + 1;
    for (int i = m_monsterPage * MonstersPerPage; i < std::min(static_cast<int>(detail.monsters.size()), (m_monsterPage + 1) * MonstersPerPage); ++i)
        model.monsters.push_back({std::to_string(detail.monsters[i].id), detail.monsters[i].name, "", detail.monsters[i].boss ? 1 : -1});
    for (const auto& reward : detail.rewards) model.rewards.push_back({"", reward});
    if (model.rewards.empty()) model.rewards.push_back({"", "Sin recompensa fija publicada"});
    const auto boss = std::find_if(detail.monsters.begin(), detail.monsters.end(), [](const auto& monster) { return monster.boss; });
    model.boss = boss != detail.monsters.end() ? "JEFE: " + boss->name : detail.monsters.empty() ? "PARIS PROJECT" : detail.monsters[0].name;
}
void CParisEventsWindow::DisablePreviews()
{
    for (auto& preview : m_previews) if (preview) preview->Disable();
}
void CParisEventsWindow::SyncPreviews()
{
    auto* doc = m_RmlView.Document();
    if (!doc || !m_snapshot || !m_snapshot->detail) { DisablePreviews(); return; }
    const auto& monsters = m_snapshot->detail->monsters;
    m_previewCards.clear(); doc->QuerySelectorAll(m_previewCards, ".guide-monster-model");
    for (int slot = 0; slot < 3; ++slot)
    {
        const auto index = static_cast<std::size_t>(m_monsterPage * MonstersPerPage + slot);
        if (index >= monsters.size() || slot >= static_cast<int>(m_previewCards.size())) { if (m_previews[slot]) m_previews[slot]->Disable(); continue; }
        if (!m_previews[slot]) m_previews[slot] = std::make_unique<Render::MonsterPreviews::MonsterPreviewTarget>();
        m_previews[slot]->Sync(m_previewCards[slot], monsters[index].id);
    }
    if (monsters.empty()) { if (m_previews[3]) m_previews[3]->Disable(); return; }
    const auto boss = std::find_if(monsters.begin(), monsters.end(), [](const auto& monster) { return monster.boss; });
    if (!m_previews[3]) m_previews[3] = std::make_unique<Render::MonsterPreviews::MonsterPreviewTarget>();
    m_previews[3]->Sync(doc->GetElementById("guide-boss-model"), boss != monsters.end() ? boss->id : monsters[0].id);
}
} // namespace mu::ui::window

