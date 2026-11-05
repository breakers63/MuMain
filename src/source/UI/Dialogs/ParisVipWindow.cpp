#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisVipWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "Network/Paris/ParisRequests.h"
#include "Network/Paris/VipProtocol.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include "UI/Dialogs/GenericConfirmDialog.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50564950u;
}

CParisVipWindow::Line CParisVipWindow::ParseLine(const Rml::String& text)
{
    Line line;
    line.text = text;
    const auto separator = text.find(':');
    if (separator != Rml::String::npos && text.find('%', separator) != Rml::String::npos)
    {
        line.label = text.substr(0, separator);
        line.value = text.substr(separator + 1);
    }
    else
    {
        line.label = text;
    }
    return line;
}

CParisVipWindow* g_pParisVipWindow = nullptr;

CParisVipWindow::~CParisVipWindow() { Release(); }

bool CParisVipWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    CObject::Show(false);
    return true;
}

void CParisVipWindow::Release()
{
    m_RmlView.Release();
    if (!m_manager) return;
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CParisVipWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto line = c.RegisterStruct<Line>();
    line.RegisterMember("text", &Line::text);
    line.RegisterMember("label", &Line::label);
    line.RegisterMember("value", &Line::value);
    c.RegisterArray<std::vector<Line>>();
    c.Bind("status", &model.status);
    c.Bind("plan_name", &model.plan_name);
    c.Bind("duration", &model.duration);
    c.Bind("benefits", &model.benefits);
    c.Bind("costs", &model.costs);
    c.Bind("current_tier", &model.current_tier);
    c.Bind("selected_days", &model.selected_days);
    c.Bind("plan", &model.plan);
    c.Bind("has_data", &model.has_data);
    c.Bind("can_buy", &model.can_buy);
    c.Bind("empty", &model.empty);
    c.BindEventCallback("paris_vip_plan", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (!args.empty()) RequestPlan(args[0].Get<int>(0));
    });
    c.BindEventCallback("paris_vip_purchase", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ConfirmPurchase(); });
    c.BindEventCallback("paris_vip_refresh", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RequestPlan(m_plan); });
    c.BindEventCallback("paris_vip_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { CloseWindow(); });
}

void CParisVipWindow::OpenWindow()
{
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    Show(true);
    RequestPlan(m_plan);
}

void CParisVipWindow::CloseWindow() { Show(false); }

void CParisVipWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else m_RmlView.Document()->Hide();
}

bool CParisVipWindow::UpdateMouseEvent() { return !IsVisible(); }

bool CParisVipWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}

bool CParisVipWindow::Update() { return true; }
bool CParisVipWindow::Render() { SyncModel(); return true; }

void CParisVipWindow::RequestPlan(int plan)
{
    if (plan < 1 || plan > 3) return;
    m_plan = plan;
    m_receivedBeforeRequest = Network::Paris::Vip::Received;
    auto& model = m_RmlView.GetModel();
    model.plan = plan;
    model.status = "Consultando plan y beneficios…";
    model.can_buy = false;
    model.has_data = false;
    model.plan_name.clear(); model.duration.clear(); model.benefits.clear(); model.costs.clear();
    m_RmlView.MarkDirty("plan");
    m_RmlView.MarkDirty("status");
    m_RmlView.MarkDirty("can_buy");
    m_RmlView.MarkDirty("has_data");
    m_RmlView.MarkDirty("plan_name"); m_RmlView.MarkDirty("duration");
    m_RmlView.MarkDirty("benefits"); m_RmlView.MarkDirty("costs");
    if (!Network::Paris::RequestVip(plan, false, 0))
    {
        model.status = "No se pudo consultar el servidor";
        m_RmlView.MarkDirty("status");
    }
}

void CParisVipWindow::ConfirmPurchase()
{
    const auto& data = Network::Paris::Vip::Data;
    const bool fresh = Network::Paris::Vip::Received != 0
        && Network::Paris::Vip::Received != m_receivedBeforeRequest
        && timeGetTime() - Network::Paris::Vip::Received < 30000
        && Network::Paris::Vip::Data.RequestedPlan == m_plan;
    if (!fresh || !data.CanBuy || data.Quote == 0 || data.RequestedPlan != m_plan || !g_pGenericConfirmDialog) return;

    GenericDialogConfig confirm;
    confirm.title = L"Confirmar compra VIP";
    for (const auto& line : data.Lines)
        confirm.lines.push_back({line, false});
    confirm.lines.push_back({L"El servidor volverá a validar precio y requisitos.", false});
    confirm.primaryLabel = L"Comprar VIP";
    confirm.showCancel = true;
    confirm.cancelLabel = L"Volver";
    const int plan = data.RequestedPlan;
    const std::uint64_t quote = data.Quote;
    confirm.onPrimary = [plan, quote]
    {
        if (!Network::Paris::RequestVip(plan, true, quote) && g_pParisVipWindow)
            g_pParisVipWindow->RequestPlan(plan);
    };
    g_pGenericConfirmDialog->Show(std::move(confirm));
}

void CParisVipWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    auto& model = m_RmlView.GetModel();
    const auto& data = Network::Paris::Vip::Data;
    const auto received = Network::Paris::Vip::Received;
    const bool fresh = received != 0 && received != m_receivedBeforeRequest && timeGetTime() - received < 30000
        && data.RequestedPlan == m_plan;
    const Rml::String status = fresh ? "Plan actualizado desde el servidor" : "Esperando respuesta actual del servidor…";
    if (model.status != status) { model.status = status; m_RmlView.MarkDirty("status"); }
    if (model.has_data != fresh) { model.has_data = fresh; m_RmlView.MarkDirty("has_data"); }
    const bool canBuy = fresh && data.CanBuy && data.Quote != 0;
    if (model.can_buy != canBuy) { model.can_buy = canBuy; m_RmlView.MarkDirty("can_buy"); }
    static constexpr const char* AccountNames[] = {"Normal", "VIP 1", "VIP 2", "VIP 3"};
    const auto accountTier = std::clamp(data.CurrentTier, 0, 3);
    if (model.current_tier != AccountNames[accountTier]) { model.current_tier = AccountNames[accountTier]; m_RmlView.MarkDirty("current_tier"); }
    const int selectedDays = data.CurrentTier == m_plan ? data.RemainingDays : 0;
    if (model.selected_days != selectedDays) { model.selected_days = selectedDays; m_RmlView.MarkDirty("selected_days"); }
    std::vector<Line> benefits, costs;
    Rml::String planName, duration;
    int section = 0;
    if (fresh)
    {
        for (const auto& source : data.Lines)
        {
            const Rml::String line = StringUtils::WideToNarrow(source.c_str());
            if (line == ">Beneficios") { section = 1; continue; }
            if (line == ">Costo") { section = 2; continue; }
            if (line == ">Resultado") { section = 3; continue; }
            if (line.rfind("Actual: ", 0) == 0 || line.rfind("Te quedan ", 0) == 0 || line == "VIP permanente") continue;
            if (line.rfind("Duraci", 0) == 0)
            {
                const auto colon = line.find(':');
                if (colon != Rml::String::npos) duration = line.substr(colon + 1);
                continue;
            }
            if (line.rfind("!Compra VIP", 0) == 0) continue;
            if (section == 0 && planName.empty()) planName = line;
            else if (section == 1 && line != "Suma los días restantes" && line != "Suma los dias restantes") benefits.push_back(ParseLine(line));
            else if (section == 2) costs.push_back(ParseLine(line));
        }
    }
    const bool empty = benefits.empty() && costs.empty();
    if (model.plan_name != planName) { model.plan_name = std::move(planName); m_RmlView.MarkDirty("plan_name"); }
    if (model.duration != duration) { model.duration = std::move(duration); m_RmlView.MarkDirty("duration"); }
    if (model.benefits != benefits) { model.benefits = std::move(benefits); m_RmlView.MarkDirty("benefits"); }
    if (model.costs != costs) { model.costs = std::move(costs); m_RmlView.MarkDirty("costs"); }
    if (model.empty != empty) { model.empty = empty; m_RmlView.MarkDirty("empty"); }
}
} // namespace mu::ui::window
