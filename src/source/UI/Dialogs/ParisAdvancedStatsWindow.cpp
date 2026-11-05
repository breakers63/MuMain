#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisAdvancedStatsWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "GameLogic/Character/AdvancedStatsSummary.h"
#include "Network/Paris/AdvancedStatsSnapshot.h"
#include "Network/Paris/ParisRequests.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Dialogs/ParisJewelBankWindow.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlNativeTextSize.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

#include <algorithm>
#include <array>

namespace mu::ui::window
{
namespace
{
constexpr BYTE StatsHead = 0xED;
constexpr BYTE StatsSnapshot = 0x30;
constexpr DWORD RequestCooldown = 2250;
constexpr DWORD RequestTimeout = 5000;
constexpr DWORD RefreshInterval = 2500;

using Field = Network::Paris::AdvancedStatsField;
struct RowSpec { const wchar_t* label; Field field; bool percent; bool section; };
constexpr RowSpec Rows[] = {
    {L"OFFENSIVE", Field::Count, false, true},
    {L"Damage increase", Field::DamageIncrease, true, false},
    {L"Critical DMG Rate", Field::CriticalDamageChance, true, false},
    {L"Critical DMG Bonus", Field::CriticalDamageBonus, false, false},
    {L"Excellent DMG Rate", Field::ExcellentDamageChance, true, false},
    {L"Double DMG Rate", Field::DoubleDamageChance, true, false},
    {L"Ignore Defense", Field::DefenseIgnoreChance, true, false},
    {L"Min Physical DMG", Field::MinimumPhysBaseDmg, false, false},
    {L"Max Physical DMG", Field::MaximumPhysBaseDmg, false, false},
    {L"Min Magic DMG", Field::MinimumWizBaseDmg, false, false},
    {L"Max Magic DMG", Field::MaximumWizBaseDmg, false, false},
    {L"DEFENSIVE", Field::Count, false, true},
    {L"Defense (PvM)", Field::DefensePvm, false, false},
    {L"Critical resistance", Field::CriticalResistance, true, false},
    {L"Excellent resistance", Field::ExcellentResistance, true, false},
    {L"Double DMG resistance", Field::DoubleResistance, true, false},
    {L"Stun resistance", Field::StunResistance, true, false},
    {L"Reflect DMG", Field::DamageReflection, true, false},
    {L"Return Enemy Attack Power", Field::FullyReflectDamageAfterHitChance, true, false},
    {L"Armor DMG Reduction", Field::ArmorDamageDecrease, true, false},
    {L"Soul Barrier Reduction", Field::SoulBarrierReceiveDecrement, true, false},
    {L"PVP & RATES", Field::Count, false, true},
    {L"PvM Attack Rate", Field::AttackRatePvm, false, false},
    {L"PvP Attack Rate", Field::AttackRatePvp, false, false},
    {L"PvM Defense Rate", Field::DefenseRatePvm, false, false},
    {L"PvP Defense Rate", Field::DefenseRatePvp, false, false},
    {L"Ignore SD Rate", Field::ShieldBypassChance, true, false},
    {L"SD Absorption Increase", Field::ShieldRateIncrease, true, false},
    {L"SD Damage Increase", Field::ShieldDecreaseRateIncrease, true, false},
    {L"RECOVERY", Field::Count, false, true},
    {L"Maximum HP", Field::MaximumHealth, false, false},
    {L"Maximum MP", Field::MaximumMana, false, false},
    {L"Full HP Recovery on Hit", Field::FullyRecoverHealthAfterHitChance, true, false},
    {L"Full MP Recovery on Hit", Field::FullyRecoverManaAfterHitChance, true, false},
    {L"RESISTANCES", Field::Count, false, true},
    {L"Ice Resistance", Field::IceResistance, false, false},
    {L"Poison Resistance", Field::PoisonResistance, false, false},
    {L"Lightning Resistance", Field::LightningResistance, false, false},
    {L"Fire Resistance", Field::FireResistance, false, false},
    {L"Earth Resistance", Field::EarthResistance, false, false},
    {L"Wind Resistance", Field::WindResistance, false, false},
    {L"Water Resistance", Field::WaterResistance, false, false},
};

Rml::String FormatValue(const RowSpec& row, const std::array<std::int32_t, CParisAdvancedStatsWindow::FieldCount>& values,
                        int receivedFields)
{
    if (row.section) return {};
    if (static_cast<int>(row.field) >= receivedFields) return "--";
    const auto value = values[static_cast<int>(row.field)];
    if (!row.percent) return std::to_string(value);
    wchar_t text[32]{};
    swprintf_s(text, L"%.2f%%", static_cast<double>(value) / 100.0);
    return StringUtils::WideToNarrow(text);
}
}

CParisAdvancedStatsWindow* g_pParisAdvancedStatsWindow = nullptr;
CParisAdvancedStatsWindow::~CParisAdvancedStatsWindow() { Release(); }

bool CParisAdvancedStatsWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    m_packetHandler = Network::Paris::RegisterHandler(StatsHead, StatsSnapshot,
        [this](const std::uint8_t* packet, int size) { ReceivePacket(packet, size); });
    g_pParisAdvancedStatsWindow = this;
    Show(false);
    return m_packetHandler != 0;
}

void CParisAdvancedStatsWindow::Release()
{
    Network::Paris::UnregisterHandler(m_packetHandler);
    m_packetHandler = 0;
    m_view.Release();
    if (g_pParisAdvancedStatsWindow == this) g_pParisAdvancedStatsWindow = nullptr;
    if (m_manager) { m_manager->RemoveUIObj(this); m_manager = nullptr; }
}

void CParisAdvancedStatsWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto row = c.RegisterStruct<Row>();
    row.RegisterMember("label", &Row::label);
    row.RegisterMember("value", &Row::value);
    row.RegisterMember("section", &Row::section);
    row.RegisterMember("alternate", &Row::alternate);
    c.RegisterArray<std::vector<Row>>();
    model.textPx = UI::RmlBridge::NativeTextPx(UI::Scaling::FontRole::Normal);
    c.Bind("text_px", &model.textPx);
    // rml-contract-unused: status: request state is retained for diagnostics; OpenGL has no status row.
    c.Bind("status", &model.status);
    c.Bind("totalPower", &model.totalPower);
    c.Bind("totalDefense", &model.totalDefense);
    c.Bind("rows", &model.rows);
    c.BindEventCallback("info_stats_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisAdvancedStatsWindow::OpenWindow()
{
    if (!g_pNewUISystem) return;
    if (g_pParisJewelBankWindow && g_pParisJewelBankWindow->IsVisible()) g_pParisJewelBankWindow->CloseWindow();
    if (g_pNewUISystem->IsVisible(INTERFACE_INVENTORY)) g_pNewUISystem->Hide(INTERFACE_INVENTORY);
    if (!g_pNewUISystem->IsVisible(INTERFACE_CHARACTER)) g_pNewUISystem->Show(INTERFACE_CHARACTER);
    if (!m_view.IsBuilt() && !m_view.Ensure()) return;
    m_open = true;
    m_hasSnapshot = false;
    m_pending = false;
    m_values.fill(0);
    m_receivedFields = 0;
    m_modelDirty = true;
    Show(true);
    RequestSnapshot();
    SyncModel();
}

void CParisAdvancedStatsWindow::CloseWindow()
{
    m_open = false;
    m_pending = false;
    Show(false);
}

void CParisAdvancedStatsWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_view.Document()) return;
    if (show) m_view.Document()->Show(); else m_view.Document()->Hide();
}

bool CParisAdvancedStatsWindow::UpdateMouseEvent()
{
    if (!IsVisible()) return true;
    return !UI::RmlBridge::IsPointerOver(m_view.Document());
}

bool CParisAdvancedStatsWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}

bool CParisAdvancedStatsWindow::Update()
{
    if (!m_open) return true;
    UI::RmlBridge::SyncNativeTextSize(m_view.Binder());
    if (!g_pNewUISystem || !g_pNewUISystem->IsVisible(INTERFACE_CHARACTER)) { CloseWindow(); return true; }
    const DWORD now = GetTickCount();
    if (m_pending && now - m_requestSentTick >= RequestTimeout) { m_pending = false; m_modelDirty = true; }
    if (!m_pending && (!m_hasSnapshot || now - m_lastSnapshotTick >= RefreshInterval)) RequestSnapshot();
    SyncModel();
    return true;
}

bool CParisAdvancedStatsWindow::Render() { return true; }

void CParisAdvancedStatsWindow::RequestSnapshot()
{
    if (!m_open || m_pending) return;
    const DWORD now = GetTickCount();
    if (m_lastRequestTick && now - m_lastRequestTick < RequestCooldown) return;
    m_pending = Network::Paris::RequestAdvancedStats();
    m_modelDirty = true;
    if (m_pending) { m_lastRequestTick = now; m_requestSentTick = now; }
}

void CParisAdvancedStatsWindow::ReceivePacket(const std::uint8_t* packet, int size)
{
    if (!m_open || !m_pending || !packet || size < 0) return;
    const auto snapshot = Network::Paris::DecodeAdvancedStatsSnapshot(
        std::span<const std::uint8_t>(packet, static_cast<std::size_t>(size)));
    if (!snapshot) return;
    m_values = snapshot->values;
    m_receivedFields = snapshot->receivedFields;
    m_hasSnapshot = true;
    m_pending = false;
    m_modelDirty = true;
    m_lastSnapshotTick = GetTickCount();
}

void CParisAdvancedStatsWindow::SyncModel()
{
    if (!m_view.Document() || !m_modelDirty) return;
    auto& model = m_view.GetModel();
    model.status = m_hasSnapshot ? "Datos actualizados desde el servidor" : (m_pending ? "Consultando estadísticas del personaje..." : "Esperando respuesta del servidor...");
    std::vector<Row> rows;
    rows.reserve(std::size(Rows));
    int optionIndex = 0;
    for (const RowSpec& row : Rows)
    {
        const bool alternate = !row.section && (optionIndex++ % 2 != 0);
        rows.push_back({StringUtils::WideToNarrow(row.label), FormatValue(row, m_values, m_receivedFields), row.section, alternate});
    }
    model.rows = std::move(rows);
    if (m_hasSnapshot)
    {
        using namespace GameLogic::Character::AdvancedStats;
        model.totalPower = "Total Power: " + std::to_string(ComputeTotalPower(m_values));
        model.totalDefense = "Total Defense: " + std::to_string(ComputeTotalDefense(m_values));
    }
    else
    {
        model.totalPower = "Total Power: --";
        model.totalDefense = "Total Defense: --";
    }
    m_view.MarkDirty("status");
    m_view.MarkDirty("rows");
    m_view.MarkDirty("totalPower");
    m_view.MarkDirty("totalDefense");
    m_modelDirty = false;
}
} // namespace mu::ui::window
