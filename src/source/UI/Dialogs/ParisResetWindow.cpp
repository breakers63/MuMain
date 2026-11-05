#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "UI/Dialogs/ParisResetWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "Network/Paris/ParisRequests.h"
#include "Network/Paris/ResetProtocol.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"
#include "UI/Dialogs/GenericConfirmDialog.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>

namespace mu::ui::window
{
namespace
{
constexpr DWORD InterfaceId = 0x50525354u;
DWORD ResetRequestStartedAt = 0;
}

CParisResetWindow* g_pParisResetWindow = nullptr;

CParisResetWindow::~CParisResetWindow() { Release(); }

bool CParisResetWindow::Create(CManager* manager)
{
    if (!manager) return false;
    m_manager = manager;
    manager->AddUIObj(InterfaceId, this);
    // Defer document/model creation until OpenWindow(), after the RmlUi context is available.
    CObject::Show(false);
    return true;
}

void CParisResetWindow::Release()
{
    m_RmlView.Release();
    if (!m_manager) return;
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CParisResetWindow::BindModel(Rml::DataModelConstructor& c, Model& model)
{
    auto line = c.RegisterStruct<Line>();
    line.RegisterMember("text", &Line::text);
    line.RegisterMember("state", &Line::state);
    c.RegisterArray<std::vector<Line>>();
    c.Bind("status", &model.status);
    c.Bind("reset_count", &model.reset_count);
    c.Bind("result", &model.result);
    c.Bind("information", &model.information);
    c.Bind("requirements", &model.requirements);
    c.Bind("rewards", &model.rewards);
    c.Bind("has_information", &model.has_information);
    c.Bind("has_requirements", &model.has_requirements);
    c.Bind("has_rewards", &model.has_rewards);
    c.Bind("has_data", &model.has_data);
    c.Bind("can_reset", &model.can_reset);
    c.Bind("pending", &model.pending);
    c.BindEventCallback("paris_reset_refresh", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RequestStatus(); });
    c.BindEventCallback("paris_reset_confirm", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ConfirmReset(); });
    c.BindEventCallback("paris_reset_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
    {
        mu::log::Get("ui")->info("Paris reset close event received");
        m_closeRequested = true;
    });
}

void CParisResetWindow::OpenWindow()
{
    mu::log::Get("ui")->info("Paris reset open begin, built={}", m_RmlView.IsBuilt());
    // Build the lazy Rml document before any path asks for its pointer or model.
    if (!m_RmlView.IsBuilt() && !m_RmlView.Ensure()) return;
    mu::log::Get("ui")->info("Paris reset open document ready");
    m_closeRequested = false;
    Show(true);
    RequestStatus();
}

void CParisResetWindow::CloseWindow() { m_closeRequested = false; Show(false); }

void CParisResetWindow::Show(bool show)
{
    if (show) CloseOtherParisWindows(this);
    CObject::Show(show);
    if (!m_RmlView.Document()) return;
    if (show) m_RmlView.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else m_RmlView.Document()->Hide();
}

bool CParisResetWindow::UpdateMouseEvent() { return !IsVisible(); }

bool CParisResetWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE)) { CloseWindow(); return false; }
    return !IsVisible();
}

bool CParisResetWindow::Update()
{
    if (m_closeRequested)
    {
        mu::log::Get("ui")->info("Paris reset close processed");
        CloseWindow();
    }
    return true;
}
bool CParisResetWindow::Render() { SyncModel(); return true; }

void CParisResetWindow::RequestStatus()
{
    m_receivedBeforeRequest = Network::Paris::Reset::GetSnapshot().second;
    ResetRequestStartedAt = GetTickCount();
    m_RmlView.GetModel().status = "Consultando requisitos y estado…";
    m_RmlView.MarkDirty("status");
    if (!Network::Paris::RequestReset(false, 0))
    {
        m_RmlView.GetModel().status = "No se pudo conectar al servidor.";
        m_RmlView.MarkDirty("status");
        mu::log::Get("ui")->warn("Paris reset status request could not be sent");
        return;
    }
    mu::log::Get("ui")->info("Paris reset status request sent");
}

void CParisResetWindow::ConfirmReset()
{
    auto [data, received] = Network::Paris::Reset::GetSnapshot();
    const bool fresh = received != 0
        && received != m_receivedBeforeRequest
        && timeGetTime() - received < 30000;
    if (!fresh || !data.CanReset || data.Token == 0 || !g_pGenericConfirmDialog) return;

    GenericDialogConfig confirm;
    confirm.title = L"Confirmar reset";
    confirm.lines.push_back({L"Se aplicará el reset autorizado por el servidor.", false});
    confirm.primaryLabel = L"Reiniciar";
    confirm.showCancel = true;
    confirm.cancelLabel = L"Volver";
    const auto token = data.Token;
    confirm.onPrimary = [token]
    {
        if (!Network::Paris::RequestReset(true, token) && g_pParisResetWindow)
            g_pParisResetWindow->RequestStatus();
    };
    g_pGenericConfirmDialog->Show(std::move(confirm));
}

void CParisResetWindow::SyncModel()
{
    if (!m_RmlView.Document()) return;
    auto& model = m_RmlView.GetModel();
    auto [data, received] = Network::Paris::Reset::GetSnapshot();
    const bool fresh = received != 0 && timeGetTime() - received < 30000;
    const bool updated = fresh && received != m_receivedBeforeRequest;
    const bool timedOut = !updated && ResetRequestStartedAt != 0
        && GetTickCount() - ResetRequestStartedAt > 10000;
    const Rml::String status = updated ? (data.Available ? "" : "Reset no disponible")
        : (timedOut ? "El servidor no respondió. Pulsa Reset para reintentar." : "Consultando el servidor…");
    if (model.status != status) { model.status = status; m_RmlView.MarkDirty("status"); }
    const bool hasData = updated;
    if (model.has_data != hasData) { model.has_data = hasData; m_RmlView.MarkDirty("has_data"); }
    const bool canReset = updated && data.CanReset;
    if (model.can_reset != canReset) { model.can_reset = canReset; m_RmlView.MarkDirty("can_reset"); }
    const Rml::String resetCount = std::to_string((std::max)(0, data.NextReset - 1));
    if (model.reset_count != resetCount) { model.reset_count = resetCount; m_RmlView.MarkDirty("reset_count"); }
    const bool pending = !updated;
    if (model.pending != pending) { model.pending = pending; m_RmlView.MarkDirty("pending"); }

    std::vector<Line> information;
    std::vector<Line> requirements;
    std::vector<Line> rewards;
    Rml::String result;
    int section = 0;
    if (updated)
    {
        for (const auto& source : data.Lines)
        {
            if (source.rfind(L"!Reset personalizado", 0) == 0 || source.rfind(L"!Espera", 0) == 0)
            {
                result = StringUtils::WideToNarrow(source.c_str() + 1);
                continue;
            }
            if (source == L">Requisitos") { section = 1; continue; }
            if (source == L">Después del reset") { section = 2; continue; }
            if (source == L">Recompensas") { section = 3; continue; }
            if (!source.empty() && source[0] == L'>') { section = 0; continue; }

            Line line{StringUtils::WideToNarrow(source.c_str()), ""};
            if (section == 1 && !source.empty() && (source[0] == L'+' || source[0] == L'!'))
            {
                line.text = StringUtils::WideToNarrow(source.c_str() + 1);
                line.state = source[0] == L'+' ? "good" : "bad";
            }
            if (section == 1)
                requirements.push_back(std::move(line));
            else if (section == 2)
                information.push_back(std::move(line));
            else if (section == 3)
            {
                if (source.find(L"puntos:") != std::wstring::npos)
                {
                    const auto colon = source.find(L':');
                    if (colon != std::wstring::npos)
                    {
                        const std::wstring renamed = L"Puntos LevelUP:" + source.substr(colon + 1);
                        line.text = StringUtils::WideToNarrow(renamed.c_str());
                    }
                }
                rewards.push_back(std::move(line));
            }
            else if (result.empty() && (source.rfind(L"No hay", 0) == 0 || source.rfind(L"!", 0) == 0))
                result = StringUtils::WideToNarrow(source.c_str() + (source[0] == L'!' ? 1 : 0));
        }
    }
    if (model.information != information) { model.information = std::move(information); m_RmlView.MarkDirty("information"); }
    if (model.requirements != requirements) { model.requirements = std::move(requirements); m_RmlView.MarkDirty("requirements"); }
    if (model.rewards != rewards) { model.rewards = std::move(rewards); m_RmlView.MarkDirty("rewards"); }
    const bool hasInformation = !model.information.empty();
    const bool hasRequirements = !model.requirements.empty();
    const bool hasRewards = !model.rewards.empty();
    if (model.has_information != hasInformation) { model.has_information = hasInformation; m_RmlView.MarkDirty("has_information"); }
    if (model.has_requirements != hasRequirements) { model.has_requirements = hasRequirements; m_RmlView.MarkDirty("has_requirements"); }
    if (model.has_rewards != hasRewards) { model.has_rewards = hasRewards; m_RmlView.MarkDirty("has_rewards"); }
    if (result.empty() && updated && !data.Available) result = "El servidor no habilitó el reset.";
    if (model.result != result) { model.result = std::move(result); m_RmlView.MarkDirty("result"); }
}
} // namespace mu::ui::window
