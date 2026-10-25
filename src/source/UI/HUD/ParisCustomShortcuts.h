#pragma once
#include "UI/HUD/ParisShortcutBindings.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Dialogs/GenericConfirmDialog.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "Render/RmlUi/RmlUiRuntime.h"
namespace UI::Shortcuts
{
inline Action Pressed()
{
    const bool control = mu::ui::window::IsPress(VK_CONTROL) || mu::ui::window::IsRepeat(VK_CONTROL);
    const bool alt = mu::ui::window::IsPress(VK_MENU) || mu::ui::window::IsRepeat(VK_MENU);
    const bool shift = mu::ui::window::IsPress(VK_SHIFT) || mu::ui::window::IsRepeat(VK_SHIFT);
    for (const auto& binding : Bindings)
        if (Resolve(binding.key, control, alt, shift) != Action::None && mu::ui::window::IsPress(binding.key))
            return binding.action;
    return Action::None;
}
template <typename Window> inline void Toggle(Window* window)
{
    if (!window) return;
    if (window->IsVisible()) window->CloseWindow();
    else window->OpenWindow();
}
inline void Execute(Action action)
{
    using namespace mu::ui::window;
    switch (action)
    {
    case Action::Vip:
        if (g_pParisVipWindow && !g_pParisVipWindow->IsVisible()) g_pNewUISystem->Show(INTERFACE_CHARACTER);
        Toggle(g_pParisVipWindow); break;
    case Action::Ranking: Toggle(g_pParisRankingWindow); break;
    case Action::Bank:
        if (g_pParisJewelBankWindow && !g_pParisJewelBankWindow->IsVisible())
            g_pNewUISystem->Show(INTERFACE_INVENTORY);
        Toggle(g_pParisJewelBankWindow); break;
    case Action::Info: Toggle(g_pParisAdvancedStatsWindow); break;
    case Action::Market: Toggle(g_pParisMarketWindow); break;
    case Action::Reset:
        if (g_pParisResetWindow && !g_pParisResetWindow->IsVisible()) g_pNewUISystem->Show(INTERFACE_CHARACTER);
        Toggle(g_pParisResetWindow); break;
    case Action::Events: Toggle(g_pParisEventsWindow); break;
    case Action::AddStats:
        if (g_pCharacterInfoWindow) g_pCharacterInfoWindow->TogglePointAllocation();
        break;
    case Action::BattlePass: Toggle(g_pParisBattlePassWindow); break;
    case Action::KillLog:
    case Action::PostLog:
        if (g_pParisActivityLogWindow) g_pParisActivityLogWindow->ToggleWindow(action == Action::KillLog);
        break;
    default: break;
    }
}

inline bool Handle()
{
    if (SceneFlag != MAIN_SCENE || GetFocus() != g_hWnd || !g_pNewUISystem || !g_pNewUIHotKey
        || g_pNewUIHotKey->IsStateGameOver() || g_pChatInputBox->HaveFocus()
        || !g_pNewUIHotKey->CanUpdateKeyEvent() || g_pNewUIHotKey->CanUpdateKeyEventRelatedMyInventory()
        || RmlUiRuntime::Instance().IsTextInputActive() || !g_MessageBox->IsEmpty()
        || mu::ui::window::g_pGenericConfirmDialog->IsVisible() || mu::ui::window::g_pGenericMenuDialog->IsVisible())
        return false;
    if (mu::ui::window::IsPress(0xC0) && mu::ui::window::g_pGestureWheelWindow)
    {
        mu::ui::window::g_pGestureWheelWindow->Toggle();
        return true;
    }
    const Action action = Pressed();
    if (action == Action::None) return false;
    Execute(action);
    return true;
}
}
