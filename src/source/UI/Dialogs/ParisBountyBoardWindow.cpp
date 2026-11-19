#include "stdafx.h"
#include "ParisBountyBoardWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Network/Paris/BountyBoardProtocol.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include "UI/Dialogs/ParisWindowGroup.h"
#include "UI/RmlBridge/RmlPointer.h"

#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <algorithm>
#include <charconv>
#include <chrono>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50424E54u;
constexpr DWORD RequestTimeoutMs = 8000;
// The server silently drops requests within 500 ms of the previous request.
constexpr DWORD SearchRequestSpacingMs = 550;
constexpr DWORD SearchTypingDelayMs = 350;
constexpr long long MaxRewardAmount = 500000000;
constexpr long long ZenRewardStep = 1000000;
constexpr const char* CurrencyNames[] = {"Zen", "Bless", "Soul", "Life", "Chaos"};

Rml::String Text(const std::wstring& value)
{
    return StringUtils::WideToNarrow(value.c_str());
}

Rml::String RewardText(const Network::Paris::BountyBoard::Row& row)
{
    if (row.Amount <= 0) return "—";
    const int currency = std::clamp<int>(row.Currency, 0, 4);
    return std::to_string(row.Amount) + " " + CurrencyNames[currency];
}

Rml::String RemainingText(unsigned long ends)
{
    if (ends == 0) return "—";
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const long long remaining = static_cast<long long>(ends) - now;
    if (remaining <= 0) return "Vencida";
    const auto hours = remaining / 3600;
    const auto minutes = (remaining % 3600) / 60;
    return hours > 0 ? std::to_string(hours) + " h " + std::to_string(minutes) + " min"
                     : std::to_string(std::max(1LL, minutes)) + " min";
}
}

CParisBountyBoardWindow* g_pParisBountyBoardWindow = nullptr;

CParisBountyBoardWindow::~CParisBountyBoardWindow() { Release(); }

bool CParisBountyBoardWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisBountyBoardWindow::Release()
{
    m_RmlView.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CParisBountyBoardWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name);
    row.RegisterMember("detail", &Row::detail);
    row.RegisterMember("reward", &Row::reward);
    row.RegisterMember("action", &Row::action);
    row.RegisterMember("index_label", &Row::index_label);
    row.RegisterMember("index", &Row::index);
    row.RegisterMember("enabled", &Row::enabled);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("status", &model.status);
    c.Bind("feature_label", &model.feature_label);
    c.Bind("feature_name", &model.feature_name);
    c.Bind("feature_reward", &model.feature_reward);
    c.Bind("feature_time", &model.feature_time);
    c.Bind("table_name_heading", &model.table_name_heading);
    c.Bind("table_status_heading", &model.table_status_heading);
    c.Bind("table_reward_heading", &model.table_reward_heading);
    c.Bind("query", &model.query);
    c.Bind("amount", &model.amount);
    c.Bind("selected", &model.selected);
    c.Bind("currency", &model.currency);
    c.Bind("rows", &model.rows);
    c.Bind("mode", &model.mode);
    c.Bind("empty", &model.empty);
    c.Bind("enabled", &model.enabled);
    c.Bind("currency_open", &model.currency_open);
    c.BindEventCallback("bounty_mode", [this](auto, auto&, const Rml::VariantList& args)
    { if (!args.empty()) SetMode(args[0].Get<int>()); });
    c.BindEventCallback("bounty_refresh", [this](auto, auto&, const Rml::VariantList&)
    {
        if (m_mode == 2) SendRequest(2);
        else if (m_mode == 1) SearchCharacter();
        else SendRequest(0);
    });
    c.BindEventCallback("bounty_search", [this](auto, auto&, const Rml::VariantList&) { SearchCharacter(); });
    c.BindEventCallback("bounty_query_change", [this](auto, Rml::Event& event, const Rml::VariantList&)
    {
        SearchCharacter(!event.GetParameter<bool>("linebreak", false));
    });
    c.BindEventCallback("bounty_currency", [this](auto, auto&, const Rml::VariantList&)
    {
        auto& model = m_RmlView.GetModel();
        model.currency_open = !model.currency_open;
        m_RmlView.MarkDirty("currency_open");
    });
    c.BindEventCallback("bounty_currency_select", [this](auto, auto&, const Rml::VariantList& args)
    {
        if (args.empty()) return;
        m_currency = std::clamp(args[0].Get<int>(), 0, 4);
        auto& model = m_RmlView.GetModel();
        model.currency = CurrencyNames[m_currency];
        model.currency_open = false;
        m_RmlView.MarkDirty("currency");
        m_RmlView.MarkDirty("currency_open");
    });
    c.BindEventCallback("bounty_amount_step", [this](auto, auto&, const Rml::VariantList& args)
    {
        if (args.empty()) return;
        StepAmount(args[0].Get<int>());
    });
    c.BindEventCallback("bounty_publish", [this](auto, auto&, const Rml::VariantList&)
    {
        long long amount = 0;
        if (m_targetName.empty()) { m_RmlView.GetModel().status = "Busca y selecciona un personaje primero."; m_RmlView.MarkDirty("status"); return; }
        if (!ReadAmount(amount) || amount <= 0 || amount > MaxRewardAmount)
        { m_RmlView.GetModel().status = "Escribe una cantidad válida."; m_RmlView.MarkDirty("status"); return; }
        Network::Paris::BountyBoard::Row selected;
        selected.Id = m_targetId;
        selected.Name = m_targetName;
        SendRequest(1, m_targetName, &selected, amount);
    });
    c.BindEventCallback("bounty_row_action", [this](auto, auto&, const Rml::VariantList& args)
    { if (!args.empty()) HandleRowAction(args[0].Get<int>(-1)); });
    c.BindEventCallback("bounty_close", [this](auto, auto&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisBountyBoardWindow::SetMode(int mode)
{
    m_queuedSearch.clear();
    m_mode = std::clamp(mode, 0, 2);
    auto& model = m_RmlView.GetModel();
    model.mode = m_mode;
    model.currency_open = false;
    model.table_name_heading = m_mode == 1 ? "PERSONAJE" : "BUSCADO";
    model.table_status_heading = m_mode == 2 ? "GANADOR" : "ESTADO";
    model.table_reward_heading = "RECOMPENSA";
    m_RmlView.MarkDirty("mode");
    m_RmlView.MarkDirty("currency_open");
    m_RmlView.MarkDirty("table_name_heading");
    m_RmlView.MarkDirty("table_status_heading");
    m_RmlView.MarkDirty("table_reward_heading");
    if (m_mode == 1)
    {
        m_targetId = {};
        m_targetName.clear();
        model.rows.clear();
        model.empty = true;
        model.selected.clear();
        model.feature_label = "PONER PRECIO";
        model.feature_name = "Busca y selecciona un personaje";
        model.feature_reward.clear();
        model.feature_time.clear();
        model.status = "Busca al personaje y luego indica la recompensa.";
        for (const char* field : {"rows", "empty", "selected", "feature_label", "feature_name", "feature_reward", "feature_time", "status"})
            m_RmlView.MarkDirty(field);
    }
    else if (m_mode == 2) SendRequest(2);
    else SendRequest(0);
}

void CParisBountyBoardWindow::SearchCharacter(bool typing)
{
    if (m_mode != 1) return;

    auto& model = m_RmlView.GetModel();
    const auto* field = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(
        m_RmlView.Document()->GetElementById("bounty_query"));
    const auto text = field ? field->GetValue() : model.query;
    const auto first = text.find_first_not_of(" \t\r\n");
    const auto last = text.find_last_not_of(" \t\r\n");
    SetMode(1);
    m_discardSearchResponse = true;

    if (first == Rml::String::npos)
    {
        model.status = "Escribe parte del nombre del personaje.";
        m_RmlView.MarkDirty("status");
        return;
    }

    m_queuedSearch = StringUtils::NarrowToWide(text.substr(first, last - first + 1));
    m_searchQueuedAt = timeGetTime();
    m_searchDelay = typing ? SearchTypingDelayMs : 0;
    model.status = "Buscando personajes…";
    m_RmlView.MarkDirty("status");
    SendQueuedSearch();
}

void CParisBountyBoardWindow::SendQueuedSearch()
{
    if (m_queuedSearch.empty()) return;
    const auto now = timeGetTime();
    if (now - m_searchQueuedAt < m_searchDelay
        || (m_waiting && m_pendingOperation == 3)
        || (m_hasSentRequest && now - m_requestStarted < SearchRequestSpacingMs)) return;
    const auto query = std::move(m_queuedSearch);
    m_queuedSearch.clear();
    m_discardSearchResponse = false;
    SendRequest(3, query);
}

void CParisBountyBoardWindow::SendRequest(BYTE operation, const std::wstring& name,
    const Network::Paris::BountyBoard::Row* row, long long amount)
{
    const std::array<BYTE, 16>* id = row ? &row->Id : nullptr;
    if (!Network::Paris::BountyBoard::Request(operation, name, id,
        static_cast<BYTE>(m_currency), amount))
    {
        m_waiting = false;
        auto& model = m_RmlView.GetModel();
        model.status = "Conéctate al servidor para consultar Cazarrecompensas.";
        m_RmlView.MarkDirty("status");
        return;
    }
    m_pendingOperation = operation;
    m_requestStarted = timeGetTime();
    m_hasSentRequest = true;
    m_waiting = true;
    auto& model = m_RmlView.GetModel();
    model.status = "Esperando respuesta del servidor…";
    m_RmlView.MarkDirty("status");
}

void CParisBountyBoardWindow::StepAmount(int direction)
{
    const long long step = m_currency == 0 ? ZenRewardStep : 1;
    long long amount = 0;
    if (!ReadAmount(amount)) amount = 0;
    amount = std::clamp(amount, 0LL, MaxRewardAmount);
    amount = std::clamp(amount + (direction < 0 ? -step : step), 1LL, MaxRewardAmount);
    m_RmlView.GetModel().amount = std::to_string(amount);
    m_RmlView.MarkDirty("amount");
}

bool CParisBountyBoardWindow::ReadAmount(long long& amount) const
{
    const auto& text = m_RmlView.GetModel().amount;
    if (text.empty()) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), amount);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

void CParisBountyBoardWindow::HandleRowAction(int index)
{
    const auto& rows = Network::Paris::BountyBoard::Data.Rows;
    if (index < 0 || index >= static_cast<int>(rows.size())) return;
    const auto& row = rows[static_cast<std::size_t>(index)];
    if (m_mode == 1 && !m_waiting && !m_discardSearchResponse && Network::Paris::BountyBoard::Data.Operation == 3)
    {
        m_targetId = row.Id;
        m_targetName = row.Name;
        auto& model = m_RmlView.GetModel();
        model.selected = "Objetivo: " + Text(row.Name);
        model.feature_label = "OBJETIVO SELECCIONADO";
        model.feature_name = Text(row.Name);
        model.feature_reward.clear();
        model.feature_time = "Indica la recompensa que quieres ofrecer";
        model.status = "Objetivo seleccionado. Indica la recompensa y publícala.";
        m_RmlView.MarkDirty("selected");
        for (const char* field : {"feature_label", "feature_name", "feature_reward", "feature_time", "status"}) m_RmlView.MarkDirty(field);
        return;
    }
    if (m_mode == 0 && !m_waiting && Network::Paris::BountyBoard::Data.Operation == 0)
    {
        m_targetId = row.Id;
        m_targetName = row.Name;
        auto& model = m_RmlView.GetModel();
        model.feature_label = "SE BUSCA";
        model.feature_name = Text(row.Name);
        model.feature_reward = RewardText(row);
        model.feature_time = "Consultando las bajas...";
        for (const char* field : {"feature_label", "feature_name", "feature_reward", "feature_time"}) m_RmlView.MarkDirty(field);
        SendRequest(4, row.Name, &row);
    }
}

void CParisBountyBoardWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    Network::Paris::BountyBoard::Clear();
    m_mode = 0;
    m_queuedSearch.clear();
    m_lastReceived = 0;
    m_targetId = {};
    m_targetName.clear();
    auto& model = m_RmlView.GetModel();
    model.mode = 0;
    model.currency_open = false;
    model.table_name_heading = "BUSCADO";
    model.table_status_heading = "ESTADO";
    model.feature_label = "SE BUSCA";
    model.feature_name = "Consultando cacerías...";
    model.feature_reward.clear();
    model.feature_time.clear();
    model.selected.clear();
    model.query.clear();
    model.currency = CurrencyNames[m_currency];
    model.rows.clear();
    model.empty = true;
    model.enabled = false;
    m_RmlView.MarkDirty("mode");
    m_RmlView.MarkDirty("currency_open");
    m_RmlView.MarkDirty("table_name_heading");
    m_RmlView.MarkDirty("table_status_heading");
    m_RmlView.MarkDirty("empty");
    m_RmlView.MarkDirty("enabled");
    m_RmlView.MarkDirty("rows");
    m_RmlView.MarkDirty("selected");
    m_RmlView.MarkDirty("query");
    m_RmlView.MarkDirty("currency");
    m_RmlView.MarkDirty("feature_label");
    m_RmlView.MarkDirty("feature_name");
    m_RmlView.MarkDirty("feature_reward");
    m_RmlView.MarkDirty("feature_time");
    m_RmlView.MarkDirty("status");
    Show(true);
    SendRequest(0);
}

void CParisBountyBoardWindow::CloseWindow() { Show(false); }

void CParisBountyBoardWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else m_RmlView.Document()->Hide();
}

bool CParisBountyBoardWindow::UpdateMouseEvent()
{
    // The bounty board is modal. Keep all mouse input inside the window while it is
    // open; relying on Rml's hover hit-test here lets clicks fall through to world
    // movement when the document has not reported a hovered element yet.
    return !IsVisible();
}

bool CParisBountyBoardWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}

bool CParisBountyBoardWindow::Update()
{
    if (IsVisible() && m_waiting && timeGetTime() - m_requestStarted >= RequestTimeoutMs)
    {
        m_waiting = false;
        auto& model = m_RmlView.GetModel();
        if (m_queuedSearch.empty() && !(m_pendingOperation == 3 && m_discardSearchResponse))
        {
            model.status = "El servidor no respondió. Intenta de nuevo.";
            m_RmlView.MarkDirty("status");
        }
    }
    if (IsVisible()) SendQueuedSearch();
    return true;
}

bool CParisBountyBoardWindow::Render()
{
    if (!IsVisible()) return true;
    SyncModel();
    return true;
}

void CParisBountyBoardWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    const auto& snapshot = Network::Paris::BountyBoard::Data;
    if (Network::Paris::BountyBoard::Received == 0 || Network::Paris::BountyBoard::Received == m_lastReceived) return;
    m_lastReceived = Network::Paris::BountyBoard::Received;
    if (snapshot.Operation != m_pendingOperation) return;
    m_waiting = false;
    if (snapshot.Operation == 3 && m_discardSearchResponse) return;
    if (snapshot.Operation == 3)
        mu::log::Get("ui")->info("Cazarrecompensas: search response, enabled={}, rows={}", snapshot.Enabled, snapshot.Rows.size());
    if ((m_mode == 1 && snapshot.Operation != 3 && snapshot.Operation != 1)
        || (m_mode == 2 && snapshot.Operation != 2)
        || (m_mode == 0 && snapshot.Operation != 0 && snapshot.Operation != 4)) return;

    auto& model = m_RmlView.GetModel();
    model.enabled = snapshot.Enabled;
    model.mode = m_mode;
    model.rows.clear();
    for (int index = 0; index < static_cast<int>(snapshot.Rows.size()); ++index)
    {
        const auto& item = snapshot.Rows[static_cast<std::size_t>(index)];
        Row row;
        row.name = Text(item.Name);
        row.index = index;
        row.index_label = std::to_string(index + 1);
        row.enabled = snapshot.Enabled;
        if (snapshot.Operation != 3) row.reward = RewardText(item);
        if (snapshot.Operation == 3) { row.detail = "Disponible"; row.action = "ELEGIR"; }
        else if (snapshot.Operation == 4) { row.detail = std::to_string(item.Kills) + " bajas · " + Text(item.Hunter); row.action.clear(); }
        else if (snapshot.Operation == 2) { row.detail = "Ganador: " + Text(item.Hunter); row.action.clear(); }
        else { row.detail = item.Ends > 0 ? "Activa" : "Vencida"; row.action = "VER BAJAS"; }
        model.rows.push_back(std::move(row));
    }
    model.empty = model.rows.empty();
    if (snapshot.Operation == 0)
    {
        model.feature_label = "SE BUSCA";
        if (!snapshot.Rows.empty())
        {
            const auto& featured = snapshot.Rows.front();
            m_targetId = featured.Id;
            m_targetName = featured.Name;
            model.feature_name = Text(featured.Name);
            model.feature_reward = RewardText(featured);
            model.feature_time = RemainingText(featured.Ends) + " restantes";
        }
        else
        {
            m_targetId = {};
            m_targetName.clear();
            model.feature_name = "No hay objetivos activos";
            model.feature_reward.clear();
            model.feature_time.clear();
        }
    }
    else if (snapshot.Operation == 2)
    {
        model.feature_label = "CACERIAS CERRADAS";
        if (!snapshot.Rows.empty())
        {
            const auto& featured = snapshot.Rows.front();
            model.feature_name = Text(featured.Hunter.empty() ? featured.Name : featured.Hunter);
            model.feature_reward = RewardText(featured);
            model.feature_time = std::to_string(featured.Kills) + " bajas";
        }
        else
        {
            model.feature_name = "Sin ficha disponible";
            model.feature_reward.clear();
            model.feature_time.clear();
        }
    }
    else if (snapshot.Operation == 3)
    {
        model.feature_label = "BUSCAR OBJETIVO";
        model.feature_name = "Selecciona un personaje de la lista";
        model.feature_reward.clear();
        model.feature_time.clear();
    }
    else if (snapshot.Operation == 4)
    {
        model.feature_label = "DETALLE DE BAJAS";
        model.feature_name = Text(m_targetName);
        model.feature_reward = model.rows.empty() ? "Sin bajas registradas" : std::to_string(model.rows.size()) + " registros";
        model.feature_time.clear();
    }
    if (!snapshot.Enabled) model.status = snapshot.Message.empty() ? "Cazarrecompensas está desactivado por el servidor." : Text(snapshot.Message);
    else if (!snapshot.Message.empty()) model.status = Text(snapshot.Message);
    else model.status.clear();
    for (const char* field : {"enabled", "mode", "rows", "empty", "status", "feature_label", "feature_name", "feature_reward", "feature_time"}) m_RmlView.MarkDirty(field);
}

extern CParisBountyBoardWindow* g_pParisBountyBoardWindow;
}
