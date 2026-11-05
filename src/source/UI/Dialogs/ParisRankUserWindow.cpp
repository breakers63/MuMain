#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisRankUserWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "Network/Paris/RankUserProtocol.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50525557u;
Rml::String Number(int value) { return std::to_string(value); }
Rml::String Text(const std::wstring& value) { return StringUtils::WideToNarrow(value.c_str()); }
} // namespace

CParisRankUserWindow* g_pParisRankUserWindow = nullptr;

CParisRankUserWindow::~CParisRankUserWindow()
{
    Release();
}

bool CParisRankUserWindow::Create(CManager* manager)
{
    if (!manager)
        return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisRankUserWindow::Release()
{
    m_RmlView.Release();
    if (m_manager)
    {
        m_manager->RemoveUIObj(this);
        m_manager = nullptr;
    }
}

void CParisRankUserWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    c.Bind("status", &model.status);
    c.Bind("name", &model.name);
    c.Bind("rank", &model.rank);
    c.Bind("character_class", &model.characterClass);
    c.Bind("level", &model.level);
    c.Bind("resets", &model.resets);
    c.Bind("total_stats", &model.totalStats);
    c.Bind("strength", &model.strength);
    c.Bind("agility", &model.agility);
    c.Bind("vitality", &model.vitality);
    c.Bind("energy", &model.energy);
    c.Bind("command", &model.command);
    c.Bind("account", &model.account);
    c.BindEventCallback("paris_rank_refresh",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Refresh(); });
    c.BindEventCallback("paris_rank_close",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisRankUserWindow::OpenWindow(std::uint16_t target)
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure())
        return;
    m_target = target;
    Show(true);
    Refresh();
}

void CParisRankUserWindow::CloseWindow()
{
    Show(false);
}

void CParisRankUserWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document())
        return;
    if (show)
        m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
        m_RmlView.Document()->Hide();
}

bool CParisRankUserWindow::UpdateMouseEvent()
{
    return !IsVisible();
}

bool CParisRankUserWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE))
    {
        CloseWindow();
        return false;
    }
    return !IsVisible();
}

bool CParisRankUserWindow::Update()
{
    return true;
}

bool CParisRankUserWindow::Render()
{
    SyncModel();
    return true;
}

void CParisRankUserWindow::Refresh()
{
    if (!Network::Paris::RankUser::Request(m_target))
    {
        auto& model = m_RmlView.GetModel();
        model.status = "No se pudo consultar el servidor";
        m_RmlView.MarkDirty("status");
    }
}

void CParisRankUserWindow::SyncModel()
{
    if (!m_RmlView.Document())
        return;
    auto& model = m_RmlView.GetModel();
    const auto& data = Network::Paris::RankUser::Data;
    const bool received = Network::Paris::RankUser::Received != 0 && data.target == m_target;
    const auto set = [this](Rml::String Model::*field, const char* binding, Rml::String value)
    {
        auto& model = m_RmlView.GetModel();
        if (model.*field != value)
        {
            model.*field = std::move(value);
            m_RmlView.MarkDirty(binding);
        }
    };
    set(&Model::status, "status", received ? Rml::String("Perfil actualizado del servidor") : Rml::String("Esperando datos del servidor…"));
    if (!received)
        return;

    constexpr std::uint16_t NameField = 1, RankField = 2, ClassField = 4, LevelField = 8,
        ResetField = 16, VipField = 32, StatsField = 64;
    set(&Model::name, "name", (data.fields & NameField) ? Text(data.name) : "Nombre privado");
    set(&Model::rank, "rank", (data.fields & RankField) ? Text(data.rank) : "");
    set(&Model::characterClass, "character_class", (data.fields & ClassField) ? Text(data.characterClass) : "Clase privada");
    set(&Model::level, "level", (data.fields & LevelField) ? Number(data.values[0]) : "Privado");
    set(&Model::resets, "resets", (data.fields & ResetField) ? Number(data.values[1]) : "Privado");
    const int totalStats = data.values[3] + data.values[4] + data.values[5] + data.values[6] + data.values[7];
    set(&Model::totalStats, "total_stats", (data.fields & StatsField) ? Number(totalStats) : "Privado");
    set(&Model::strength, "strength", (data.fields & StatsField) ? Number(data.values[3]) : "Privado");
    set(&Model::agility, "agility", (data.fields & StatsField) ? Number(data.values[4]) : "Privado");
    set(&Model::vitality, "vitality", (data.fields & StatsField) ? Number(data.values[5]) : "Privado");
    set(&Model::energy, "energy", (data.fields & StatsField) ? Number(data.values[6]) : "Privado");
    set(&Model::command, "command", (data.fields & StatsField) ? Number(data.values[7]) : "Privado");
    set(&Model::account, "account", (data.fields & VipField) ? (data.values[2] ? "VIP" : "Normal") : "No publicado");
}
} // namespace mu::ui::window
