#include "stdafx.h"
#include "UI/Character/ParisStatAllocationController.h"
#include "Network/Paris/ParisPacketDispatch.h"
#include "Network/Paris/ParisRequests.h"
#include "Network/Server/WSclient.h"
#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Scenes/SceneCore.h"
#include "Core/Utilities/StringUtils.h"
#include <RmlUi/Core/Event.h>
#include <charconv>
#include <climits>

namespace UI::Character {
namespace { constexpr DWORD RequestTimeout = 5000, AssignmentInterval = 50; }
ParisStatAllocationController::ParisStatAllocationController()
{
    const char* labels[] = {"Fuerza", "Agilidad", "Vitalidad", "Energia", "Comando"};
    for (int index = 0; index < 5; ++index) m_rows.push_back({labels[index], "0", index});
    m_textHandler = Network::Paris::RegisterTextHandler([this](std::wstring_view text)
    {
        if (!text.starts_with(L"[#PSL1:")) return false;
        const auto snapshot = Network::Paris::DecodeStatLimits(text);
        if (snapshot) { std::scoped_lock lock(m_receiveMutex); m_receivedLimits = snapshot; }
        return true;
    });
}
ParisStatAllocationController::~ParisStatAllocationController() { Network::Paris::UnregisterTextHandler(m_textHandler); }
void ParisStatAllocationController::Dirty(const char* variable) { if (m_model) m_model.DirtyVariable(variable); }
void ParisStatAllocationController::Bind(Rml::DataModelConstructor& c)
{
    m_model = c.GetModelHandle();
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("name", &Row::name); row.RegisterMember("amount", &Row::amount);
    row.RegisterMember("index", &Row::index); row.RegisterMember("full", &Row::full); row.RegisterMember("enabled", &Row::enabled);
    c.RegisterArray<std::vector<Row>>();
    c.Bind("allocation_rows", &m_rows); c.Bind("allocation_open", &m_state.open);
    c.Bind("allocation_percent", &m_state.percentMode); c.Bind("allocation_busy", &m_state.applying);
    c.Bind("allocation_auto", &m_state.automatic); c.Bind("allocation_status", &m_status); c.Bind("allocation_available", &m_available);
    c.BindEventCallback("allocation_open_panel", [this](auto, auto&, const auto&) { Open(); });
    c.BindEventCallback("allocation_close", [this](auto, auto&, const auto&) { Close(); });
    c.BindEventCallback("allocation_apply", [this](auto, auto&, const auto&) { Apply(); });
    c.BindEventCallback("allocation_automatic", [this](auto, auto&, const auto&) { ToggleAutomatic(); });
    c.BindEventCallback("allocation_mode", [this](auto, auto&, const Rml::VariantList& args)
        { if (!args.empty()) SetMode(args[0].Get<bool>(false)); });
    c.BindEventCallback("allocation_full", [this](auto, auto&, const Rml::VariantList& args)
        { if (!args.empty()) ToggleFull(args[0].Get<int>(-1)); });
}
unsigned int ParisStatAllocationController::Base(int index) const
{
    switch (index) {
    case 0: return CharacterAttribute->Strength; case 1: return CharacterAttribute->Dexterity;
    case 2: return CharacterAttribute->Vitality; case 3: return CharacterAttribute->Energy;
    case 4: return CharacterAttribute->Charisma; default: return 0;
    }
}
int ParisStatAllocationController::Available() const { return static_cast<int>(std::min<unsigned int>(CharacterAttribute->LevelUpPoint, INT_MAX)); }
std::array<int,5> ParisStatAllocationController::Room() const
{
    std::array<int,5> room{};
    if (!m_limitsReady) return room;
    const int count = gCharacterManager.GetBaseClass(Hero->Class) == CLASS_DARK_LORD ? 5 : 4;
    for (int index = 0; index < count; ++index) room[index] = std::max(0, m_limits[index] - static_cast<int>(Base(index)));
    return room;
}
void ParisStatAllocationController::SetStatus()
{
    const auto status = StringUtils::WideToNarrow(m_state.status);
    if (status != m_status) { m_status = status; Dirty("allocation_status"); }
    for (const char* field : {"allocation_busy", "allocation_auto", "allocation_percent", "allocation_open"}) Dirty(field);
}
void ParisStatAllocationController::RequestLimits()
{
    if (!Hero || m_limitsPending) return;
    ++m_request;
    if (m_request == 0) ++m_request;
    m_limitsReady = false;
    m_limitsPending = Network::Paris::RequestStatLimits(Hero->ID, m_request);
    m_limitsAt = GetTickCount();
    m_state.status = m_limitsPending ? L"Consultando limites del servidor..." : L"Sin conexion con el servidor.";
    SetStatus();
}
void ParisStatAllocationController::ConsumeLimits()
{
    std::optional<Network::Paris::StatLimitsSnapshot> snapshot;
    { std::scoped_lock lock(m_receiveMutex); snapshot.swap(m_receivedLimits); }
    if (!snapshot || !m_limitsPending || snapshot->request != m_request) return;
    m_limits = snapshot->values; m_limitsReady = true; m_limitsPending = false;
    m_state.status = L"Escribe cantidades y pulsa ASIGNAR.";
    SetStatus();
}
void ParisStatAllocationController::Open()
{
    m_state.open = true;
    const bool darkLord = Hero && gCharacterManager.GetBaseClass(Hero->Class) == CLASS_DARK_LORD;
    m_rows[4].enabled = darkLord;
    if (!darkLord) { m_rows[4].amount = "0"; m_state.amounts[4] = m_state.percentages[4] = 0; m_state.full[4] = false; }
    Dirty("allocation_rows"); Dirty("allocation_open");
    if (!m_state.applying) RequestLimits();
}
void ParisStatAllocationController::Hide()
{
    m_state.open = false;
    if (m_state.applying && !m_state.automatic) m_state.stopRequested = true;
    Dirty("allocation_open");
}
void ParisStatAllocationController::Close()
{
    if (m_state.applying) { m_state.stopRequested = true; m_state.automatic = false; }
    else m_state.open = false;
    SetStatus();
}
bool ParisStatAllocationController::ReadFields()
{
    auto values = std::array<int,5>{};
    for (const auto& row : m_rows)
    {
        if (!row.enabled) continue;
        int value = 0;
        const auto result = std::from_chars(row.amount.data(), row.amount.data() + row.amount.size(), value);
        const int maximum = m_state.percentMode ? 100 : GameLogic::Character::StatAllocationState::MaxAmount;
        if (row.amount.empty()) value = 0;
        else if (result.ec != std::errc{} || result.ptr != row.amount.data() + row.amount.size() || value < 0 || value > maximum)
        { m_state.status = L"Revisa las cantidades ingresadas."; SetStatus(); return false; }
        values[row.index] = value;
    }
    if (m_state.percentMode) m_state.percentages = values; else m_state.amounts = values;
    return true;
}
void ParisStatAllocationController::WriteFields()
{
    for (auto& row : m_rows) { row.amount = std::to_string(m_state.percentMode ? m_state.percentages[row.index] : m_state.amounts[row.index]); row.full = m_state.full[row.index]; }
    Dirty("allocation_rows");
}
void ParisStatAllocationController::SetMode(bool percent)
{
    if (m_state.applying || m_state.automatic) return;
    if (!ReadFields()) return;
    m_state.percentMode = percent; WriteFields(); SetStatus();
}
void ParisStatAllocationController::ToggleFull(int index)
{
    if (index < 0 || index >= 5 || !m_rows[index].enabled || m_state.applying || m_state.automatic || !ReadFields()) return;
    if (!m_limitsReady) { RequestLimits(); return; }
    m_state.full[index] = !m_state.full[index];
    if (!m_state.full[index]) m_state.amounts[index] = 0;
    m_state.PlanFull(Available(), Room()); WriteFields();
}
void ParisStatAllocationController::Apply()
{
    if (m_state.applying || !ReadFields()) return;
    if (!m_limitsReady) { RequestLimits(); return; }
    if (m_state.percentMode)
    {
        if (m_state.PercentTotal() < 1 || m_state.PercentTotal() > 100) { m_state.status = L"La suma debe ser de 1 a 100%."; SetStatus(); return; }
        m_state.remainders.fill(0); m_state.PlanPercent(Available(), Room());
    }
    else m_state.PlanFull(Available(), Room());
    if (m_state.Total() == 0) { m_state.status = L"Sin puntos para asignar."; SetStatus(); return; }
    m_state.applying = true; m_state.status = L"Esperando al servidor..."; SetStatus();
}
void ParisStatAllocationController::ToggleAutomatic()
{
    if (m_state.automatic) { m_state.automatic = false; m_state.stopRequested = m_state.applying; SetStatus(); return; }
    if (m_state.applying || !ReadFields()) return;
    if (m_state.StartAutomatic()) { m_autoBudget = Available(); RequestLimits(); }
    SetStatus();
}
void ParisStatAllocationController::UpdateAutomatic(unsigned int points)
{
    if (m_state.automatic && points > m_previousPoints)
    {
        m_autoBudget = static_cast<int>(std::min<long long>(INT_MAX, static_cast<long long>(m_autoBudget) + points - m_previousPoints));
        if (!m_state.applying && !m_limitsPending) RequestLimits();
    }
    if (!m_state.automatic || m_state.applying || !m_limitsReady || m_autoBudget <= 0) return;
    m_state.PlanAutomatic(m_autoBudget, Available(), Room()); m_autoBudget = 0;
    m_state.applying = m_state.Total() > 0;
    if (m_state.applying) m_state.status = L"Repartiendo automaticamente...";
    SetStatus();
}
void ParisStatAllocationController::UpdateAssignment(unsigned int points, unsigned long now)
{
    if (!m_state.applying) return;
    if (m_state.waiting)
    {
        const auto current = Base(m_state.pendingStat);
        if (current == m_state.previousValue + m_state.pendingAmount && points < m_state.previousPoints)
        { m_state.amounts[m_state.pendingStat] = 0; m_state.waiting = false; }
        else if (current > m_state.previousValue && points < m_state.previousPoints)
        { m_state.automatic = false; m_state.Stop(L"El servidor aplico un limite. Revisa stats."); }
        else if (now - m_state.sentAt >= RequestTimeout)
        { m_state.automatic = false; m_state.Stop(L"Sin confirmacion. Revisa tus puntos."); }
    }
    if (!m_state.applying || m_state.waiting || now - m_state.sentAt < AssignmentInterval) return;
    if (m_state.stopRequested) { m_state.Stop(L"Detenido. Se conserva lo confirmado."); SetStatus(); return; }
    if (m_state.Total() == 0 || points == 0)
    { m_state.Stop(m_state.automatic ? L"Automatico activo. Esperando puntos." : L"Asignacion finalizada."); WriteFields(); SetStatus(); return; }
    if (m_state.Total() > Available()) { m_state.automatic = false; m_state.Stop(L"Los puntos disponibles cambiaron."); SetStatus(); return; }
    for (int index = 0; index < 5; ++index)
    {
        if (m_state.amounts[index] == 0) continue;
        m_state.pendingStat = index; m_state.pendingAmount = m_state.amounts[index];
        m_state.previousValue = Base(index); m_state.previousPoints = points; m_state.sentAt = now;
        m_state.waiting = Network::Paris::RequestStatAllocation(Hero->ID, index, m_state.pendingAmount);
        if (!m_state.waiting) { m_state.automatic = false; m_state.Stop(L"Sin conexion con el servidor."); }
        SetStatus(); break;
    }
}
void ParisStatAllocationController::Tick()
{
    if (!Hero || !CharacterAttribute || SceneFlag != MAIN_SCENE || !SocketClient || !SocketClient->IsConnected())
    { m_state.automatic = false; m_state.Stop(L"Asignacion cerrada."); Hide(); m_owner.clear(); m_limitsReady = m_limitsPending = false; return; }
    const unsigned int points = CharacterAttribute->LevelUpPoint;
    if (m_owner != CharacterAttribute->Name || m_previousReset != CharacterAttribute->Resets || CharacterAttribute->Level < m_previousLevel)
    {
        m_state = {}; m_limitsReady = m_limitsPending = false; m_autoBudget = 0;
        m_owner = CharacterAttribute->Name; m_previousPoints = points; WriteFields(); SetStatus();
    }
    ConsumeLimits();
    const DWORD now = GetTickCount();
    if (m_limitsPending && now - m_limitsAt >= RequestTimeout)
    { m_limitsPending = false; m_state.automatic = false; m_state.status = L"Sin limites del servidor. Revisa plugin."; SetStatus(); }
    UpdateAutomatic(points); UpdateAssignment(points, now);
    m_previousPoints = points; m_previousLevel = CharacterAttribute->Level; m_previousReset = CharacterAttribute->Resets;
    const auto available = std::to_string(points);
    if (available != m_available) { m_available = available; Dirty("allocation_available"); }
    SetStatus();
}
}
