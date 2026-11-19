// Breda: the dungeons of MuDream. See DungeonWindow.h.
//
// Packets (main code 7E, little endian), the counterpart of Breda.Dungeons/DungeonPackets.cs:
//   client -> server  C1 04 7E 00                          open: asks for the window
//                     C1 06 7E 01 [dungeon] [difficulty]   join (the leader starts, a member joins its party's run)
//                     C1 04 7E 02                          start now (the leader)
//                     C1 04 7E 03                          leave the dungeon
//   server -> client  C2 [len u16 BE] 7E 00 ...            the window (see ParseWindow)
//                     C1 18 7E 01 ...                      the state of the run
//                     C1 10 7E 02 ...                      the result
//                     C1 06 7E 03 [message] [argument]     what a request resulted in
#include "stdafx.h"

#include "UI/Events/DungeonWindow.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>

#include "Audio/DSPlaySound.h"
#include "Core/Utilities/StringUtils.h"
#include "Dotnet/Connection.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInterface.h"
#include "I18N/All.h"
#include "Network/Server/WSclient.h"
#include "Render/RmlUi/RmlUiRuntime.h"
#include "Scenes/SceneCore.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowSystem.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlStackingOrder.h"
#include "UI/RmlBridge/RmlWindowClose.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

extern Connection* SocketClient;

namespace
{
using Paris::Dungeons::RunState;

constexpr BYTE DungeonCode = 0x7E;
constexpr BYTE WindowSubCode = 0x00;
constexpr BYTE StateSubCode = 0x01;
constexpr BYTE ResultSubCode = 0x02;
constexpr BYTE MessageSubCode = 0x03;
constexpr std::size_t HeaderLength = 32;
constexpr std::size_t DungeonLength = 40;
constexpr std::size_t DifficultyLength = 56;
constexpr std::size_t NameLength = 32;

mu::ui::window::CDungeonWindow* g_dungeonWindow = nullptr;
mu::ui::window::CDungeonTracker* g_dungeonTracker = nullptr;
Paris::Dungeons::WindowInfo g_lastWindow;

std::uint64_t NowMilliseconds()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

std::string Narrow(const wchar_t* text)
{
    return text == nullptr ? std::string() : StringUtils::WideToNarrow(text);
}

// A localized text with printf placeholders (%d, %ls), as UTF-8.
template <typename... Args> std::string WFormat(const wchar_t* format, Args... args)
{
    wchar_t buffer[512];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), format, args...);
    return Narrow(buffer);
}

std::wstring Widen(const std::string& text)
{
    return StringUtils::NarrowToWide(text);
}

std::uint16_t ReadU16(std::span<const BYTE> data, std::size_t offset)
{
    return static_cast<std::uint16_t>(data[offset] | (data[offset + 1] << 8));
}

std::uint32_t ReadU32(std::span<const BYTE> data, std::size_t offset)
{
    return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(data[offset + 2]) << 16) | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

std::string ReadName(std::span<const BYTE> data, std::size_t offset)
{
    std::string name;
    for (std::size_t i = 0; i < NameLength && offset + i < data.size() && data[offset + i] != 0; ++i)
        name.push_back(static_cast<char>(data[offset + i]));
    return name;
}

void Send(const BYTE* data, int length)
{
    if (SocketClient != nullptr && SocketClient->IsConnected())
    {
        SocketClient->Send(data, length);
    }
}

void SendSimple(BYTE subCode)
{
    const BYTE packet[] = {0xC1, 0x04, DungeonCode, subCode};
    Send(packet, sizeof(packet));
}

bool ParseWindow(std::span<const BYTE> data, Paris::Dungeons::WindowInfo& info)
{
    if (data.size() < HeaderLength || data[0] != 0xC2)
        return false;

    info.received = true;
    info.enabled = data[5] != 0;
    info.mode = data[6];
    info.entriesLeft = data[7] == 0xFF ? -1 : data[7];
    info.entriesPerDay = data[8];
    const int dungeonCount = data[9];
    const int difficultyCount = data[10];
    info.level = ReadU16(data, 11);
    info.resets = ReadU16(data, 13);
    info.grandResets = ReadU16(data, 15);
    info.powerScore = ReadU32(data, 17);
    info.partyDungeon = data[21] == 0xFF ? -1 : data[21];
    info.partyDifficulty = data[22] == 0xFF ? -1 : data[22];
    info.partyState = data[23];
    info.canStart = data[24] != 0;

    const std::size_t requiredSize = HeaderLength + std::size_t(dungeonCount) * DungeonLength + std::size_t(difficultyCount) * DifficultyLength;
    if (dungeonCount > 20 || difficultyCount > 20 || data.size() != requiredSize)
        return false;
    std::size_t offset = HeaderLength;
    for (int i = 0; i < dungeonCount && offset + DungeonLength <= data.size(); ++i, offset += DungeonLength)
    {
        Paris::Dungeons::DungeonInfo dungeon;
        dungeon.id = data[offset];
        dungeon.active = data[offset + 1] != 0;
        dungeon.map = ReadU16(data, offset + 2);
        dungeon.stages = data[offset + 4];
        // Bytes 5-7 (Breda, 2026-11-09): in the range of resets, minimum, maximum (FF: none). An older
        // server sends zeros there: no range.
        const bool hasRange = data[offset + 6] != 0 || data[offset + 7] != 0;
        dungeon.resetsOk = !hasRange || data[offset + 5] != 0;
        dungeon.minimumResets = data[offset + 6];
        dungeon.maximumResets = !hasRange || data[offset + 7] == 0xFF ? -1 : data[offset + 7];
        dungeon.name = ReadName(data, offset + 8);
        info.dungeons.push_back(std::move(dungeon));
    }

    for (int i = 0; i < difficultyCount && offset + DifficultyLength <= data.size(); ++i, offset += DifficultyLength)
    {
        Paris::Dungeons::DifficultyInfo difficulty;
        difficulty.id = data[offset];
        difficulty.meets = data[offset + 1] != 0;
        difficulty.healthPercent = ReadU16(data, offset + 2);
        difficulty.damagePercent = ReadU16(data, offset + 4);
        difficulty.defensePercent = ReadU16(data, offset + 6);
        difficulty.experiencePercent = ReadU16(data, offset + 8);
        difficulty.level = ReadU16(data, offset + 10);
        difficulty.resets = ReadU16(data, offset + 12);
        difficulty.grandResets = ReadU16(data, offset + 14);
        difficulty.itemGroup = static_cast<std::int16_t>(ReadU16(data, offset + 16));
        difficulty.itemNumber = ReadU16(data, offset + 18);
        difficulty.itemLevel = data[offset + 20];
        difficulty.hasItem = data[offset + 21] != 0;
        difficulty.requiresMaster = data[offset + 22] != 0;
        difficulty.name = ReadName(data, offset + 24);
        info.difficulties.push_back(std::move(difficulty));
    }

    return true;
}

// The MuDream texts of the dungeons (Text_Eng.xml 9550-9556), by id.
const wchar_t* DungeonDescription(int id)
{
    switch (id)
    {
    case 0: return I18N::Game::DungeonDescription0;
    case 1: return I18N::Game::DungeonDescription1;
    case 2: return I18N::Game::DungeonDescription2;
    case 3: return I18N::Game::DungeonDescription3;
    case 4: return I18N::Game::DungeonDescription4;
    case 5: return I18N::Game::DungeonDescription5;
    case 6: return I18N::Game::DungeonDescription6;
    default: return I18N::Game::DungeonComingSoon;
    }
}

// The images of MuDream (Data/Interface/Dungeon): art and button 1-6 for the dungeons 0-5, 61 for the others.
std::string ImageNumber(int id)
{
    return id >= 0 && id <= 5 ? std::to_string(id + 1) : std::string("61");
}

std::string ArtSource(int id)
{
    return "../../../Dungeon/art_" + ImageNumber(id) + ".tga";
}

std::string ButtonSource(int id)
{
    return "../../../Dungeon/button_" + ImageNumber(id) + ".tga";
}

std::string DifficultySource(int id)
{
    return "../../../Dungeon/difficulty_" + std::to_string(std::clamp(id + 1, 1, 6)) + ".tga";
}

std::string StateText(RunState state)
{
    switch (state)
    {
    case RunState::Entrance: return Narrow(I18N::Game::DungeonStateEntrance);
    case RunState::Prepare: return Narrow(I18N::Game::DungeonStatePrepare);
    case RunState::Play: return Narrow(I18N::Game::DungeonStatePlay);
    case RunState::Finish: return Narrow(I18N::Game::DungeonStateFinish);
    case RunState::Clear: return Narrow(I18N::Game::DungeonStateClear);
    default: return Narrow(I18N::Game::DungeonStateNone);
    }
}

std::string MessageText(int message, int argument)
{
    switch (message)
    {
    case 0: return "";
    case 1: return Narrow(I18N::Game::DungeonDisabled);
    case 2: return Narrow(I18N::Game::DungeonMessageLevel);
    case 3: return Narrow(I18N::Game::DungeonMessageResets);
    case 4: return Narrow(I18N::Game::DungeonMessageGrandResets);
    case 5: return Narrow(I18N::Game::DungeonMessageNoEntries);
    case 6: return Narrow(I18N::Game::DungeonMessageItem);
    case 7: return Narrow(I18N::Game::DungeonMessageAlready);
    case 8: return Narrow(I18N::Game::DungeonMessageNotLeader);
    case 9: return Narrow(I18N::Game::DungeonMessageParty);
    case 11: return Narrow(I18N::Game::DungeonMessageNotActive);
    case 12: return WFormat(I18N::Game::DungeonMessageLeftBehind, argument);
    case 13: return Narrow(I18N::Game::DungeonMessageInDungeon);
    case 14: return Narrow(I18N::Game::DungeonMessageResetRange);
    case 15: return Narrow(I18N::Game::DungeonMessageMaster);
    default: return Narrow(I18N::Game::DungeonMessageFailed);
    }
}

std::string Clock(unsigned seconds)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02u:%02u", seconds / 60, seconds % 60);
    return buffer;
}

std::wstring ItemName(int group, int number, int level)
{
    if (group < 0 || ItemAttribute == nullptr)
        return L"-";
    std::wstring name = ItemAttribute[ITEMINDEX(group, number)].Name;
    if (name.empty())
        name = std::to_wstring(group) + L"/" + std::to_wstring(number);
    if (level > 0)
        name += L" +" + std::to_wstring(level);
    return name;
}
} // namespace

namespace Paris::Dungeons
{
void ReceivePacket(std::span<const BYTE> packet)
{
    if (packet.size() < 4)
        return;

    const bool isC1 = packet[0] == 0xC1 || packet[0] == 0xC3;
    if ((!isC1 && (packet[0] != 0xC2 && packet[0] != 0xC4)) || (!isC1 && packet.size() < 5))
        return;
    const std::size_t declaredSize = isC1 ? packet[1] : (std::size_t(packet[1]) << 8) | packet[2];
    if (declaredSize != packet.size())
        return;
    const BYTE subCode = isC1 ? packet[3] : packet[4];
    if (!isC1 && subCode == WindowSubCode)
    {
        WindowInfo info;
        if (ParseWindow(packet, info))
        {
            g_lastWindow = info;
            if (g_dungeonWindow != nullptr)
                g_dungeonWindow->OnWindow(info);
        }
    }
    else if (isC1 && subCode == StateSubCode && packet.size() >= 17)
    {
        RunInfo run;
        run.state = static_cast<RunState>(packet[4]);
        run.dungeon = packet[5];
        run.difficulty = packet[6];
        run.stage = packet[7];
        run.stageCount = packet[8];
        run.monstersLeft = ReadU16(packet, 9);
        run.players = packet[11];
        run.secondsLeft = ReadU32(packet, 12);
        run.canStart = packet[16] != 0;
        run.receivedAt = NowMilliseconds();
        if (g_dungeonTracker != nullptr)
            g_dungeonTracker->OnState(run);
    }
    else if (isC1 && subCode == ResultSubCode && packet.size() >= 14)
    {
        if (g_dungeonTracker != nullptr)
            g_dungeonTracker->OnResult(packet[4] != 0, packet[5], packet[6], ReadU32(packet, 8), ReadU16(packet, 12));
    }
    else if (isC1 && subCode == MessageSubCode && packet.size() >= 6)
    {
        if (g_dungeonWindow != nullptr)
            g_dungeonWindow->OnMessage(packet[4], packet[5]);
    }
}
} // namespace Paris::Dungeons

namespace mu::ui::window
{
CDungeonWindow* GetDungeonWindow()
{
    return g_dungeonWindow;
}

CDungeonTracker* GetDungeonTracker()
{
    return g_dungeonTracker;
}

// ---------------------------------------------------------------------------------------------
// CDungeonWindow
// ---------------------------------------------------------------------------------------------
CDungeonWindow::CDungeonWindow() = default;

CDungeonWindow::~CDungeonWindow()
{
    Release();
}

bool CDungeonWindow::Create(CManager* pNewUIMng)
{
    if (pNewUIMng == nullptr)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_DUNGEON, this);
    g_dungeonWindow = this;

    if (RmlUiRuntime::Instance().IsCreated())
    {
        m_RmlView.Ensure();
    }

    Show(false);
    return true;
}

void CDungeonWindow::Release()
{
    m_RmlView.Release();
    if (g_dungeonWindow == this)
        g_dungeonWindow = nullptr;

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CDungeonWindow::BindRmlModel(Rml::DataModelConstructor& c, DungeonRmlModel& model)
{
    auto dungeon = c.RegisterStruct<DungeonEntry>();
    dungeon.RegisterMember("index", &DungeonEntry::index);
    dungeon.RegisterMember("id", &DungeonEntry::id);
    dungeon.RegisterMember("name", &DungeonEntry::name);
    dungeon.RegisterMember("button_src", &DungeonEntry::buttonSrc);
    dungeon.RegisterMember("active", &DungeonEntry::active);
    dungeon.RegisterMember("selected", &DungeonEntry::selected);
    dungeon.RegisterMember("party", &DungeonEntry::party);
    c.RegisterArray<std::vector<DungeonEntry>>();

    auto difficulty = c.RegisterStruct<DifficultyEntry>();
    difficulty.RegisterMember("index", &DifficultyEntry::index);
    difficulty.RegisterMember("id", &DifficultyEntry::id);
    difficulty.RegisterMember("name", &DifficultyEntry::name);
    difficulty.RegisterMember("icon_src", &DifficultyEntry::iconSrc);
    difficulty.RegisterMember("selected", &DifficultyEntry::selected);
    difficulty.RegisterMember("meets", &DifficultyEntry::meets);
    c.RegisterArray<std::vector<DifficultyEntry>>();

    c.Bind("title", &model.title);
    c.Bind("select_label", &model.selectLabel);
    c.Bind("difficulty_label", &model.difficultyLabel);
    c.Bind("boost_label", &model.boostLabel);
    c.Bind("requirements_label", &model.requirementsLabel);
    c.Bind("art_src", &model.artSrc);
    c.Bind("dungeon_name", &model.dungeonName);
    c.Bind("description", &model.description);
    c.Bind("power_text", &model.powerText);
    c.Bind("health_text", &model.healthText);
    c.Bind("damage_text", &model.damageText);
    c.Bind("defense_text", &model.defenseText);
    c.Bind("experience_text", &model.experienceText);
    c.Bind("minimum_level", &model.minimumLevel);
    c.Bind("resets_range", &model.resetsRange);
    c.Bind("your_resets", &model.yourResets);
    c.Bind("requirement_text", &model.requirementText);
    c.Bind("entries_text", &model.entriesText);
    c.Bind("item_text", &model.itemText);
    c.Bind("requirement_met", &model.requirementMet);
    c.Bind("entries_met", &model.entriesMet);
    c.Bind("item_met", &model.itemMet);
    c.Bind("join_label", &model.joinLabel);
    c.Bind("can_join", &model.canJoin);
    c.Bind("status_text", &model.statusText);
    c.Bind("waiting", &model.waiting);
    c.Bind("dungeons", &model.dungeons);
    c.Bind("difficulties", &model.difficulties);

    const auto queue = [this](std::function<void()> action) { m_PendingActions.push_back(std::move(action)); };
    c.BindEventCallback("dg_dungeon",
        [this, queue](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            if (args.size() == 1)
                queue([this, index = args[0].Get<int>(-1)] { SelectDungeon(index); });
        });
    c.BindEventCallback("dg_difficulty",
        [this, queue](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            if (args.size() == 1)
                queue([this, index = args[0].Get<int>(-1)] { SelectDifficulty(index); });
        });
    c.BindEventCallback("dg_join",
        [this, queue](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            queue([this] { Join(); });
        });
    UI::RmlBridge::BindWindowClose(c, INTERFACE_DUNGEON);
}

void CDungeonWindow::Show(bool bShow)
{
    CObject::Show(bShow);
    if (m_RmlView.Document())
    {
        if (bShow)
            m_RmlView.Document()->Show();
        else
            m_RmlView.Document()->Hide();
    }
}

bool CDungeonWindow::UpdateMouseEvent()
{
    return !UI::RmlBridge::IsPointerOver(m_RmlView.Document());
}

bool CDungeonWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(INTERFACE_DUNGEON);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CDungeonWindow::Update()
{
    if (!m_PendingActions.empty())
    {
        auto actions = std::move(m_PendingActions);
        m_PendingActions.clear();
        for (auto& action : actions)
            action();
    }

    return true;
}

float CDungeonWindow::GetLayerDepth()
{
    static const float depth = UI::RmlBridge::StackingDepthForDocument("dungeon.rml").value_or(10.08f);
    return depth;
}

void CDungeonWindow::OpeningProcess()
{
    m_RmlView.Ensure();
    auto& model = m_RmlView.GetModel();
    model.statusText.clear();
    Rebuild();
    SendSimple(WindowSubCode);
}

void CDungeonWindow::OnWindow(const Paris::Dungeons::WindowInfo& info)
{
    m_Info = info;
    // The run of the party is selected, so a member joins it with one click.
    if (info.partyDungeon >= 0)
    {
        for (int i = 0; i < static_cast<int>(info.dungeons.size()); ++i)
        {
            if (info.dungeons[i].id == info.partyDungeon)
                m_SelectedDungeon = i;
        }
        for (int i = 0; i < static_cast<int>(info.difficulties.size()); ++i)
        {
            if (info.difficulties[i].id == info.partyDifficulty)
                m_SelectedDifficulty = i;
        }
    }

    m_SelectedDungeon = std::clamp(m_SelectedDungeon, 0, std::max(0, static_cast<int>(info.dungeons.size()) - 1));
    m_SelectedDifficulty = std::clamp(m_SelectedDifficulty, 0, std::max(0, static_cast<int>(info.difficulties.size()) - 1));
    if (!IsVisible())
        g_pNewUISystem->Show(INTERFACE_DUNGEON);
    Rebuild();
}

void CDungeonWindow::OnMessage(int message, int argument)
{
    if (!m_RmlView.Document())
        return;

    auto& model = m_RmlView.GetModel();
    model.statusText = MessageText(message, argument);
    m_RmlView.MarkDirty("status_text");
    if (message == 0 || message == 12)
    {
        // Entered: the window closes, the tracker shows the run.
        PlayBuffer(SOUND_CLICK01);
        if (IsVisible())
            g_pNewUISystem->Hide(INTERFACE_DUNGEON);
    }
}

void CDungeonWindow::SelectDungeon(int index)
{
    if (index < 0 || index >= static_cast<int>(m_Info.dungeons.size()))
        return;

    m_SelectedDungeon = index;
    PlayBuffer(SOUND_CLICK01);
    Rebuild();
}

void CDungeonWindow::SelectDifficulty(int index)
{
    if (index < 0 || index >= static_cast<int>(m_Info.difficulties.size()))
        return;

    m_SelectedDifficulty = index;
    PlayBuffer(SOUND_CLICK01);
    Rebuild();
}

void CDungeonWindow::Join()
{
    if (!m_Info.received || !m_Info.enabled || m_Info.dungeons.empty() || m_Info.difficulties.empty())
        return;

    const auto& dungeon = m_Info.dungeons[m_SelectedDungeon];
    const auto& difficulty = m_Info.difficulties[m_SelectedDifficulty];
    const BYTE packet[] = {0xC1, 0x06, DungeonCode, StateSubCode, static_cast<BYTE>(dungeon.id), static_cast<BYTE>(difficulty.id)};
    Send(packet, sizeof(packet));
    PlayBuffer(SOUND_CLICK01);
}

void CDungeonWindow::Rebuild()
{
    if (!m_RmlView.Document())
        return;

    auto& model = m_RmlView.GetModel();
    model.waiting = !m_Info.received;
    model.title = Narrow(I18N::Game::DungeonTitle);
    model.selectLabel = Narrow(I18N::Game::DungeonSelect);
    model.difficultyLabel = Narrow(I18N::Game::DungeonDifficulty);
    model.boostLabel = Narrow(I18N::Game::DungeonBoost);
    model.requirementsLabel = Narrow(I18N::Game::DungeonRequirements);
    model.joinLabel = Narrow(I18N::Game::DungeonJoin);

    model.dungeons.clear();
    for (int i = 0; i < static_cast<int>(m_Info.dungeons.size()); ++i)
    {
        const auto& info = m_Info.dungeons[i];
        DungeonEntry entry;
        entry.index = i;
        entry.id = info.id;
        entry.name = info.name;
        entry.buttonSrc = ButtonSource(info.id);
        entry.active = info.active;
        entry.selected = i == m_SelectedDungeon;
        entry.party = info.id == m_Info.partyDungeon;
        model.dungeons.push_back(std::move(entry));
    }

    model.difficulties.clear();
    for (int i = 0; i < static_cast<int>(m_Info.difficulties.size()); ++i)
    {
        const auto& info = m_Info.difficulties[i];
        DifficultyEntry entry;
        entry.index = i;
        entry.id = info.id;
        entry.name = info.name;
        entry.iconSrc = DifficultySource(info.id);
        entry.selected = i == m_SelectedDifficulty;
        entry.meets = info.meets;
        model.difficulties.push_back(std::move(entry));
    }

    if (!m_Info.received || !m_Info.enabled || m_Info.dungeons.empty() || m_Info.difficulties.empty())
    {
        model.artSrc = ArtSource(0);
        model.dungeonName = "";
        model.description = Narrow(m_Info.received ? I18N::Game::DungeonDisabled : I18N::Game::DungeonLoading);
        model.canJoin = false;
        model.healthText = model.damageText = model.defenseText = model.experienceText = "";
        model.requirementText = model.entriesText = model.itemText = model.powerText = "";
    }
    else
    {
        const auto& dungeon = m_Info.dungeons[m_SelectedDungeon];
        const auto& difficulty = m_Info.difficulties[m_SelectedDifficulty];
        model.artSrc = ArtSource(dungeon.id);
        model.dungeonName = dungeon.name;
        model.description = Narrow(DungeonDescription(dungeon.id));
        model.powerText = WFormat(I18N::Game::DungeonPowerScore, std::to_wstring(m_Info.powerScore).c_str());
        model.healthText = WFormat(I18N::Game::DungeonMonstersHealth, difficulty.healthPercent);
        model.damageText = WFormat(I18N::Game::DungeonMonstersDamage, difficulty.damagePercent);
        model.defenseText = WFormat(I18N::Game::DungeonMonstersDefense, difficulty.defensePercent);
        model.experienceText = WFormat(I18N::Game::DungeonExperienceRate, difficulty.experiencePercent);

        switch (m_Info.mode)
        {
        case 1:
            model.requirementText = WFormat(I18N::Game::DungeonRequiredResets, difficulty.resets, m_Info.resets);
            model.requirementMet = m_Info.resets >= difficulty.resets;
            break;
        case 2:
            model.requirementText = WFormat(I18N::Game::DungeonRequiredGrandResets, difficulty.grandResets, m_Info.grandResets);
            model.requirementMet = m_Info.grandResets >= difficulty.grandResets;
            break;
        default:
            model.requirementText = WFormat(I18N::Game::DungeonRequiredLevel, difficulty.level, m_Info.level);
            model.requirementMet = m_Info.level >= difficulty.level;
            break;
        }

        if (difficulty.requiresMaster)
        {
            model.requirementText += "  -  " + Narrow(I18N::Game::DungeonRequiredMaster);
            model.requirementMet = model.requirementMet && difficulty.meets;
        }

        // Breda: one dungeon per range of resets; its range comes before the requirement of the difficulty.
        if (dungeon.minimumResets > 0 || dungeon.maximumResets >= 0)
        {
            const std::string range = dungeon.maximumResets >= 0
                ? WFormat(I18N::Game::DungeonResetRange, dungeon.minimumResets, dungeon.maximumResets, m_Info.resets)
                : WFormat(I18N::Game::DungeonResetRangeFrom, dungeon.minimumResets, m_Info.resets);
            model.requirementText = range + "  -  " + model.requirementText;
            model.requirementMet = model.requirementMet && dungeon.resetsOk;
        }

        if (m_Info.entriesLeft < 0)
        {
            model.entriesText = Narrow(I18N::Game::DungeonEntriesUnlimited);
            model.entriesMet = true;
        }
        else
        {
            model.entriesText = WFormat(I18N::Game::DungeonEntries, m_Info.entriesLeft, m_Info.entriesPerDay);
            model.entriesMet = m_Info.entriesLeft > 0;
        }

        model.itemText = WFormat(I18N::Game::DungeonItem, ItemName(difficulty.itemGroup, difficulty.itemNumber, difficulty.itemLevel).c_str());
        model.itemMet = difficulty.itemGroup < 0 || difficulty.hasItem;

        const bool joinsParty = m_Info.partyDungeon >= 0;
        model.canJoin = dungeon.active && dungeon.resetsOk && (joinsParty ? m_Info.partyState == 1 : m_Info.canStart) && difficulty.meets && model.itemMet;
    }

    UpdateParisPresentation();
    for (const char* name : {"minimum_level", "resets_range", "your_resets", "title", "select_label", "difficulty_label", "boost_label", "requirements_label", "join_label",
                             "dungeons", "difficulties", "art_src", "dungeon_name", "description", "power_text",
                             "health_text", "damage_text", "defense_text", "experience_text", "requirement_text",
                             "entries_text", "item_text", "requirement_met", "entries_met", "item_met", "can_join",
                             "status_text", "waiting"})
    {
        m_RmlView.MarkDirty(name);
    }
}

void CDungeonWindow::UpdateParisPresentation()
{
    auto& model = m_RmlView.GetModel();
    model.title = "MAZMORRAS";
    model.selectLabel = "SELECCIONAR MAZMORRA";
    model.difficultyLabel = "DIFICULTAD";
    model.boostLabel = "ATRIBUTOS";
    model.requirementsLabel = "REQUISITOS";
    model.joinLabel = "ENTRAR";
    for (auto& difficulty : model.difficulties)
        difficulty.name = difficulty.id == 0 ? "NORMAL" : difficulty.id == 1 ? "DIFICIL" : "MAESTRO";
    if (!m_Info.received || m_Info.dungeons.empty() || m_Info.difficulties.empty()) return;
    const auto& dungeon = m_Info.dungeons[m_SelectedDungeon];
    const auto& difficulty = m_Info.difficulties[m_SelectedDifficulty];
    model.minimumLevel = std::to_string(difficulty.level);
    model.resetsRange = std::to_string(dungeon.minimumResets) + (dungeon.maximumResets < 0
        ? " o mas" : " - " + std::to_string(dungeon.maximumResets));
    model.yourResets = std::to_string(m_Info.resets);
    model.powerText = std::to_string(m_Info.powerScore);
    model.healthText = std::to_string(difficulty.healthPercent) + "%";
    model.damageText = std::to_string(difficulty.damagePercent) + "%";
    model.defenseText = std::to_string(difficulty.defensePercent) + "%";
    model.experienceText = std::to_string(difficulty.experiencePercent) + "%";
    model.entriesText = m_Info.entriesLeft < 0 ? "Sin limite"
        : std::to_string(std::max(0, m_Info.entriesPerDay - m_Info.entriesLeft)) + " / " + std::to_string(m_Info.entriesPerDay);
    model.itemText = difficulty.itemGroup < 0 ? "Ninguno"
        : Narrow(ItemName(difficulty.itemGroup, difficulty.itemNumber, difficulty.itemLevel).c_str());
    model.description = "Explora las ruinas y enfrenta monstruos elite";
    if (m_Info.entriesLeft == 0) model.statusText = "No quedan entradas disponibles hoy";
    else if (!dungeon.resetsOk) model.statusText = "No cumples el requisito de resets";
    else if (!difficulty.meets) model.statusText = "No cumples los requisitos de esta dificultad";
    else if (!model.itemMet) model.statusText = "Falta el objeto requerido";
    else if (m_Info.canStart) model.statusText = "";
}

// ---------------------------------------------------------------------------------------------
// CDungeonTracker
// ---------------------------------------------------------------------------------------------
CDungeonTracker::CDungeonTracker() = default;

CDungeonTracker::~CDungeonTracker()
{
    Release();
}

bool CDungeonTracker::Create(CManager* pNewUIMng)
{
    if (pNewUIMng == nullptr)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_DUNGEON_TRACKER, this);
    g_dungeonTracker = this;
    if (RmlUiRuntime::Instance().IsCreated())
    {
        m_RmlView.Ensure();
    }

    // Always on: the document shows itself while the hero is in a dungeon (SyncVisibility).
    Show(true);
    return true;
}

void CDungeonTracker::Release()
{
    m_RmlView.Release();
    if (g_dungeonTracker == this)
        g_dungeonTracker = nullptr;

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CDungeonTracker::BindRmlModel(Rml::DataModelConstructor& c, DungeonTrackerRmlModel& model)
{
    c.Bind("shown", &model.shown);
    c.Bind("collapsed", &model.collapsed);
    c.Bind("title", &model.title);
    c.Bind("state_text", &model.stateText);
    c.Bind("count_text", &model.countText);
    c.Bind("time_text", &model.timeText);
    c.Bind("can_start", &model.canStart);
    c.Bind("start_label", &model.startLabel);
    c.Bind("leave_label", &model.leaveLabel);
    c.Bind("result_shown", &model.resultShown);
    c.Bind("result_success", &model.resultSuccess);
    c.Bind("result_title", &model.resultTitle);
    c.Bind("result_line1", &model.resultLine1);
    c.Bind("result_line2", &model.resultLine2);
    c.Bind("result_time", &model.resultTime);
    c.Bind("result_experience", &model.resultExperience);
    c.Bind("ok_label", &model.okLabel);

    const auto queue = [this](std::function<void()> action) { m_PendingActions.push_back(std::move(action)); };
    c.BindEventCallback("dg_toggle",
        [this, queue](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            queue([this]
            {
                auto& model = m_RmlView.GetModel();
                model.collapsed = !model.collapsed;
                m_RmlView.MarkDirty("collapsed");
                PlayBuffer(SOUND_CLICK01);
            });
        });
    c.BindEventCallback("dg_start",
        [queue](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            queue([] { SendSimple(ResultSubCode); PlayBuffer(SOUND_CLICK01); });
        });
    c.BindEventCallback("dg_leave",
        [queue](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            queue([] { SendSimple(MessageSubCode); PlayBuffer(SOUND_CLICK01); });
        });
    c.BindEventCallback("dg_result_ok",
        [this, queue](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&)
        {
            queue([this]
            {
                auto& m = m_RmlView.GetModel();
                m.resultShown = false;
                m_RmlView.MarkDirty("result_shown");
                PlayBuffer(SOUND_CLICK01);
            });
        });
}

void CDungeonTracker::Show(bool bShow)
{
    CObject::Show(bShow);
    SyncVisibility();
}

bool CDungeonTracker::UpdateMouseEvent()
{
    return !m_DocumentShown || !UI::RmlBridge::IsPointerOver(m_RmlView.Document());
}

float CDungeonTracker::GetLayerDepth()
{
    static const float depth = UI::RmlBridge::StackingDepthForDocument("dungeon_tracker.rml").value_or(3.61f);
    return depth;
}

std::string CDungeonTracker::DungeonName(int id) const
{
    for (const auto& dungeon : g_lastWindow.dungeons)
    {
        if (dungeon.id == id)
            return dungeon.name;
    }

    static const char* const Names[] = {"Kardamahal", "Kanturu", "Nightmare's Lair", "Curse of Crywolf", "Karutan Underground", "Imperial Guardian", "Shadow Abyss"};
    return id >= 0 && id < 7 ? Names[id] : std::string("Dungeon");
}

void CDungeonTracker::OnState(const Paris::Dungeons::RunInfo& run)
{
    m_Run = run;
    m_LastSecondShown = ~0ull;
    Refresh();
}

void CDungeonTracker::OnResult(bool success, int dungeon, int difficulty, unsigned seconds, int experiencePercent)
{
    m_RmlView.Ensure();
    if (!m_RmlView.Document())
        return;

    auto& model = m_RmlView.GetModel();
    model.resultShown = true;
    model.resultSuccess = success;
    model.resultTitle = Narrow(I18N::Game::DungeonResult);
    model.resultLine1 = Narrow(success ? I18N::Game::DungeonSuccess : I18N::Game::DungeonFailed);
    model.resultLine2 = success ? DungeonName(dungeon) : Narrow(I18N::Game::DungeonComeBack);
    for (const auto& info : g_lastWindow.difficulties)
    {
        if (success && info.id == difficulty)
            model.resultLine2 += " (" + info.name + ")";
    }
    model.resultTime = WFormat(I18N::Game::DungeonCompleteTime, Widen(Clock(seconds)).c_str());
    model.resultExperience = WFormat(I18N::Game::DungeonExperienceRate, experiencePercent);
    model.okLabel = Narrow(I18N::Game::DungeonOk);
    for (const char* name : {"result_shown", "result_success", "result_title", "result_line1", "result_line2", "result_time", "result_experience", "ok_label"})
        m_RmlView.MarkDirty(name);
    PlayBuffer(SOUND_CLICK01);
    SyncVisibility();
}

void CDungeonTracker::Refresh()
{
    m_RmlView.Ensure();
    if (!m_RmlView.Document())
        return;

    auto& model = m_RmlView.GetModel();
    model.shown = m_Run.state != RunState::None;
    const std::uint64_t elapsed = (NowMilliseconds() - m_Run.receivedAt) / 1000;
    const unsigned left = m_Run.secondsLeft > elapsed ? static_cast<unsigned>(m_Run.secondsLeft - elapsed) : 0u;
    const std::wstring name = Widen(DungeonName(m_Run.dungeon));
    if (m_Run.state == RunState::Play && m_Run.stage > m_Run.stageCount)
        model.title = WFormat(I18N::Game::DungeonBossStage, name.c_str());
    else if (m_Run.stage > 0)
        model.title = WFormat(I18N::Game::DungeonStage, name.c_str(), m_Run.stage);
    else
        model.title = Narrow(name.c_str());
    model.stateText = StateText(m_Run.state);
    model.countText = WFormat(I18N::Game::DungeonMonstersPlayers, m_Run.monstersLeft, m_Run.players);
    model.timeText = m_Run.state == RunState::Entrance ? WFormat(I18N::Game::DungeonEntranceCloses, static_cast<int>(left)) : Clock(left);
    model.canStart = m_Run.canStart;
    model.startLabel = Narrow(I18N::Game::DungeonStartNow);
    model.leaveLabel = Narrow(I18N::Game::DungeonLeave);
    for (const char* name2 : {"shown", "title", "state_text", "count_text", "time_text", "can_start", "start_label", "leave_label"})
        m_RmlView.MarkDirty(name2);
    SyncVisibility();
}

void CDungeonTracker::SyncVisibility()
{
    if (!m_RmlView.Document())
        return;

    const auto& model = m_RmlView.GetModel();
    const bool sceneAllowsShow = SceneFlag == MAIN_SCENE && LoadingWorld < 30;
    const bool show = IsVisible() && sceneAllowsShow && (model.shown || model.resultShown);
    if (show == m_DocumentShown)
        return;

    m_DocumentShown = show;
    if (show)
        m_RmlView.Document()->Show();
    else
        m_RmlView.Document()->Hide();
}

bool CDungeonTracker::Update()
{
    m_RmlView.Ensure();
    if (!m_PendingActions.empty())
    {
        auto actions = std::move(m_PendingActions);
        m_PendingActions.clear();
        for (auto& action : actions)
            action();
    }

    if (!m_RmlView.Document())
        return true;

    // The clock counts down between the states of the server (one a second).
    const std::uint64_t second = NowMilliseconds() / 1000;
    if (m_Run.state != RunState::None && second != m_LastSecondShown)
    {
        m_LastSecondShown = second;
        Refresh();
    }

    SyncVisibility();
    return true;
}
} // namespace mu::ui::window
