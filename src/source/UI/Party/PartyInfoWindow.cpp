
#include "stdafx.h"
#include "Network/Paris/PartyFinderProtocol.h"
#include "GameLogic/Social/PartyFollow.h"
#include "I18N/All.h"

#include "UI/Party/PartyInfoWindow.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Core/WindowGeometry.h"
#include "UI/Scaling/UITransform.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlWindowClose.h"
#include "UI/RmlBridge/RmlNativeTextSize.h"
#include "UI/RmlBridge/RmlTheme.h"
#include "Render/RmlUi/RmlUiRuntime.h"
#include "Core/Utilities/StringUtils.h"
#include "GameLogic/Events/CSChaosCastle.h"
#include "Audio/DSPlaySound.h"
#include "GameLogic/Events/w_CursedTemple.h"
#include "World/MapInfra/MapManager.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

using namespace SEASON3B;
using namespace mu::ui::window;

CPartyInfoWindow::CPartyInfoWindow()
{
    m_pNewUIMng = NULL;
    m_bParty = false;
    m_iSelectedCharID = -1;
}

CPartyInfoWindow::~CPartyInfoWindow()
{
    Release();
}

bool CPartyInfoWindow::Create(CManager* pNewUIMng)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(mu::ui::window::INTERFACE_PARTY, this);

    LoadImages();

    if (RmlUiRuntime::Instance().IsCreated())
    {
        BuildRmlUi();
    }

    Show(false);

    return true;
}

void CPartyInfoWindow::BindRmlModel(Rml::DataModelConstructor& c, PartyInfoRmlModel& model)
{
    c.Bind("text_px", &model.textPx);

    c.Bind("has_party", &model.hasParty);
    c.Bind("window_title", &model.windowTitle);
    c.Bind("exit_tooltip", &model.exitTooltip);

    c.RegisterArray<std::vector<Rml::String>>();
    auto member = c.RegisterStruct<PartyMemberRow>();
    member.RegisterMember("name", &PartyMemberRow::name);
    member.RegisterMember("map_text", &PartyMemberRow::mapText);
    member.RegisterMember("coord_text", &PartyMemberRow::coordText);
    member.RegisterMember("hp_text", &PartyMemberRow::hpText);
    member.RegisterMember("hp_percent", &PartyMemberRow::hpPercent);
    member.RegisterMember("mana_text", &PartyMemberRow::manaText);
    member.RegisterMember("mana_percent", &PartyMemberRow::manaPercent);
    member.RegisterMember("level_text", &PartyMemberRow::levelText);
    member.RegisterMember("buffs", &PartyMemberRow::buffs);
    member.RegisterMember("is_leader", &PartyMemberRow::isLeader);
    member.RegisterMember("show_kick", &PartyMemberRow::showKick);
    member.RegisterMember("can_follow", &PartyMemberRow::canFollow);
    member.RegisterMember("can_transfer", &PartyMemberRow::canTransfer);
    member.RegisterMember("follow_label", &PartyMemberRow::followLabel);
    member.RegisterMember("index", &PartyMemberRow::index);
    c.RegisterArray<std::vector<PartyMemberRow>>();
    c.Bind("members", &model.members);

    UI::RmlBridge::BindWindowClose(c, mu::ui::window::INTERFACE_PARTY);
    c.BindEventCallback("party_kick_member",
        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& arguments)
        {
            if (arguments.size() == 1)
                RmlClickKickMember(arguments[0].Get<int>(-1));
        });

    auto finder = c.RegisterStruct<FinderRow>();
    finder.RegisterMember("id", &FinderRow::id);
    finder.RegisterMember("leader", &FinderRow::leader);
    finder.RegisterMember("map", &FinderRow::map);
    finder.RegisterMember("slots", &FinderRow::slots);
    finder.RegisterMember("full", &FinderRow::full);
    c.RegisterArray<std::vector<FinderRow>>();
    c.Bind("finder_parties", &model.finderParties);
    c.Bind("is_leader", &model.isLeader);
    c.Bind("is_public", &model.isPublic);
    c.Bind("auto_accept_party", &model.autoAcceptParty);
    c.Bind("public_option_label", &model.publicOptionLabel);
    c.Bind("follow_option_label", &model.followOptionLabel);
    c.Bind("auto_option_label", &model.autoOptionLabel);
    model.publicOptionLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderMakePublic);
    model.followOptionLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderAllowFollow);
    model.autoOptionLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderAutoAcceptEnable);
    c.Bind("show_buffs", &model.showBuffs);
    c.Bind("block_summon", &model.blockSummon);
    c.Bind("show_buffs_label", &model.showBuffsLabel);
    c.Bind("block_summon_label", &model.blockSummonLabel);
    c.Bind("active_tab", &model.activeTab);
    c.Bind("is_follow_enabled", &model.isFollowEnabled);
    c.Bind("members_tab", &model.membersTab);
    c.Bind("search_tab", &model.searchTab);
    c.Bind("settings_tab", &model.settingsTab);
    c.Bind("leader_only", &model.leaderOnly);
    c.Bind("join_blocked", &model.joinBlocked);
    c.Bind("pass_leader", &model.passLeader);
    c.Bind("no_party", &model.noParty);

    c.Bind("finder_empty", &model.finderEmpty);
    c.Bind("finder_intro", &model.finderIntro);
    c.Bind("finder_help", &model.finderHelp);
    c.Bind("finder_title", &model.finderTitle);
    c.Bind("random_label", &model.randomLabel);
    c.Bind("public_list_label", &model.publicListLabel);
    c.Bind("join_label", &model.joinLabel);
    c.Bind("refresh_label", &model.refreshLabel);
    c.Bind("publish_label", &model.publishLabel);
    c.Bind("travel_label", &model.travelLabel);
    c.Bind("status", &model.status);
    c.Bind("empty_label", &model.emptyLabel);
    c.Bind("page_label", &model.pageLabel);
    model.membersTab = StringUtils::WideToNarrow(I18N::Game::PartyFinderMembersTab);
    model.searchTab = StringUtils::WideToNarrow(I18N::Game::PartyFinderSearchTab);
    model.settingsTab = StringUtils::WideToNarrow(I18N::Game::PartyFinderSettingsTab);
    model.leaderOnly = StringUtils::WideToNarrow(I18N::Game::PartyFinderLeaderOnly);
    model.joinBlocked = StringUtils::WideToNarrow(I18N::Game::PartyFinderJoinBlocked);
    model.passLeader = StringUtils::WideToNarrow(I18N::Game::PartyFinderPassLeader);
    model.noParty = StringUtils::WideToNarrow(I18N::Game::PartyFinderNoParty);
    model.finderIntro = StringUtils::WideToNarrow(I18N::Game::PartyFinderIntro);
    model.finderHelp = StringUtils::WideToNarrow(I18N::Game::PartyFinderListHelp);
    model.finderTitle = StringUtils::WideToNarrow(I18N::Game::PartyFinderTitle);
    model.randomLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderRandom);
    model.publicListLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderPublicList);
    model.joinLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderJoin);
    model.refreshLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderRefresh);
    model.publishLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderPublish);
    model.travelLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderTravel);
    model.emptyLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderEmpty);
    c.BindEventCallback("party_setting", [this](Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList& args)
    {
        event.StopPropagation();
        if (args.size() != 1) return;
        const int option = args[0].Get<int>(-1);
        const auto& state = Network::Paris::PartyFinder::Data;
        switch (option)
        {
        case 0: m_PendingAction = state.isPublic ? 2 : 1; break;
        case 1: m_PendingAction = state.followEnabled ? 12 : 11; break;
        case 2: m_PendingAction = state.autoAcceptParty ? 14 : 13; break;
        case 3: m_PendingAction = state.showBuffs ? 19 : 18; break;
        case 4: m_PendingAction = state.blockSummon ? 17 : 16; break;
        default: return;
        }
    });
    c.BindEventCallback("finder_action", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (args.empty()) return;
        m_PendingAction = args[0].Get<int>(-1);
        m_PendingIdentity = args.size() == 2 ? args[1].Get<Rml::String>() : Rml::String{};
    });
    c.BindEventCallback("party_tab", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        if (args.size() == 1) m_PartyTab = std::clamp(args[0].Get<int>(0), 0, 2);
    });
    c.BindEventCallback("party_leader", [this](Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList& args)
    {
        event.StopPropagation();
        const int index = args.size() == 1 ? args[0].Get<int>(-1) : -1;
        if (!Hero || PartyNumber < 2 || wcscmp(Party[0].Name, Hero->ID) || index <= 0 || index >= PartyNumber) return;
        m_PendingAction = 9;
        m_PendingIdentity = StringUtils::WideToNarrow(Party[index].Name);
    });
    model.windowTitle = StringUtils::WideToNarrow(I18N::Game::Party);
    model.exitTooltip = StringUtils::WideToNarrow(I18N::Game::ClosePartyWindowP);


}

void CPartyInfoWindow::BuildRmlUi()
{
    m_RmlView.Ensure();
}

void CPartyInfoWindow::Release()
{
    UnloadImages();

    m_RmlView.Release();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void CPartyInfoWindow::OpenningProcess()
{
    m_PartyTab = PartyNumber > 0 ? 0 : 1;
    SocketClient->ToGameServer()->SendPartyListRequest();
    m_LastFinderPoll = timeGetTime();
    Network::Paris::PartyFinder::Request(0);
}

void CPartyInfoWindow::ClosingProcess()
{
}

void CPartyInfoWindow::RmlClickKickMember(int index)
{
    if (index < 0 || index >= PartyNumber)
        return;

    LeaveParty(index);
}

bool CPartyInfoWindow::UpdateMouseEvent()
{
    // RmlUi handles both close targets; over the panel, the windows below get no mouse.
    return !UI::RmlBridge::IsPointerOver(m_RmlView.Document());
}

bool CPartyInfoWindow::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_PARTY) == true)
    {
        if (mu::ui::window::IsPress(VK_ESCAPE) == true)
        {
            g_pNewUISystem->Hide(mu::ui::window::INTERFACE_PARTY);
            PlayBuffer(SOUND_CLICK01);

            return false;
        }
    }

    return true;
}

bool CPartyInfoWindow::Update()
{
    ProcessFinderAction();
    SetParty(PartyNumber > 0);
    SyncRmlModel();
    return true;
}

bool CPartyInfoWindow::Render()
{
    // RmlUi's #panel owns all chrome/text/button rendering now; nothing left to draw natively.
    return true;
}

bool CPartyInfoWindow::LeaveParty(const int iIndex)
{
    if (!gMapManager.IsCursedTemple())
    {
        PlayBuffer(SOUND_CLICK01);
        SocketClient->ToGameServer()->SendPartyPlayerKickRequest(Party[iIndex].Number);
    }

    SetParty(false);

    return true;
}

void CPartyInfoWindow::Show(bool bShow)
{
    BuildRmlUi();
    mu::ui::window::CObject::Show(bShow);
    if (bShow) SyncRmlModel();
    if (m_RmlView.Document())
    {
        if (bShow) m_RmlView.Document()->Show();
        else m_RmlView.Document()->Hide();
    }
}

void CPartyInfoWindow::SetParty(bool bParty)
{
    m_bParty = bParty;
}

float CPartyInfoWindow::GetLayerDepth()
{
    return 2.4f;
}

void CPartyInfoWindow::SyncRmlModel()
{
    if (!m_RmlView.Document())
        return;

    if (!IsVisible()) return;
    SyncFinderModel();
    auto& model = m_RmlView.GetModel();

    UI::RmlBridge::SyncNativeTextSize(m_RmlView.Binder());

    if (model.hasParty != m_bParty)
    {
        model.hasParty = m_bParty;
        m_RmlView.MarkDirty("has_party");
    }

    if (!m_bParty)
        return;

    wchar_t szText[256] = { 0, };
    std::vector<PartyMemberRow> members;
    members.reserve(PartyNumber);

    for (int i = 0; i < PartyNumber; i++)
    {
        PARTY_t* pMember = &Party[i];

        PartyMemberRow row;
        row.index = i;
        row.name = StringUtils::WideToNarrow(pMember->Name);
        row.mapText = StringUtils::WideToNarrow(gMapManager.GetMapName(pMember->Map));

        mu_swprintf(szText, L"(%d,%d)", pMember->x, pMember->y);
        row.coordText = StringUtils::WideToNarrow(szText);

        row.hpPercent = pMember->maxHP > 0 ? (pMember->currHP * 100.f) / pMember->maxHP : 0.f;
        mu_swprintf(szText, L"%d %ls %d", pMember->currHP, I18N::Game::Text2374, pMember->maxHP);
        row.hpText = StringUtils::WideToNarrow(szText);

        if (const auto* status = Network::Paris::PartyFinder::FindMember(row.name))
        {
            row.hpPercent = status->maximumHealth > 0 ? std::clamp(100.f * status->health / status->maximumHealth,0.f,100.f) : 0.f;
            row.hpText = std::to_string(status->health) + " / " + std::to_string(status->maximumHealth);
            row.manaPercent = status->maximumMana > 0 ? std::clamp(100.f * status->mana / status->maximumMana,0.f,100.f) : 0.f;
            row.manaText = std::to_string(status->mana) + " / " + std::to_string(status->maximumMana);
            mu_swprintf(szText,I18N::Game::PartyFinderLevelResets,status->level,status->resets);
            row.levelText = StringUtils::WideToNarrow(szText);
            if (Network::Paris::PartyFinder::Data.showBuffs)
                for (int id : status->buffs)
                    row.buffs.push_back("image(atlas" + std::to_string(id < 81 ? 1 : 2) + "-" + std::to_string(id < 81 ? id-1 : id-81) + ")");
        }
        else row.manaText = "-- / --";
        row.isLeader = (i == 0);
        row.canFollow = Network::Paris::PartyFinder::Data.followEnabled && wcscmp(pMember->Name, Hero->ID) != 0;
        row.canTransfer = i > 0 && !wcscmp(Party[0].Name, Hero->ID);
        row.followLabel = StringUtils::WideToNarrow(GameLogic::Social::PartyFollow::IsFollowing(pMember->Name)
            ? I18N::Game::PartyFinderStop : I18N::Game::PartyFinderFollow);
        row.showKick = !wcscmp(Party[0].Name, Hero->ID) || !wcscmp(Party[i].Name, Hero->ID);

        members.push_back(row);
    }

    model.members = std::move(members);
    m_RmlView.MarkDirty("members");
}

void CPartyInfoWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_PARTY_BASE_WINDOW_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_PARTY_BASE_WINDOW_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_PARTY_BASE_WINDOW_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_PARTY_BASE_WINDOW_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_PARTY_BASE_WINDOW_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_PARTY_BASE_WINDOW_BTN_EXIT, GL_LINEAR);		// Exit Button

    LoadBitmap(L"Interface\\newui_item_table01(L).tga", IMAGE_PARTY_TABLE_TOP_LEFT);
    LoadBitmap(L"Interface\\newui_item_table01(R).tga", IMAGE_PARTY_TABLE_TOP_RIGHT);
    LoadBitmap(L"Interface\\newui_item_table02(L).tga", IMAGE_PARTY_TABLE_BOTTOM_LEFT);
    LoadBitmap(L"Interface\\newui_item_table02(R).tga", IMAGE_PARTY_TABLE_BOTTOM_RIGHT);
    LoadBitmap(L"Interface\\newui_item_table03(Up).tga", IMAGE_PARTY_TABLE_TOP_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(Dw).tga", IMAGE_PARTY_TABLE_BOTTOM_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(L).tga", IMAGE_PARTY_TABLE_LEFT_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(R).tga", IMAGE_PARTY_TABLE_RIGHT_PIXEL);

    LoadBitmap(L"Interface\\newui_party_lifebar01.jpg", IMAGE_PARTY_HPBAR_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_party_lifebar02.jpg", IMAGE_PARTY_HPBAR, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_party_flag.tga", IMAGE_PARTY_FLAG, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_party_x.tga", IMAGE_PARTY_EXIT, GL_LINEAR);
}

void CPartyInfoWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_PARTY_BASE_WINDOW_BACK);
    DeleteBitmap(IMAGE_PARTY_BASE_WINDOW_TOP);
    DeleteBitmap(IMAGE_PARTY_BASE_WINDOW_LEFT);
    DeleteBitmap(IMAGE_PARTY_BASE_WINDOW_RIGHT);
    DeleteBitmap(IMAGE_PARTY_BASE_WINDOW_BOTTOM);
    DeleteBitmap(IMAGE_PARTY_BASE_WINDOW_BTN_EXIT);

    DeleteBitmap(IMAGE_PARTY_TABLE_RIGHT_PIXEL);
    DeleteBitmap(IMAGE_PARTY_TABLE_LEFT_PIXEL);
    DeleteBitmap(IMAGE_PARTY_TABLE_BOTTOM_PIXEL);
    DeleteBitmap(IMAGE_PARTY_TABLE_TOP_PIXEL);
    DeleteBitmap(IMAGE_PARTY_TABLE_BOTTOM_RIGHT);
    DeleteBitmap(IMAGE_PARTY_TABLE_BOTTOM_LEFT);
    DeleteBitmap(IMAGE_PARTY_TABLE_TOP_RIGHT);
    DeleteBitmap(IMAGE_PARTY_TABLE_TOP_LEFT);

    DeleteBitmap(IMAGE_PARTY_HPBAR_BACK);
    DeleteBitmap(IMAGE_PARTY_HPBAR);
    DeleteBitmap(IMAGE_PARTY_FLAG);
    DeleteBitmap(IMAGE_PARTY_EXIT);
}

void CPartyInfoWindow::ProcessFinderAction()
{
    if (m_PendingFollow >= 0)
    {
        GameLogic::Social::PartyFollow::Toggle(m_PendingFollow);
        m_PendingFollow = -1;
    }
    if (m_PendingAction == 7 || m_PendingAction == 8)
    {
        const int pages = std::max(1, static_cast<int>((Network::Paris::PartyFinder::Listings.size()
            + Network::Paris::PartyFinder::PageSize - 1) / Network::Paris::PartyFinder::PageSize));
        m_FinderPage = std::clamp(m_FinderPage + (m_PendingAction == 7 ? -1 : 1), 0, pages - 1);
        m_PendingAction = -1;
    }
    if ((m_PendingAction >= 0 && m_PendingAction <= 5) || m_PendingAction == 9 || m_PendingAction == 11 || m_PendingAction == 12 || m_PendingAction == 13 || m_PendingAction == 14 || (m_PendingAction >= 16 && m_PendingAction <= 19))
    {
        if (PartyNumber > 0 && (m_PendingAction == 3 || m_PendingAction == 4)) { m_PendingAction = -1; return; }
        const bool leaderSetting = m_PendingAction == 1 || m_PendingAction == 2 || (m_PendingAction >= 11 && m_PendingAction <= 14);
        if (PartyNumber > 0 && leaderSetting && (!Hero || wcscmp(Party[0].Name, Hero->ID)))
        { m_PendingAction = -1; return; }
        // The server limits mutations to one per second. Keep a clicked action queued,
        // instead of sending it inside that interval and silently losing the click.
        constexpr unsigned long FinderActionInterval = 1050;
        const auto now = timeGetTime();
        if (m_LastFinderActionSent && now - m_LastFinderActionSent < FinderActionInterval) return;
        m_LastFinderActionSent = now;
        m_LastFinderPoll = now;
        Network::Paris::PartyFinder::Request(static_cast<BYTE>(m_PendingAction >= 16 ? m_PendingAction - 4 : m_PendingAction == 13 ? 10 : m_PendingAction == 14 ? 11 : m_PendingAction == 9 ? 7 : m_PendingAction == 11 ? 8 : m_PendingAction == 12 ? 9 : m_PendingAction), m_PendingIdentity);
        m_PendingAction = -1;
    }
    if (IsVisible() && timeGetTime() - m_LastFinderPoll >= 1000)
    {
        m_LastFinderPoll = timeGetTime();
        Network::Paris::PartyFinder::Request(0);
    }
}

void CPartyInfoWindow::SyncFinderModel()
{
    using namespace Network::Paris::PartyFinder;
    const auto& snapshot = Network::Paris::PartyFinder::Data;
    auto& model = m_RmlView.GetModel();
    model.isLeader = Hero && PartyNumber > 0 && !wcscmp(Party[0].Name, Hero->ID);
    model.isPublic = Received && snapshot.isPublic;
    model.activeTab = m_PartyTab;
    model.isFollowEnabled = snapshot.followEnabled;
    model.autoAcceptParty = snapshot.autoAcceptParty;
    model.showBuffs = snapshot.showBuffs;
    model.blockSummon = snapshot.blockSummon;
    model.showBuffsLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderShowBuffs);
    model.blockSummonLabel = StringUtils::WideToNarrow(I18N::Game::PartyFinderBlockSummon);
    const int pages = std::max(1, static_cast<int>((Listings.size() + PageSize - 1) / PageSize));
    m_FinderPage = std::clamp(m_FinderPage, 0, pages - 1);
    model.pageLabel = std::to_string(m_FinderPage + 1) + " / " + std::to_string(pages);
    model.finderEmpty = Listings.empty();
    model.finderParties.clear();
    const int end = std::min(static_cast<int>(Listings.size()), (m_FinderPage + 1) * PageSize);
    for (int i = m_FinderPage * PageSize; i < end; ++i)
    {
        const auto& row = Listings[i];
        model.finderParties.push_back({row.id, row.leader, StringUtils::WideToNarrow(gMapManager.GetMapName(row.map)),
            std::to_string(row.members) + "/" + std::to_string(row.capacity), row.members >= row.capacity});
    }
    if (!Received) model.status = StringUtils::WideToNarrow(I18N::Game::PartyFinderWaiting);
    else if (snapshot.request != m_LastFinderResponse)
    {
        m_LastFinderResponse = snapshot.request;
        const wchar_t* errors[] = {L"", I18N::Game::PartyFinderUnavailable, I18N::Game::PartyFinderDenied,
            I18N::Game::PartyFinderFull, I18N::Game::PartyFinderMissing, I18N::Game::PartyFinderRestricted, I18N::Game::PartyFinderBlocked};
        model.status = StringUtils::WideToNarrow(errors[std::clamp(snapshot.status, 0, 6)]);
    }
    for (const char* key : {"show_buffs", "block_summon", "show_buffs_label", "block_summon_label", "auto_accept_party", "is_follow_enabled", "active_tab", "is_leader", "is_public", "page_label", "finder_empty", "finder_parties", "status"})
        m_RmlView.MarkDirty(key);
}
