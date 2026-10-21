#include "stdafx.h"
#include "UI/Windows/LoginWin.h"

#include "Core/Utilities/StringUtils.h"
#include "Data/GameConfig/GameConfig.h"
#include "Data/GameConfig/SavedLoginAccountSlots.h"
#include <array>
#include "UI/Windows/RememberPasswordPrompt.h"

#include <RmlUi/Core/Event.h>

void CLoginWin::BindSavedAccounts(Rml::DataModelConstructor& c, LoginRmlModel& model)
{
    auto row = c.RegisterStruct<SavedAccountRow>();
    row.RegisterMember("index", &SavedAccountRow::index);
    row.RegisterMember("label", &SavedAccountRow::label);
    row.RegisterMember("occupied", &SavedAccountRow::occupied);
    row.RegisterMember("selected", &SavedAccountRow::selected);
    row.RegisterMember("password_saved", &SavedAccountRow::passwordSaved);
    c.RegisterArray<std::vector<SavedAccountRow>>();
    c.Bind("saved_accounts", &model.savedAccounts);
    c.Bind("saved_account_count", &model.savedAccountCount);
    c.Bind("account_status", &model.accountStatus);
    c.BindEventCallback("login_save_account", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        { SaveCurrentAccount(); });
    c.BindEventCallback("login_select_account", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        { if (args.size() == 1) SelectSavedAccount(args[0].Get<int>(-1)); });
    c.BindEventCallback("login_delete_account", [this](Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList& args)
        { event.StopPropagation(); if (args.size() == 1) DeleteSavedAccount(args[0].Get<int>(-1)); });
}

void CLoginWin::RefreshSavedAccounts()
{
    auto& model = m_RmlView.GetModel();
    model.savedAccounts.clear();
    int count = 0;
    for (std::size_t slot = 0; slot < SavedLoginAccountLimit; ++slot)
    {
        const SavedLoginAccount account = GameConfig::GetInstance().ReadSavedLoginAccount(slot);
        const bool occupied = !account.username.empty();
        count += occupied ? 1 : 0;
        model.savedAccounts.push_back({static_cast<int>(slot),
            occupied ? StringUtils::WideToNarrow(account.username.c_str()) : "Espacio disponible",
            occupied, static_cast<int>(slot) == m_selectedAccountSlot, !account.password.empty()});
    }
    model.savedAccountCount = std::to_string(count) + " / " + std::to_string(SavedLoginAccountLimit);
    m_RmlView.MarkDirty("saved_accounts");
    m_RmlView.MarkDirty("saved_account_count");
}

void CLoginWin::SetAccountStatus(const char* text)
{
    m_RmlView.GetModel().accountStatus = text;
    m_RmlView.MarkDirty("account_status");
}

void CLoginWin::SnapshotCredentials()
{
    const auto& model = m_RmlView.GetModel();
    wcsncpy_s(m_prevUsername, StringUtils::NarrowToWide(model.username).c_str(), _TRUNCATE);
    wcsncpy_s(m_prevPassword, StringUtils::NarrowToWide(model.password).c_str(), _TRUNCATE);
}

void CLoginWin::SelectSavedAccount(int slot)
{
    if (!IsVisible() || slot < 0 || slot >= static_cast<int>(SavedLoginAccountLimit)
        || UI::Login::RememberPasswordChoiceState() == UI::Login::RememberPasswordChoice::Pending)
        return;
    m_selectedAccountSlot = slot;
    const SavedLoginAccount account = GameConfig::GetInstance().ReadSavedLoginAccount(slot);
    if (!account.username.empty())
    {
        auto& model = m_RmlView.GetModel();
        model.username = StringUtils::WideToNarrow(account.username.c_str());
        model.password = StringUtils::WideToNarrow(account.password.c_str());
        m_bRememberMeChecked = true;
        m_RememberMe = 1;
        GameConfig::GetInstance().SetRememberMe(true);
        m_bSavePasswordChecked = !account.password.empty();
        SnapshotCredentials();
        m_RmlView.MarkDirty("username");
        m_RmlView.MarkDirty("password");
        FocusPassword();
    }
    RefreshSavedAccounts();
    SetAccountStatus(account.username.empty() ? "Escribe los datos y pulsa GUARDAR CUENTA." : "Cuenta seleccionada.");
}

void CLoginWin::SaveCurrentAccount(bool automatic)
{
    if (!IsVisible() || UI::Login::RememberPasswordChoiceState() == UI::Login::RememberPasswordChoice::Pending)
        return;
    const auto& model = m_RmlView.GetModel();
    const SavedLoginAccount account{StringUtils::NarrowToWide(model.username), StringUtils::NarrowToWide(model.password)};
    if (account.username.empty() || account.username.size() > MAX_USERNAME_SIZE || account.password.size() > MAX_PASSWORD_SIZE)
    {
        SetAccountStatus("Escribe una cuenta valida antes de guardar.");
        return;
    }
    std::array<SavedLoginAccount, SavedLoginAccountLimit> accounts;
    for (std::size_t slot = 0; slot < accounts.size(); ++slot)
        accounts[slot] = GameConfig::GetInstance().ReadSavedLoginAccount(slot);
    const int target = Data::Login::ChooseAccountSlot(accounts, account.username, m_selectedAccountSlot, !automatic);
    if (target < 0)
    {
        SetAccountStatus("Sin espacios libres. Selecciona una cuenta para reemplazarla.");
        return;
    }
    if (!GameConfig::GetInstance().SaveLoginAccount(target, account, m_bSavePasswordChecked))
    {
        SetAccountStatus("No se pudo guardar. Comprueba los datos de la cuenta.");
        return;
    }
    m_selectedAccountSlot = target;
    RefreshSavedAccounts();
    SetAccountStatus("Cuenta guardada en este equipo.");
}

void CLoginWin::DeleteSavedAccount(int slot)
{
    if (!IsVisible() || slot < 0 || slot >= static_cast<int>(SavedLoginAccountLimit)
        || UI::Login::RememberPasswordChoiceState() == UI::Login::RememberPasswordChoice::Pending)
        return;
    GameConfig::GetInstance().DeleteLoginAccount(slot);
    if (m_selectedAccountSlot == slot)
        m_selectedAccountSlot = -1;
    RefreshSavedAccounts();
    SetAccountStatus("Cuenta eliminada de la lista.");
}
