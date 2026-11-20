#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisEventsWindow.h"
#include "UI/Events/ParisEventPages.h"
#include "UI/Events/ParisEventPortraits.h"
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
constexpr int MonstersPerPage = 3;
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
void MirrorBossPreview(Rml::ElementDocument* document)
{
    auto* primary = document->GetElementById("guide-boss-model");
    auto* secondary = document->GetElementById("guide-secondary-boss-model");
    if (!secondary) return;
    const auto source = primary && primary->IsVisible() ? primary->GetAttribute<Rml::String>("src", "") : Rml::String{};
    if (source.empty()) { secondary->SetProperty("visibility", "hidden"); return; }
    if (secondary->GetAttribute<Rml::String>("src", "") != source) secondary->SetAttribute("src", source);
    secondary->SetProperty("visibility", "visible");
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
    row.RegisterMember("selected", &Row::selected); row.RegisterMember("portrait", &Row::portrait);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("status", &model.status); c.Bind("title", &model.title);
    c.Bind("phase", &model.phase); c.Bind("countdown", &model.countdown);
    c.Bind("players", &model.players); c.Bind("boss", &model.boss);
    c.Bind("entries", &model.entries); c.Bind("category", &model.category); c.Bind("requirement_split", &model.requirementSplit);
    c.Bind("page", &model.page); c.Bind("pages", &model.pages);
    c.Bind("monster_page", &model.monsterPage); c.Bind("monster_pages", &model.monsterPages);
    c.Bind("empty", &model.empty); c.Bind("has_detail", &model.hasDetail); c.Bind("can_join", &model.canJoin);
    c.Bind("events", &model.events);
    c.Bind("requirements", &model.requirements); c.Bind("monsters", &model.monsters); c.Bind("rewards", &model.rewards);
    BindActions(c);
}
void CParisEventsWindow::BindActions(Rml::DataModelConstructor& c)
{
    c.BindEventCallback("paris_event_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
    c.BindEventCallback("paris_event_join", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Request(true); });
    c.BindEventCallback("paris_event_select", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (args.empty()) return;
        m_selected = args[0].Get<Rml::String>(); m_monsterPage = 0;
        m_needsRequest = true; Request(); SyncModel();
    });
    c.BindEventCallback("paris_event_page", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) ChangePage(args[0].Get<int>()); });
    c.BindEventCallback("paris_monster_page", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) { m_monsterPage = std::clamp(m_monsterPage + args[0].Get<int>(), 0, m_RmlView.GetModel().monsterPages - 1); SyncModel(); } });
}
void CParisEventsWindow::ChangePage(int delta)
{
    if (!m_snapshot) return;
    const auto pages = UI::Events::BuildGuidePages(m_snapshot->events);
    if (pages.empty()) return;
    m_page = (m_page + (delta < 0 ? -1 : 1) + static_cast<int>(pages.size())) % static_cast<int>(pages.size());
    m_selected = pages[m_page].events.front()->key;
    m_monsterPage = 0;
    m_needsRequest = true;
    Request();
    SyncModel();
}
void CParisEventsWindow::SyncEventPortraits()
{
    auto* doc = m_RmlView.Document();
    if (!doc) return;
    m_portraitElements.clear();
    doc->QuerySelectorAll(m_portraitElements, ".guide-event-model");
    const auto& rows = m_RmlView.GetModel().events;
    for (std::size_t slot = 0; slot < m_rowPreviews.size(); ++slot)
    {
        if (slot >= rows.size() || slot >= m_portraitElements.size() || rows[slot].portrait < 0)
        {
            if (m_rowPreviews[slot]) m_rowPreviews[slot]->Disable();
            continue;
        }
        if (!m_rowPreviews[slot]) m_rowPreviews[slot] = std::make_unique<Render::MonsterPreviews::MonsterPreviewTarget>(true);
        m_rowPreviews[slot]->Sync(m_portraitElements[slot], rows[slot].portrait);
    }
}
void CParisEventsWindow::Request(bool join)
{
    if (!Hero || timeGetTime() - m_lastRequest < 850) return;
    if (m_character != Hero->ID)
    {
        m_character = Hero->ID; m_snapshot.reset(); m_selected = "-"; m_portraitCache.clear();
        m_pending = false; m_monsterPage = 0; m_page = 0;
    }
    if (join && (m_pending || m_needsRequest)) return;
    if (join && (!m_snapshot || !m_snapshot->detail || !m_RmlView.GetModel().canJoin)) return;
    m_sequence = m_sequence % INT_MAX + 1;
    { std::lock_guard lock(m_mutex); m_received.reset(); m_decoder.Expect(m_sequence); }
    wchar_t command[120]{};
    const auto key = StringUtils::NarrowToWide(m_selected);
    swprintf_s(command, L"/eventos %u %ls%ls", m_sequence, join ? L"join:" : L"", key.c_str());
    if (Network::Paris::RequestJewelBank(Hero->ID, command)) { m_lastRequest = timeGetTime(); m_pending = true; m_needsRequest = false; }
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
    if (m_needsRequest || timeGetTime() - m_lastRequest >= (m_pending ? 10000 : RefreshMilliseconds)) Request();
    if (timeGetTime() >= m_nextSync) { m_nextSync = timeGetTime() + 250; SyncModel(); }
    return true;
}
bool CParisEventsWindow::Render()
{
    if (IsVisible()) { SyncEventPortraits(); SyncPreviews(); }
    return true;
}
void CParisEventsWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    { std::lock_guard lock(m_mutex); if (m_received) { m_snapshot = std::move(m_received); m_received.reset(); m_pending = false; if (m_snapshot->detail && m_selected == "-") m_selected = m_snapshot->detail->key; } }
    auto& model = m_RmlView.GetModel();
    const bool fresh = m_snapshot && timeGetTime() - m_snapshot->at < StaleMilliseconds;
    model.status = fresh ? "Datos en vivo · Requisitos de tu personaje" : "Esperando información del servidor";
    const auto pages = m_snapshot ? UI::Events::BuildGuidePages(m_snapshot->events) : std::vector<UI::Events::GuidePage>{};
    model.pages = std::max(1, static_cast<int>(pages.size()));
    m_page = std::clamp(m_page, 0, model.pages - 1); model.page = m_page + 1;
    model.category = pages.empty() ? "EVENTOS" : UI::Events::GuideCategoryName(pages[m_page].category);
    model.events.clear(); model.empty = pages.empty();
    if (!pages.empty()) for (const auto* event : pages[m_page].events)
    {
        const auto& e = *event;
        const bool active = e.phase == "running" || e.phase == "open" || e.phase == "available" || e.phase == "registration";
        const int portrait = m_portraitCache.contains(e.key) ? m_portraitCache.at(e.key) : UI::Events::GuidePortrait(e.key);
        model.events.push_back({e.key, UI::Events::GuideEventName(e), !fresh ? "Sin conexión" : e.seconds < 0 ? Phase(e.phase) : Countdown(e.seconds, m_snapshot->at, fresh), active ? 1 : -1, e.key == m_selected, portrait});
    }
    SyncDetail(model, fresh);
    for (const auto* field : {"status","category","entries","requirement_split","pages","page","events","empty","has_detail","title","phase","countdown","players","boss","requirements","monsters","rewards","can_join","monster_page","monster_pages"}) m_RmlView.MarkDirty(field);
}
void CParisEventsWindow::SyncDetail(Model& model, bool fresh)
{
    model.hasDetail = m_snapshot && m_snapshot->detail.has_value() && m_snapshot->detail->key == m_selected;
    model.requirements.clear(); model.monsters.clear(); model.rewards.clear(); model.canJoin = false;
    if (!model.hasDetail) return;
    const auto& detail = *m_snapshot->detail;
    model.requirementSplit = static_cast<int>((detail.requirements.size() + 1) / 2);
    model.entries = "--";
    for (const auto& row : detail.requirements)
        if (row.label == "Entradas restantes" || row.label == "Remaining entries" || row.label == "Entradas diarias") model.entries = row.value;
    model.title = UI::Events::GuideEventName(detail); model.phase = fresh ? Phase(detail.phase) : "Actualizando...";
    model.countdown = Countdown(detail.seconds, m_snapshot->at, fresh);
    model.players = (detail.players < 0 ? "--" : std::to_string(detail.players)) + " / " + (detail.maximum < 0 ? "--" : std::to_string(detail.maximum));
    model.canJoin = fresh && !m_pending && !m_needsRequest && detail.key == m_selected && detail.canJoin;
    for (const auto& row : detail.requirements) model.requirements.push_back({"", row.label, row.value, row.state});
    model.monsterPages = std::max(1, static_cast<int>((detail.monsters.size() + MonstersPerPage - 1) / MonstersPerPage));
    m_monsterPage = std::clamp(m_monsterPage, 0, model.monsterPages - 1); model.monsterPage = m_monsterPage + 1;
    for (int i = m_monsterPage * MonstersPerPage; i < std::min(static_cast<int>(detail.monsters.size()), (m_monsterPage + 1) * MonstersPerPage); ++i)
        model.monsters.push_back({std::to_string(detail.monsters[i].id), detail.monsters[i].name, "", detail.monsters[i].boss ? 1 : -1});
    for (const auto& reward : detail.rewards) model.rewards.push_back({"", reward});
    if (model.rewards.empty()) model.rewards.push_back({"", "Sin recompensa fija publicada"});
    const auto boss = std::find_if(detail.monsters.begin(), detail.monsters.end(), [](const auto& monster) { return monster.boss; });
    if (!detail.monsters.empty()) m_portraitCache[detail.key] = boss != detail.monsters.end() ? boss->id : detail.monsters[0].id;
    model.boss = boss != detail.monsters.end() ? "JEFE: " + boss->name : detail.monsters.empty() ? "PARIS PROJECT" : detail.monsters[0].name;
}
void CParisEventsWindow::DisablePreviews()
{
    for (auto& preview : m_previews) if (preview) preview->Disable();
    for (auto& preview : m_rowPreviews) if (preview) preview->Disable();
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
    MirrorBossPreview(doc);
}
} // namespace mu::ui::window

