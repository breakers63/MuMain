#include "stdafx.h"
#include "ActiveInvasionPanel.h"
#include "CustomFeaturesWindow.h"
#include "ActivityLogView.h"
#include "ParisActivityLogs.h"
#include "ParisOverlayInput.h"
#include "MentorPanel.h"
#include "SoulPanel.h"
#include "AuctionPanel.h"
#include "GmEventsPanel.h"
#include "UI/NewUI/Character/ParisRankUser.h"
#include "UI/NewUI/Quests/NewUICustomQuestCenter.h"
#include "UI/NewUI/NewUISystem.h"
#include "Network/Server/WSclient.h"

namespace SEASON3B
{
namespace
{
constexpr DWORD InterfaceKey = 0x50465857;
}
CParisFeatureWindow::~CParisFeatureWindow() { Release(); }
bool CParisFeatureWindow::Create(CNewUIManager* manager, CNewUI3DRenderMng* renderManager)
{
    if (!manager || !renderManager) return false;
    SetLayoutMode(UI::Scaling::LayoutMode::WorldOverlay);
    m_manager = manager;
    m_renderManager = renderManager;
    manager->AddUIObj(InterfaceKey, this);
    renderManager->Add3DRenderObj(this);
    UI::Invasions::Load();
    UI::ActivityLogs::Load();
    UI::Mentor::Load();
    UI::Soul::Load();
    UI::Auctions::Load();
    UI::GmEvents::Load();
    return true;
}
void CParisFeatureWindow::Release()
{
    if (!m_manager) return;
    UI::Invasions::Unload();
    UI::GmEvents::Unload();
    UI::Auctions::Unload();
    UI::Soul::Unload();
    UI::Mentor::Unload();
    UI::ActivityLogs::Unload();
    m_renderManager->Remove3DRenderObj(this);
    m_renderManager = nullptr;
    m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CParisFeatureWindow::Close()
{
    UI::Invasions::Close();
    UI::ActivityLogs::Close();
    UI::Mentor::Close();
    UI::Soul::Close();
    UI::Auctions::Close();
    UI::GmEvents::Close();
}
void CParisFeatureWindow::Open(int action)
{
    Close();
    switch (action)
    {
    case 10: UI::ActivityLogs::Toggle(UI::ActivityLogs::Kind::Kills); break;
    case 11: UI::ActivityLogs::Toggle(UI::ActivityLogs::Kind::Posts); break;
    case 12: UI::Mentor::Toggle(); break;
    case 13: UI::GmEvents::Toggle(); break;
    case 14: UI::Soul::Toggle(); break;
    case 15: UI::Auctions::Toggle(); break;
    }
}
bool CParisFeatureWindow::AnyOpen()
{
    return UI::Invasions::IsOpen() || UI::ActivityLogs::Visible != UI::ActivityLogs::Kind::None || UI::Mentor::IsOpen()
        || UI::Soul::IsOpen() || UI::Auctions::IsOpen() || UI::GmEvents::IsOpen();
}
bool CParisFeatureWindow::UpdateMouseEvent()
{
    if (!AnyOpen() && UI::OverlayInput::BlocksWorld()) return false;
    return !(UI::Invasions::Mouse() || UI::ActivityLogs::Mouse() || UI::Mentor::Mouse() || UI::Soul::Mouse()
        || UI::Auctions::Mouse() || UI::GmEvents::Mouse());
}
bool CParisFeatureWindow::UpdateKeyEvent()
{
    if (!AnyOpen() || !IsPress(VK_ESCAPE)) return true;
    Close();
    UI::OverlayInput::Capture();
    return false;
}
bool CParisFeatureWindow::Update()
{
    if (SceneFlag != MAIN_SCENE || !SocketClient || !SocketClient->IsConnected()) { Close(); UI::Invasions::Reset(); }
    return true;
}
bool CParisFeatureWindow::Render()
{
    UI::Invasions::Render();
    UI::GmEvents::Render();
    UI::ActivityLogs::Render();
    UI::Mentor::Render();
    UI::Soul::Render();
    UI::Auctions::Render();
    if (!AnyOpen()) UI::Character::RankUser::Render();
    DisableAlphaBlend();
    return true;
}
void CParisFeatureWindow::Render3D()
{
    UI::Invasions::Render3D();
    if (UI::ActivityLogs::Visible != UI::ActivityLogs::Kind::None) UI::ActivityLogs::ActivityLog().Render3D();
    UI::Mentor::Render3D();
    UI::Auctions::Render3D();
    if (g_pCustomQuestCenter && g_pCustomQuestCenter->IsWindowOpen()) g_pCustomQuestCenter->RenderRewardItems3D();
}
bool CParisFeatureWindow::IsVisible() const { return m_manager && SceneFlag == MAIN_SCENE; }
CNewUIObj* CParisFeatureWindow::GetLayoutOwner() const { return const_cast<CParisFeatureWindow*>(this); }
float CParisFeatureWindow::GetLayerDepth() { return 11.5f; }
float CParisFeatureWindow::GetKeyEventOrder() { return 1.f; }
}
