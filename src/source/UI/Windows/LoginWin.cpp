//*****************************************************************************
// File: LoginWin.cpp
//*****************************************************************************

#include "stdafx.h"
#include "UI/Windows/LoginWin.h"
#include "UI/Windows/LoginMainWin.h"
#include "UI/Windows/ServerSelWin.h"
#include "Core/Input/Input.h"
#include "UI/Core/SceneUICoordinator.h"
#include "UI/Windows/CreditWin.h"
#include "UI/Windows/SysMenuWin.h"
#include "UI/Windows/MsgWin.h"
#include "Character/CharMakeWin.h"
#include "Core/Globals/_enum.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Network/Reconnect/ReconnectManager.h"
#include "Scenes/SceneCore.h"
#include "I18N/All.h"

#include "Audio/DSPlaySound.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Dialogs/MessageBox.h"
#include "UI/Windows/RememberPasswordPrompt.h"

#include "Network/Server/ServerListManager.h"
#ifdef _WIN32
#include <dpapi.h>
#endif

#include "Data/GameConfig/GameConfig.h"
#include "Data/GameConfig/GameConfigConstants.h"

#include "Render/RmlUi/RmlUiRuntime.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlNativeText.h"
#include "UI/RmlBridge/RmlTheme.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlPasswordField.h"
#include "Core/Utilities/StringUtils.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <cmath>

extern unsigned int WindowWidth, WindowHeight;

namespace
{
// Scales the original's pixel sizes (the legacy bounding box) like login.rcss's own units
// (UI::RmlBridge::SceneWindowRatio()). Reads WindowWidth/WindowHeight, not
// CInput::Instance().GetScreenWidth/Height() -- the latter doesn't reliably match the values
// RmlUiRuntime::OnResize() uses, which caused the panel/input box to visibly drift off-position.
float LoginUIScaleRatio()
{
    return UI::RmlBridge::SceneWindowRatio(static_cast<int>(WindowWidth), static_cast<int>(WindowHeight));
}

    int ScaledOffset(int value, float ratio)
    {
        return static_cast<int>(std::lround(value * ratio));
    }
}

extern int g_iChatInputType;
extern int  LogIn;
extern wchar_t LogInID[MAX_USERNAME_SIZE + 1];
extern BYTE Version[SIZE_PROTOCOLVERSION];
extern BYTE Serial[SIZE_PROTOCOLSERIAL + 1];

CLoginWin g_LoginWin;

CLoginWin::CLoginWin() = default;

CLoginWin::~CLoginWin()
{
    // Backstop join: std::thread's destructor calls std::terminate() if still joinable, and
    // there's no guarantee ProcessPendingConnectionReconnect() ran one last time before shutdown.
    if (m_ConnectionReconnectThread.joinable())
        m_ConnectionReconnectThread.join();
}

void CLoginWin::Create()
{
    m_RememberMe = GameConfig::GetInstance().GetRememberMe();
    if (m_RememberMe)
    {
        // Use the helper we built to fill m_Username[11] and m_Password[21]
        GameConfig::GetInstance().DecryptCredentials(m_Username, m_Password, _countof(m_Username), _countof(m_Password));
    }
    else
    {
        // Ensure they are empty if RememberMe is off
        m_Username[0] = L'\0';
        m_Password[0] = L'\0';
    }

    // Tracks login.rcss's #panel width/height (329 x 245 original pixels) by the same ratio rather than a fixed
    // 329x245, so UpdateMouseEvent()'s hit-test rect (m_Size) doesn't go stale against RmlUi's
    // auto-fitting visuals.
    RefreshLayout();
    m_ptPos.x = m_ptPos.y = 0;

    // Tab order between the two fields is RmlUi's own document-level navigation (ElementDocument
    // handles KI_TAB, and WidgetTextInput deliberately lets it bubble), replacing the native boxes'
    // reciprocal SetTabTarget() pair.
    auto& model = m_RmlView.GetModel();
    model.username.clear();
    model.password.clear();
    if (m_RememberMe) {
        model.username = StringUtils::WideToNarrow(m_Username);
        model.password = StringUtils::WideToNarrow(m_Password);
        m_bRememberMeChecked = true;
    }
    // Create() runs on every entry to the login scene, but the document outlives it: push the
    // fresh values into the fields, or they keep what an earlier visit typed.
    m_RmlView.MarkDirty("username");
    m_RmlView.MarkDirty("password");

    // The password is only pre-filled and re-saved when the player previously
    // opted in on a trusted machine.
    m_bSavePasswordChecked = (m_RememberMe != 0) && GameConfig::GetInstance().GetSavePassword();

    // Seed the edit-detection snapshot with what we just loaded so filling the
    // fields here is not mistaken for the player editing them.
    wcsncpy(m_prevUsername, m_RememberMe ? m_Username : L"", _countof(m_prevUsername) - 1);
    wcsncpy(m_prevPassword, m_RememberMe ? m_Password : L"", _countof(m_prevPassword) - 1);

    this->FirstLoad = 1;

    // Builds once; Create() re-runs on every resolution change and only repositions it afterward
    // (see SetPosition()).
    m_RmlView.Ensure();

    CSceneUICoordinator::Instance().GetNewStyleMng().AddUIObj(mu::ui::window::INTERFACE_LOGIN, this);
    Show(false);
}

void CLoginWin::BindRmlModel(Rml::DataModelConstructor& c, LoginRmlModel& model)
{
    BindSavedAccounts(c, model);
    c.Bind("remember_me_checked", &model.rememberMeChecked);
    c.Bind("save_password_checked", &model.savePasswordChecked);
    c.Bind("server_name", &model.serverName);
    c.Bind("username", &model.username);
    c.Bind("password", &model.password);

    c.BindEventCallback("login_ok_click",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RmlClickOk(); });
    c.BindEventCallback("login_cancel_click",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RmlClickCancel(); });
    c.BindEventCallback("login_toggle_remember_me",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RmlToggleRememberMe(); });
    c.BindEventCallback("login_toggle_save_password",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { RmlToggleSavePassword(); });
    c.BindEventCallback("login_toggle_password_visibility",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { TogglePasswordVisibility(); });
}

void CLoginWin::TogglePasswordVisibility()
{
    if (!IsVisible() || !m_RmlView.Document())
        return;
    SetPasswordVisible(!m_passwordVisible);
    m_passwordFocusPending = true;
}

void CLoginWin::SetPasswordVisible(bool visible)
{
    m_passwordVisible = visible;
    if (!m_RmlView.Document())
        return;
    UI::RmlBridge::SetPasswordVisible(m_RmlView.Document()->GetElementById("input_password"),
        m_RmlView.Document()->GetElementById("input_password_visible"), visible);
}

void CLoginWin::FocusPendingPassword()
{
    if (!m_passwordFocusPending || !m_RmlView.Document())
        return;
    const char* id = m_passwordVisible ? "input_password_visible" : "input_password";
    Rml::Element* field = m_RmlView.Document()->GetElementById(id);
    if (field && field->IsVisible() && field->Focus())
        m_passwordFocusPending = false;
}

void CLoginWin::OnRmlReloaded()
{
    // The panel's size follows the theme's units (LoginUIScaleRatio()); keep it where
    // CSceneUICoordinator::CreateLoginScene() places it: centred, 2/3 down the free height.
    const SIZE previousSize = m_Size;
    const float uiScale = LoginUIScaleRatio();
    m_Size.cx = ScaledOffset(329, uiScale);
    m_Size.cy = ScaledOffset(245, uiScale);
    SetPosition(m_ptPos.x + (previousSize.cx - m_Size.cx) / 2, m_ptPos.y + (previousSize.cy - m_Size.cy) * 2 / 3);
}

void CLoginWin::RefreshLayout()
{
    constexpr int panelWidth = 329;
    constexpr int panelHeight = 245;
    const float uiScale = LoginUIScaleRatio();
    m_Size.cx = ScaledOffset(panelWidth, uiScale);
    m_Size.cy = ScaledOffset(panelHeight, uiScale);
}

void CLoginWin::SetPosition(int x, int y)
{
	m_ptPos.x = x;
	m_ptPos.y = y;

	// No field repositioning here: both credential fields are RmlUi elements positioned by each
	// theme's RCSS, so moving #panel carries them along.

	// RmlUi panel origin: real window pixels, no scale conversion needed (RmlUi's Context already
	// operates in real pixels; only the panel's own size/children are dp, scaled by RmlUi itself).
	if (m_RmlView.Document())
	{
		if (Rml::Element* panel = m_RmlView.Document()->GetElementById("panel"))
		{
			// Responsive themes anchor the panel to the viewport in RCSS.
			if (panel->IsClassSet("viewport-anchored"))
				return;
			panel->SetProperty("left", std::to_string(x) + "px");
			panel->SetProperty("top", std::to_string(y) + "px");
		}
	}
}

void CLoginWin::Show(bool bShow)
{
    mu::ui::window::CObject::Show(bShow);

    // Hiding the document unfocuses it (Context::UnfocusDocument()), which is what releases SDL
    // text input now that the fields are RmlUi's -- no separate widget state to drive (#447).
    if (m_RmlView.Document())
    {
        if (bShow) { RefreshSavedAccounts(); SyncRmlModel(); m_RmlView.Document()->Show(); }
        else
        {
            SetPasswordVisible(false);
            m_passwordFocusPending = false;
            m_RmlView.Document()->Hide();
        }
    }
}

void CLoginWin::Release()
{
    // Hide, not unload -- the document/model are created once and reused.
    SetPasswordVisible(false);
    m_passwordFocusPending = false;
    m_RmlView.Hide();
}

void CLoginWin::RmlClickOk() { SubmitLogin(); }
void CLoginWin::RmlClickCancel() { SubmitCancel(); }

void CLoginWin::RmlToggleRememberMe()
{
	m_bRememberMeChecked = !m_bRememberMeChecked;
	if (UI::Login::RememberPasswordChoiceState() != UI::Login::RememberPasswordChoice::Pending)
		ApplyRememberMeChange();
}

void CLoginWin::RmlToggleSavePassword()
{
	m_bSavePasswordChecked = !m_bSavePasswordChecked;
	if (UI::Login::RememberPasswordChoiceState() != UI::Login::RememberPasswordChoice::Pending)
		ApplySavePasswordChange();
}

bool CLoginWin::UpdateWhileActive()
{
	if (CInput::Instance().IsKeyDown(VK_RETURN))
	{
		SubmitLogin();
		return true;
	}

	if (CInput::Instance().IsKeyDown(VK_ESCAPE))
	{
		SubmitCancel();
		return true;
	}

	return true;
}

// Called both from the immediate RmlUi click callbacks and from UpdateWhileActive()'s keyboard
// polling, so each re-checks the "remember password" prompt's Pending state itself.
void CLoginWin::SubmitLogin()
{
	if (UI::Login::RememberPasswordChoiceState() == UI::Login::RememberPasswordChoice::Pending)
		return;
	PlayBuffer(SOUND_CLICK01);
	RequestLogin();
}

void CLoginWin::SubmitCancel()
{
	if (UI::Login::RememberPasswordChoiceState() == UI::Login::RememberPasswordChoice::Pending)
		return;
	PlayBuffer(SOUND_CLICK01);
	CancelLogin();
}

void CLoginWin::ApplyRememberMeChange()
{
	GameConfig& config = GameConfig::GetInstance();
	m_RememberMe = m_bRememberMeChecked;
	config.SetRememberMe(m_RememberMe != 0);

	// Switching off "remember me" revokes everything: drop the stored
	// credentials from config.ini now so they can't linger if the game is
	// closed before the next login.
	if (!m_RememberMe)
	{
		m_bSavePasswordChecked = false;
		config.ClearCredentials();
	}
}

void CLoginWin::ApplySavePasswordChange()
{
	GameConfig& config = GameConfig::GetInstance();

	if (!m_bSavePasswordChecked)
	{
		// Player unticked it: drop the stored password from config.ini now.
		config.SetSavePassword(false);
		config.SetEncryptedPassword(L"");
		config.Save();
		return;
	}

	// Enabling requires confirmation. Revert the tick immediately; it is
	// re-applied only if the dialog is accepted, so a cancel (however the dialog
	// closes) always leaves the box unchecked.
	m_bSavePasswordChecked = false;

	// Storing the password implies remembering the account.
	if (!m_RememberMe)
	{
		m_RememberMe = 1;
		m_bRememberMeChecked = true;
		config.SetRememberMe(true);
	}
	UI::Login::OpenRememberPasswordPrompt();
}

bool CLoginWin::UpdateWhileShown()
{
    // Prevents Enter/Esc from double-firing past an overlaying modal: a higher-depth modal
    // claiming UpdateMouseEvent() doesn't by itself stop this window's own VK_RETURN/VK_ESCAPE
    // polling in UpdateWhileActive(). WasSysMenuToggledByEscThisFrame() is needed in addition to
    // g_SysMenuWin.IsVisible() because an Esc that just closed the menu this frame already reads
    // back IsVisible()==false. g_CharMakeWin.IsVisible() covers the same gap: this window keeps
    // ticking on the character-select scene too, so without it, Esc closing Character Create also
    // fires this window's own SubmitCancel() and logs out unexpectedly.
    SetActive(!(g_CreditWin.IsVisible() || g_MsgWin.IsVisible() || g_SysMenuWin.IsVisible()
                || g_CharMakeWin.IsVisible()
                || CSceneUICoordinator::Instance().WasSysMenuToggledByEscThisFrame()
                || UI::Login::RememberPasswordChoiceState() == UI::Login::RememberPasswordChoice::Pending));

    // Polls the "Remember Password" dialog's own Enter/Esc while it's open.
    UI::Login::Tick();

    ApplyRememberPasswordChoice();
    RevokeSavedCredentialsIfEdited();
    FocusPendingPassword();
    return true;
}

void CLoginWin::ApplyRememberPasswordChoice()
{
    // Applied here, not UpdateWhileActive: the modal leaves this window inactive, but
    // UpdateWhileShown keeps being called.
    const UI::Login::RememberPasswordChoice choice = UI::Login::RememberPasswordChoiceState();
    if (choice != UI::Login::RememberPasswordChoice::Ok
        && choice != UI::Login::RememberPasswordChoice::Cancel)
        return;

    UI::Login::ClearRememberPasswordChoice();

    const bool bAccepted = (choice == UI::Login::RememberPasswordChoice::Ok);
    GameConfig::GetInstance().SetSavePassword(bAccepted);
    m_bSavePasswordChecked = bAccepted;
}

void CLoginWin::RevokeSavedCredentialsIfEdited()
{
    // Drop the obsolete last-login credentials when editing, while retaining the
    // checkbox choice the player explicitly accepted for this form's next login.
    const auto& model = m_RmlView.GetModel();
    const std::wstring curUser = StringUtils::NarrowToWide(model.username);
    const std::wstring curPass = StringUtils::NarrowToWide(model.password);

    if (curUser == m_prevUsername && curPass == m_prevPassword)
        return;

    GameConfig& config = GameConfig::GetInstance();
    const bool bHadStored = !config.GetEncryptedUsername().empty()
        || !config.GetEncryptedPassword().empty();
    if (bHadStored)
    {
        config.ClearCredentials();
    }

    wcsncpy(m_prevUsername, curUser.c_str(), _countof(m_prevUsername) - 1);
    m_prevUsername[_countof(m_prevUsername) - 1] = L'\0';
    wcsncpy(m_prevPassword, curPass.c_str(), _countof(m_prevPassword) - 1);
    m_prevPassword[_countof(m_prevPassword) - 1] = L'\0';
}

bool CLoginWin::Render()
{
    if (FirstLoad)
    {
        // Land on whichever field the player still has to fill in.
        if (wcslen(m_Username) > 0)
            FocusPassword();
        else
            FocusUsername();
        FirstLoad = 0;
    }

    // g_CreditWin renders via plain CSprite/g_pRenderText, strictly before RmlUi's frame-final
    // document render, so login.rml's panel would otherwise always paint over it. RmlUi always
    // renders last regardless of GetLayerDepth(), so this must be toggled every frame rather than
    // just on Show().
    const bool coveredByCredits = g_CreditWin.IsVisible();
    // Through the transition-only helper, not a bare Show(): this runs every frame, and
    // ElementDocument::Show() defaults to FocusFlag::Auto, which would hand focus back to the
    // document and blur #input_account/#input_password on every single frame.
    UI::RmlBridge::SyncDocumentVisibility(m_RmlView.Document(), !coveredByCredits);


    // Everything renders via the RmlUi overlay now, the two credential fields included, so there's
    // no native text pass left here -- and no need for the old g_SysMenuWin guard, which existed
    // only because raw CUITextInputBox pixels ignored RmlUi's own document ordering.
    SyncRmlModel();

    return true;
}

void CLoginWin::ApplyCredentialLimits()
{
    if (!m_RmlView.Document()) return;

    if (Rml::Element* account = m_RmlView.Document()->GetElementById("input_account"))
        account->SetAttribute("maxlength", static_cast<int>(MAX_USERNAME_SIZE));
    if (Rml::Element* password = m_RmlView.Document()->GetElementById("input_password"))
        password->SetAttribute("maxlength", static_cast<int>(MAX_PASSWORD_SIZE));
    if (Rml::Element* plain = m_RmlView.Document()->GetElementById("input_password_visible"))
        plain->SetAttribute("maxlength", static_cast<int>(MAX_PASSWORD_SIZE));
    SetPasswordVisible(m_passwordVisible);
}

void CLoginWin::FocusCredentialField(const char* elementId, bool selectAll)
{
    if (!m_RmlView.Document()) return;

    Rml::Element* field = m_RmlView.Document()->GetElementById(elementId);
    if (field == nullptr || !field->Focus())
        return;

    // Reproduces the native GiveFocus(TRUE): pre-select the text so a retry after a failed login
    // overwrites it in one keystroke instead of appending to it.
    if (selectAll)
    {
        if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(field))
            input->Select();
    }
}

void CLoginWin::FocusUsername(bool selectAll)
{
    FocusCredentialField("input_account", selectAll);
}

void CLoginWin::FocusPassword(bool selectAll)
{
    FocusCredentialField(m_passwordVisible ? "input_password_visible" : "input_password", selectAll);
}

void CLoginWin::SyncRmlModel()
{
    if (!m_RmlView.Document()) return;

    const bool rememberChecked = m_bRememberMeChecked;
    if (m_RmlView.GetModel().rememberMeChecked != rememberChecked)
    {
        m_RmlView.GetModel().rememberMeChecked = rememberChecked;
        m_RmlView.MarkDirty("remember_me_checked");
    }

    const bool saveChecked = m_bSavePasswordChecked;
    if (m_RmlView.GetModel().savePasswordChecked != saveChecked)
    {
        m_RmlView.GetModel().savePasswordChecked = saveChecked;
        m_RmlView.MarkDirty("save_password_checked");
    }

    wchar_t szServerName[MAX_TEXT_LENGTH] = {};
    constexpr wchar_t serverCaption[] = L"%ls-%d · Servidor";
    mu_swprintf(szServerName, serverCaption, g_ServerListManager->GetSelectServerName(), g_ServerListManager->GetSelectServerIndex());
    const std::string serverNameUtf8 = StringUtils::WideToNarrow(szServerName);
    if (m_RmlView.GetModel().serverName != serverNameUtf8)
    {
        m_RmlView.GetModel().serverName = serverNameUtf8;
        m_RmlView.MarkDirty("server_name");
    }


}

bool CLoginWin::UpdateMouseEvent()
{
    if (!IsVisible())
        return true;
    return !UI::RmlBridge::IsPointerOver(m_RmlView.Document());
}

void CLoginWin::RequestLogin()
{
    if (CurrentProtocolState == REQUEST_JOIN_SERVER)
        return;

    const auto& model = m_RmlView.GetModel();
    wcsncpy(m_Username, StringUtils::NarrowToWide(model.username).c_str(), _countof(m_Username) - 1);
    m_Username[_countof(m_Username) - 1] = L'\0';
    wcsncpy(m_Password, StringUtils::NarrowToWide(model.password).c_str(), _countof(m_Password) - 1);
    m_Password[_countof(m_Password) - 1] = L'\0';

    if (wcslen(m_Username) <= 0)
    {
        Show(false);
        CSceneUICoordinator::Instance().PopUpMsgWin(MESSAGE_INPUT_ID);
    }
    else if (wcslen(m_Password) <= 0)
    {
        Show(false);
        CSceneUICoordinator::Instance().PopUpMsgWin(MESSAGE_INPUT_PASSWORD);
    }
    else
    {
        SaveLoginPreferences();
        SubmitCredentials(m_Username, m_Password);
    }
}

void CLoginWin::SaveLoginPreferences()
{
    GameConfig& config = GameConfig::GetInstance();
    config.SetRememberMe(m_bRememberMeChecked);
    if (!m_bRememberMeChecked)
    {
        config.ClearCredentials();
        return;
    }
    SaveCurrentAccount(true);
    config.SetSavePassword(m_bSavePasswordChecked);
    config.EncryptAndSaveCredentials(m_Username, m_Password);
}

void CLoginWin::SubmitCredentials(const wchar_t* pszUsername, const wchar_t* pszPassword)
{
    if (CurrentProtocolState != RECEIVE_JOIN_SERVER_SUCCESS)
        return;

    if (pszUsername == nullptr || pszPassword == nullptr)
        return;

    // Put the login screen away before the request goes out. The manual path
    // hides it on the click; a caller that submits credentials directly must
    // not leave it up, or its text fields keep the keyboard focus and no key
    // reaches the game afterwards. Hiding an already hidden window is a no-op.
    Show(false);

    g_ConsoleDebug->Write(MCD_NORMAL, L"Login with the following account: %ls", pszUsername);

    g_ErrorReport.Write(L"> Login Request.\r\n");
    g_ErrorReport.Write(L"> Try to Login \"%ls\"\r\n", pszUsername);

    LogIn = 1;
    // The destination is a fixed-size global. The click path arrives here with
    // a string the input box already bounded, but this entry point is public:
    // anything longer is truncated rather than written past the end.
    wcsncpy(LogInID, pszUsername, MAX_USERNAME_SIZE);
    LogInID[MAX_USERNAME_SIZE] = L'\0';
    CurrentProtocolState = REQUEST_LOG_IN;

    SocketClient->ToGameServer()->SendLogin(pszUsername, pszPassword, Version, Serial);

    // Keep the credentials in memory so auto-reconnect can re-login
    // without prompting after an in-game disconnect.
    ReconnectManager::Instance().CacheCredentials(pszUsername, pszPassword);

    g_pSystemLogBox->AddText(I18N::Game::VerifyingYourAccount, mu::ui::window::TYPE_SYSTEM_MESSAGE);
    g_pSystemLogBox->AddText(I18N::Game::PleaseWait, mu::ui::window::TYPE_SYSTEM_MESSAGE);
}

void CLoginWin::CancelLogin()
{
    // Hide immediately, run the blocking reconnect on a background thread. Guard against ESC-spam
    // stacking threads: if one is already in flight, this press is a no-op.
    Show(false);
    // Keep the animated login world and its menu visible. The selector is restored only
    // when a fresh ConnectServer list arrives, so a stale server cannot be clicked.
    g_ServerSelWin.Show(false);
    g_LoginMainWin.Show(true);

    if (m_bConnectionReconnectInFlight.load())
        return;

    if (m_ConnectionReconnectThread.joinable())
        m_ConnectionReconnectThread.join(); // previous run already finished and was consumed

    m_bConnectionReconnectInFlight.store(true);
    m_bConnectionReconnectDone.store(false);
    m_ConnectionReconnectThread = std::thread([this]
    {
        ConnectConnectionServer();
        m_bConnectionReconnectDone.store(true);
    });
}

void CLoginWin::ProcessPendingConnectionReconnect()
{
    if (m_ConnectionReconnectThread.joinable())
        m_ConnectionReconnectThread.join();
    m_bConnectionReconnectDone.store(false);
    m_bConnectionReconnectInFlight.store(false);
}

void CLoginWin::ConnectConnectionServer()
{
    LogIn = 0;
    CurrentProtocolState = REQUEST_JOIN_SERVER;
    CreateSocket(szServerIpAddress, g_ServerPort);
}
