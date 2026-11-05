#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisEventsWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "Network/Paris/EventTimerState.h"
#include "UI/Core/WindowManager.h"
#include "UI/RmlBridge/RmlDraggable.h"
#include <cstdio>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50455654u;
struct EventInfo { int id; int page; const wchar_t* name; };
constexpr EventInfo Events[] = {
    {1, 0, L"Chaos Castle"}, {2, 0, L"Devil Square"}, {3, 0, L"Blood Castle"},
    {4, 1, L"Golden Invasion"}, {5, 1, L"Red Dragon Invasion"}, {6, 1, L"White Wizard"},
    {9, 2, L"Happy Hour"}, {17, 2, L"Box of Fortune"},
    {10, 3, L"Bali Invasion"}, {11, 3, L"Scorpion Invasion"}, {12, 3, L"Strange Rabbit"},
    {13, 3, L"Grizzly"}, {7, 4, L"Erohim"}, {8, 4, L"Lord of Kundun"},
    {15, 4, L"Medusa"}, {16, 4, L"Gaion the Gladiator"},
};
constexpr const wchar_t* PageNames[] = {
    L"Eventos clásicos", L"Invasiones", L"Bonificaciones", L"Mini invasiones", L"Jefes"
};
}

CParisEventsWindow* g_pParisEventsWindow = nullptr;

CParisEventsWindow::~CParisEventsWindow() { Release(); }

bool CParisEventsWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_RmlView.SetAfterBuild([this]
    {
        auto* document = m_RmlView.Document();
        auto* panel = document->GetElementById("panel");
        auto* title = document->GetElementById("title");
        if (panel && title)
            UI::RmlBridge::MakeDraggable(title, panel, nullptr,
                [panel] { UI::RmlBridge::KeepInsideWindow(panel); });
    });
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisEventsWindow::Release()
{
    m_RmlView.Release();
    if (!m_manager) return;
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CParisEventsWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto tab = c.RegisterStruct<Tab>();
    tab.RegisterMember("label", &Tab::label);
    tab.RegisterMember("index", &Tab::index);
    tab.RegisterMember("selected", &Tab::selected);
    auto event = c.RegisterStruct<EventRow>();
    event.RegisterMember("name", &EventRow::name);
    event.RegisterMember("state", &EventRow::state);
    event.RegisterMember("remaining", &EventRow::remaining);
    event.RegisterMember("index", &EventRow::index);
    event.RegisterMember("seconds", &EventRow::seconds);
    event.RegisterMember("active", &EventRow::active);
    c.RegisterArray<std::vector<Tab>>();
    c.RegisterArray<std::vector<EventRow>>();
    c.Bind("status", &model.status);
    c.Bind("title", &model.title);
    c.Bind("page", &model.page);
    c.Bind("tabs", &model.tabs);
    c.Bind("events", &model.events);
    c.Bind("empty", &model.empty);
    c.BindEventCallback("paris_event_tab", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (!args.empty()) SelectTab(args[0].Get<int>(-1));
    });
    c.BindEventCallback("paris_event_page", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (!args.empty()) SelectTab((m_selectedTab + args[0].Get<int>() + 5) % 5); });
    c.BindEventCallback("paris_event_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisEventsWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    Show(true);
}

void CParisEventsWindow::CloseWindow() { Show(false); }

void CParisEventsWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else m_RmlView.Document()->Hide();
}

bool CParisEventsWindow::UpdateMouseEvent() { return !IsVisible(); }
bool CParisEventsWindow::UpdateKeyEvent()
{
    if (IsVisible() && (IsPress(VK_ESCAPE) || IsPress('H'))) { CloseWindow(); return false; }
    return !IsVisible();
}
bool CParisEventsWindow::Update() { return true; }
bool CParisEventsWindow::Render()
{
    if (IsVisible() && timeGetTime() >= m_nextSync) { m_nextSync = timeGetTime() + 200; SyncModel(); }
    return true;
}

void CParisEventsWindow::SelectTab(int tab)
{
    if (tab < 0 || tab >= static_cast<int>(std::size(PageNames))) return;
    auto& model = m_RmlView.GetModel();
    m_selectedTab = tab;
    for (auto& entry : model.tabs)
    {
        const bool selected = entry.index == tab;
        if (entry.selected != selected) { entry.selected = selected; m_RmlView.MarkDirty("tabs"); }
    }
    SyncModel();
}

void CParisEventsWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    auto& model = m_RmlView.GetModel();
    if (model.tabs.empty())
        for (int i = 0; i < static_cast<int>(std::size(PageNames)); ++i)
            model.tabs.push_back({StringUtils::WideToNarrow(PageNames[i]), i, i == m_selectedTab});
    model.title = StringUtils::WideToNarrow(PageNames[m_selectedTab]);
    model.page = m_selectedTab + 1;
    m_RmlView.MarkDirty("title"); m_RmlView.MarkDirty("page");
    std::vector<EventRow> rows;
    bool anyReceived = false;
    for (const auto& event : Events)
    {
        if (event.page != m_selectedTab) continue;
        int state = 0;
        int seconds = 0;
        const bool received = Network::Paris::EventTimers.Get(event.id, timeGetTime(), state, seconds);
        anyReceived = anyReceived || received;
        char countdown[32]{};
        std::snprintf(countdown, sizeof(countdown), "%02d:%02d:%02d", seconds / 3600, (seconds / 60) % 60, seconds % 60);
        rows.push_back({StringUtils::WideToNarrow(event.name), received ? state != 0 ? "En curso" : "Próximo" : "Sin horario",
            received ? countdown : "--:--:--", static_cast<int>(rows.size()), seconds, received && state != 0});
    }
    const Rml::String summary = anyReceived ? "Horarios actualizados desde el servidor" : "Esperando horarios del servidor";
    if (model.status != summary) { model.status = summary; m_RmlView.MarkDirty("status"); }
    const bool empty = rows.empty();
    if (model.empty != empty) { model.empty = empty; m_RmlView.MarkDirty("empty"); }
    if (model.events != rows) { model.events = std::move(rows); m_RmlView.MarkDirty("events"); }
}
} // namespace mu::ui::window
