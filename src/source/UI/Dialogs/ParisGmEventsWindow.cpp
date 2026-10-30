#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisGmEventsWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "World/MapInfra/MapManager.h"
#include <cwctype>
#include <cstdio>
#include "Network/Paris/GmEventsProtocol.h"
#include "UI/Core/WindowManager.h"
#include "UI/Dialogs/GenericConfirmDialog.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

namespace mu::ui::window
{
namespace { constexpr DWORD InterfaceId = 0x50474D45u; }

CParisGmEventsWindow* g_pParisGmEventsWindow = nullptr;

CParisGmEventsWindow::~CParisGmEventsWindow() { Release(); }

bool CParisGmEventsWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisGmEventsWindow::Release()
{
    m_RmlView.Release();
    if (!m_manager) return;
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CParisGmEventsWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto event = c.RegisterStruct<EventRow>();
    event.RegisterMember("name", &EventRow::name);
    event.RegisterMember("index", &EventRow::index);
    event.RegisterMember("phase", &EventRow::phase);
    event.RegisterMember("selected", &EventRow::selected);
    c.RegisterArray<std::vector<EventRow>>();
    // rml-contract-unused: duration: rendered as the formatted duration_text clock.
    // rml-contract-unused: time_left: rendered as the formatted countdown clock.
    // rml-contract-unused: map: rendered as map_name instead of the numeric map id.
    // rml-contract-unused: selected: event rows expose their own selected state.
    // rml-contract-unused: fresh: controls mutation availability through can_start/can_register/can_stop.
    c.Bind("status", &model.status);
    c.Bind("query", &model.query); c.Bind("announcement", &model.announcement);
    c.Bind("countdown", &model.countdown); c.Bind("duration_text", &model.duration_text); c.Bind("map_name", &model.map_name);
    c.Bind("selected_name", &model.selected_name);
    c.Bind("phase", &model.phase);
    c.Bind("requirement", &model.requirement);
    c.Bind("events", &model.events);
    c.Bind("selected", &model.selected);
    c.Bind("participants", &model.participants);
    c.Bind("maximum", &model.maximum);
    c.Bind("time_left", &model.time_left);
    c.Bind("duration", &model.duration);
    c.Bind("map", &model.map);
    c.Bind("authorized", &model.authorized);
    c.Bind("fresh", &model.fresh);
    c.Bind("can_start", &model.can_start);
    c.Bind("can_register", &model.can_register);
    c.Bind("can_stop", &model.can_stop);
    c.Bind("empty", &model.empty);
    c.BindEventCallback("paris_gm_select", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (!args.empty()) Select(args[0].Get<int>(-1));
    });
    c.BindEventCallback("paris_gm_start", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { SendAction(1); });
    c.BindEventCallback("paris_gm_register", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { SendAction(2); });
    c.BindEventCallback("paris_gm_stop", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { SendAction(3); });
    c.BindEventCallback("paris_gm_announce", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RequestAnnouncement(); });
    c.BindEventCallback("paris_gm_refresh", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        m_receivedBeforeRequest = Network::Paris::GmEvents::Received;
        m_lastRequest = timeGetTime();
        Network::Paris::GmEvents::Request(0, static_cast<BYTE>(std::clamp(m_RmlView.GetModel().selected, 0, Network::Paris::GmEvents::Count - 1)));
    });
    c.BindEventCallback("paris_gm_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisGmEventsWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    auto& model = m_RmlView.GetModel();
    model.status = "Consultando autorización y eventos…";
    model.fresh = false;
    m_RmlView.MarkDirty("status");
    m_RmlView.MarkDirty("fresh");
    m_receivedBeforeRequest = Network::Paris::GmEvents::Received;
    m_lastRequest = timeGetTime();
    Network::Paris::GmEvents::Request(0, static_cast<BYTE>(model.selected));
    Show(true);
}

void CParisGmEventsWindow::CloseWindow() { Show(false); }

void CParisGmEventsWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else m_RmlView.Document()->Hide();
}

bool CParisGmEventsWindow::UpdateMouseEvent() { return !IsVisible(); }
bool CParisGmEventsWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}
bool CParisGmEventsWindow::Update() { return true; }
bool CParisGmEventsWindow::Render()
{
    if (!IsVisible()) return true;
    if (timeGetTime() >= m_nextViewSync || m_syncedQuery != m_RmlView.GetModel().query || m_syncedReceived != Network::Paris::GmEvents::Received)
    {
        m_nextViewSync = timeGetTime() + 250;
        m_syncedQuery = m_RmlView.GetModel().query; m_syncedReceived = Network::Paris::GmEvents::Received;
        SyncModel();
    }
    if (IsVisible() && Network::Paris::GmEvents::Authorized
        && timeGetTime() - Network::Paris::GmEvents::Received >= 1000
        && timeGetTime() - m_lastRequest >= 1000)
    {
        m_receivedBeforeRequest = Network::Paris::GmEvents::Received;
        m_lastRequest = timeGetTime();
        Network::Paris::GmEvents::Request(0, static_cast<BYTE>(std::clamp(m_RmlView.GetModel().selected, 0, Network::Paris::GmEvents::Count - 1)));
    }
    return true;
}

void CParisGmEventsWindow::Select(int index)
{
    if (index < 0 || index >= 10) return;
    auto& model = m_RmlView.GetModel();
    if (model.selected != index) { model.selected = index; m_RmlView.MarkDirty("selected"); }
    m_nextViewSync = 0;
    m_receivedBeforeRequest = Network::Paris::GmEvents::Received;
    m_lastRequest = timeGetTime();
    if (index < Network::Paris::GmEvents::Count) Network::Paris::GmEvents::Request(0, static_cast<BYTE>(index));
}

void CParisGmEventsWindow::SendAction(int action)
{
    const auto& data = Network::Paris::GmEvents::Events;
    const auto& model = m_RmlView.GetModel();
    if (!Network::Paris::GmEvents::Authorized || model.selected < 0 || model.selected >= Network::Paris::GmEvents::Count
        || Network::Paris::GmEvents::Received == 0 || timeGetTime() - Network::Paris::GmEvents::Received >= 5000)
        return;
    const auto& event = data[model.selected];
    const bool allowed = (action == 1 && event.Enabled && event.Phase == 0)
        || (action == 2 && event.Enabled && event.Phase == 1)
        || (action == 3 && event.Phase != 0);
    if (!allowed) return;
    Network::Paris::GmEvents::Request(static_cast<BYTE>(action), static_cast<BYTE>(model.selected));
    m_receivedBeforeRequest = Network::Paris::GmEvents::Received;
    m_lastRequest = timeGetTime();
}

void CParisGmEventsWindow::RequestAnnouncement()
{
    if (!Network::Paris::GmEvents::Authorized || !g_pGenericConfirmDialog) return;
    GenericDialogConfig confirm;
    confirm.title = L"Anuncio del evento";
    confirm.lines.push_back({L"El mensaje se enviará a los jugadores conectados.", false});
    GenericDialogConfig::InputField input;
    input.mode = GenericDialogConfig::InputField::Mode::Text;
    input.maxLength = 120;
    input.initialText = StringUtils::NarrowToWide(m_RmlView.GetModel().announcement);
    confirm.input = input;
    confirm.primaryLabel = L"Enviar anuncio";
    confirm.showCancel = true;
    confirm.cancelLabel = L"Cancelar";
    confirm.onPrimary = [this]
    {
        if (!g_pGenericConfirmDialog) return;
        const std::wstring message = g_pGenericConfirmDialog->GetInputText();
        if (message.empty()) return;
        Network::Paris::GmEvents::Request(4, static_cast<BYTE>(std::clamp(m_RmlView.GetModel().selected, 0, Network::Paris::GmEvents::Count - 1)), message.c_str());
        m_receivedBeforeRequest = Network::Paris::GmEvents::Received;
        m_lastRequest = timeGetTime();
    };
    g_pGenericConfirmDialog->Show(std::move(confirm));
}

void CParisGmEventsWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    auto& model = m_RmlView.GetModel();
    const auto& data = Network::Paris::GmEvents::Events;
    const DWORD received = Network::Paris::GmEvents::Received;
    const bool fresh = Network::Paris::GmEvents::Authorized && received != 0
        && received != m_receivedBeforeRequest && timeGetTime() - received < 5000;
    const auto setBool = [this](bool& field, bool value, const char* key)
    { if (field != value) { field = value; m_RmlView.MarkDirty(key); } };
    const auto setInt = [this](int& field, int value, const char* key)
    { if (field != value) { field = value; m_RmlView.MarkDirty(key); } };
    const Rml::String status = !Network::Paris::GmEvents::Authorized ? "Esperando autorización del servidor"
        : fresh ? "Estado actualizado desde el servidor" : "Esperando respuesta actual del servidor…";
    if (model.status != status) { model.status = status; m_RmlView.MarkDirty("status"); }
    setBool(model.authorized, Network::Paris::GmEvents::Authorized, "authorized");
    setBool(model.fresh, fresh, "fresh");
    std::vector<EventRow> rows;
    std::wstring query = StringUtils::NarrowToWide(model.query);
    for (auto& letter : query) letter = static_cast<wchar_t>(std::towlower(letter));
    for (int i = 0; i < Network::Paris::GmEvents::Count; ++i)
    {
        const auto& source = data[i];
        std::wstring candidate = source.Name;
        for (auto& letter : candidate) letter = static_cast<wchar_t>(std::towlower(letter));
        if (!query.empty() && candidate.find(query) == std::wstring::npos) continue;
        rows.push_back({source.Name.empty() ? "Evento " + std::to_string(i + 1) : StringUtils::WideToNarrow(source.Name.c_str()),
            i, source.Phase, i == model.selected});
    }
    const bool empty = rows.empty();
    setBool(model.empty, empty, "empty");
    if (model.events != rows) { model.events = std::move(rows); m_RmlView.MarkDirty("events"); }
    if (received == 0 || model.selected < 0) return;
    const auto& event = data[model.selected];
    const auto assign = [this](Rml::String& field, Rml::String value, const char* key)
    { if (field != value) { field = std::move(value); m_RmlView.MarkDirty(key); } };
    assign(model.selected_name, StringUtils::WideToNarrow(event.Name.c_str()), "selected_name");
    static constexpr const char* Phases[] = {"Inactivo", "Inscripciones abiertas", "Preparación", "En curso"};
    assign(model.phase, Phases[std::clamp(event.Phase, 0, 3)], "phase");
    assign(model.requirement, StringUtils::WideToNarrow(event.Requirement.c_str()), "requirement");
    setInt(model.participants, event.Participants, "participants");
    setInt(model.maximum, event.Maximum, "maximum");
    setInt(model.time_left, std::max(0, event.Seconds - static_cast<int>((timeGetTime() - received) / 1000)), "time_left");
    setInt(model.duration, event.Duration, "duration");
    setInt(model.map, event.Map, "map");
    char clock[24]{};
    std::snprintf(clock, sizeof(clock), "%02d:%02d", model.time_left / 60, model.time_left % 60);
    assign(model.countdown, clock, "countdown");
    std::snprintf(clock, sizeof(clock), "%02d:%02d", event.Duration / 60, event.Duration % 60);
    assign(model.duration_text, clock, "duration_text");
    assign(model.map_name, StringUtils::WideToNarrow(gMapManager.GetMapName(event.Map)), "map_name");
    setBool(model.can_start, fresh && event.Enabled && event.Phase == 0, "can_start");
    setBool(model.can_register, fresh && event.Enabled && event.Phase == 1, "can_register");
    setBool(model.can_stop, fresh && event.Phase != 0, "can_stop");
}
} // namespace mu::ui::window
