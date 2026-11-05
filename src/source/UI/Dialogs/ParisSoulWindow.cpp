#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisSoulWindow.h"
#include "Network/Paris/SoulProtocol.h"
#include "GameLogic/Character/SoulAllocation.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include <RmlUi/Core/Event.h>
#include <algorithm>
#include <cstdio>

namespace mu::ui::window
{
namespace
{
namespace Soul = Network::Paris::Soul;
constexpr DWORD InterfaceId = 0x50534F55u;
constexpr DWORD ReplyTimeout = 5000;
Rml::String Text(const std::wstring& value) { return StringUtils::WideToNarrow(value.c_str()); }
Rml::String Label(const char* key, const char* fallback)
{
    const auto& value = Soul::Label(key);
    return value.empty() ? fallback : Text(value);
}
}
CParisSoulWindow* g_pParisSoulWindow = nullptr;
CParisSoulWindow::~CParisSoulWindow() { Release(); }
bool CParisSoulWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}
void CParisSoulWindow::Release()
{
    m_RmlView.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisSoulWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    m_handle = c.GetModelHandle();
    auto tier = c.RegisterStruct<Tier>();
    tier.RegisterMember("index", &Tier::index); tier.RegisterMember("rank", &Tier::rank);
    tier.RegisterMember("top", &Tier::top); tier.RegisterMember("hint", &Tier::hint);
    tier.RegisterMember("unlocked", &Tier::unlocked);
    c.RegisterArray<std::vector<Tier>>();
    auto column = c.RegisterStruct<Column>();
    column.RegisterMember("index", &Column::index); column.RegisterMember("rank", &Column::rank);
    column.RegisterMember("name", &Column::name); column.RegisterMember("benefit", &Column::benefit);
    column.RegisterMember("progress", &Column::progress); column.RegisterMember("add", &Column::add);
    column.RegisterMember("subtract", &Column::subtract); column.RegisterMember("tiers", &Column::tiers);
    c.RegisterArray<std::vector<Column>>();
    c.Bind("columns", &model.columns); c.Bind("category", &model.category);
    c.Bind("remaining", &model.remaining); c.Bind("status", &model.status);
    c.Bind("confirmation", &model.confirmation); c.Bind("confirm_reset", &model.confirmReset);
    c.Bind("loading", &model.loading); c.Bind("pending", &model.pending);
    c.Bind("can_apply", &model.canApply); c.Bind("can_reset", &model.canReset);
    c.BindEventCallback("soul_tab", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (args.empty() || m_RmlView.GetModel().confirmReset) return;
        m_RmlView.GetModel().category = std::clamp(args[0].Get<int>(), 0, 3);
        m_refresh = true;
    });
    c.BindEventCallback("soul_rank", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    { if (args.size() == 2) ChangeRank(args[0].Get<int>(), args[1].Get<int>()); });
    c.BindEventCallback("soul_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Show(false); });
    c.BindEventCallback("soul_cancel", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    { if (!m_RmlView.GetModel().pending) { m_draft = Soul::Data.Ranks; m_refresh = true; } });
    c.BindEventCallback("soul_apply", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    { if (m_RmlView.GetModel().canApply) Send(1); });
    c.BindEventCallback("soul_reset", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    { if (m_RmlView.GetModel().canReset) { m_RmlView.GetModel().confirmReset = true; m_refresh = true; } });
    c.BindEventCallback("soul_confirm", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    { if (m_RmlView.GetModel().confirmReset && m_RmlView.GetModel().canReset) { m_RmlView.GetModel().confirmReset = false; Send(2); } });
    c.BindEventCallback("soul_dismiss", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    { m_RmlView.GetModel().confirmReset = false; m_refresh = true; });
}
void CParisSoulWindow::OpenWindow()
{
    if (!m_RmlView.Ensure()) return;
    m_draft = Soul::Data.Ranks;
    m_observed = Soul::Received;
    auto& model = m_RmlView.GetModel();
    model.pending = model.confirmReset = false;
    m_queriedAfterTimeout = false;
    m_refresh = true;
    Show(true);
    Send(0);
}
void CParisSoulWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else m_RmlView.Document()->Hide();
}
bool CParisSoulWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE))
    {
        auto& model = m_RmlView.GetModel();
        if (model.confirmReset) { model.confirmReset = false; m_refresh = true; }
        else Show(false);
        return false;
    }
    return !IsVisible();
}
bool CParisSoulWindow::Update() { if (IsVisible()) SyncModel(); return true; }
bool CParisSoulWindow::Render() { if (IsVisible()) SyncModel(); return true; }
void CParisSoulWindow::Send(int action)
{
    auto& model = m_RmlView.GetModel();
    if (action != 0 && (model.pending || m_observed != Soul::Received)) return;
    if (!Soul::Request(static_cast<BYTE>(action), static_cast<BYTE>(model.category), m_draft))
        model.status = "No se pudo enviar la solicitud al servidor.";
    else { m_sent = timeGetTime(); if (action != 0) { model.pending = true; m_queriedAfterTimeout = false; } }
    m_refresh = true;
}
void CParisSoulWindow::ChangeRank(int column, int delta)
{
    const auto& model = m_RmlView.GetModel();
    if (model.pending || model.confirmReset || m_observed != Soul::Received || column < 0 || column >= 4) return;
    if (GameLogic::Character::SoulAllocation::Change(Soul::Data, m_draft, model.category * 4 + column, delta)) m_refresh = true;
}
void CParisSoulWindow::SyncModel()
{
    auto& model = m_RmlView.GetModel();
    if (m_observed != Soul::Received)
    {
        m_observed = Soul::Received;
        m_draft = Soul::Data.Ranks;
        model.pending = model.confirmReset = false;
        m_queriedAfterTimeout = false;
        m_refresh = true;
    }
    if (model.pending && !m_queriedAfterTimeout && timeGetTime() - m_sent > ReplyTimeout)
    {
        // Query once after a missing reply; never repeat a mutation automatically.
        m_queriedAfterTimeout = true;
        Send(0);
    }
    if (m_refresh) RefreshModel();
}
void CParisSoulWindow::RefreshModel()
{
    m_refresh = false;
    const auto& data = Soul::Data;
    auto& model = m_RmlView.GetModel();
    model.loading = Soul::Received == 0;
    model.remaining = Label("Remaining", "Puntos de alma disponibles") + ": " + std::to_string(std::max(0LL, data.Total - GameLogic::Character::SoulAllocation::Cost(data, m_draft)));
    model.canApply = !model.loading && data.Enabled && !model.pending && m_draft != data.Ranks;
    model.canReset = !model.loading && data.Enabled && data.AllowReset && data.Spent > 0 && !model.pending;
    model.confirmation = Label("ConfirmReset", "¿Reiniciar Soul System? Coste:") + " " + std::to_string(data.ResetFee);
    model.status = model.loading ? Label("Loading", "Consultando con el servidor…") : !data.Enabled ? Label("Disabled", "Soul System deshabilitado")
        : model.pending ? Label("Pending", "Esperando confirmación del servidor…") : data.Decision > 0 ? Label(("Error" + std::to_string(data.Decision)).c_str(), "El servidor rechazó la solicitud.") : "";
    model.columns.clear();
    for (int column = 0; column < 4; ++column)
    {
        const int index = model.category * 4 + column;
        const auto& rule = data.Abilities[index];
        const double fraction = static_cast<double>(std::min(m_draft[index], rule.MaximumRank)) / std::max(1, rule.MaximumRank);
        char benefit[64]{};
        std::snprintf(benefit, sizeof(benefit), "+%.2f%s", rule.Enabled ? rule.MaximumBenefit * fraction : 0., index == 3 || index == 11 ? "" : "%");
        Column row;
        row.index = column; row.rank = m_draft[index]; row.name = Text(rule.Name); row.benefit = benefit;
        row.progress = std::to_string(std::clamp(fraction * 100., 0., 100.)) + "%";
        row.add = data.Enabled && rule.Enabled && !model.pending && m_draft[index] < rule.MaximumRank;
        row.subtract = data.Enabled && data.AllowReallocation && !model.pending && m_draft[index] > 0;
        for (int tier = 0; tier < 3; ++tier)
        {
            char bonus[64]{}; std::snprintf(bonus, sizeof(bonus), "+%.2f", rule.Bonuses[tier]);
            row.tiers.push_back({tier, rule.Milestones[tier], data.Enabled && rule.Enabled && data.Ranks[index] >= rule.Milestones[tier],
                std::to_string(100. * rule.Milestones[tier] / std::max(1, rule.MaximumRank)) + "%", Label("Milestone", "Hito") + " " + bonus});
        }
        model.columns.push_back(std::move(row));
    }
    m_handle.DirtyAllVariables();
}
}
