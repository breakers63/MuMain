// Breda: the dungeons of MuDream. The server side is the Breda.Dungeons plugin of OpenMU; both talk

// through the packets C1/C2 7E described in DungeonWindow.cpp.

//  - CDungeonWindow: opened at the Dungeon Entrance NPC: the dungeons, the difficulties with their

//    boost and requirements, and Join.

//  - CDungeonTracker: always on screen while the hero is in a dungeon: the stage, the monsters left,

//    the players, the time and Start now; and the result when the dungeon ends.

#pragma once



#include <cstdint>

#include <functional>

#include <span>

#include <string>

#include <vector>



#include "UI/Core/WindowObject.h"

#include "UI/Core/WindowManager.h"

#include "UI/RmlBridge/RmlThemedView.h"



namespace Rml { class ElementDocument; }



namespace Paris::Dungeons

{

// Handles a packet with the main code 7E from the game server.

void ReceivePacket(std::span<const BYTE> packet);



struct DungeonInfo

{

    int id = 0;

    bool active = false;

    int map = 0;

    int stages = 0;

    bool resetsOk = true;  // the character's resets are in the range of the dungeon

    int minimumResets = 0;

    int maximumResets = -1; // -1: no limit

    std::string name;

};



struct DifficultyInfo

{

    int id = 0;

    bool meets = false;

    int healthPercent = 100;

    int damagePercent = 100;

    int defensePercent = 100;

    int experiencePercent = 100;

    int level = 0;

    int resets = 0;

    int grandResets = 0;

    int itemGroup = -1;

    int itemNumber = 0;

    int itemLevel = 0;

    bool hasItem = false;

    bool requiresMaster = false;

    std::string name;

};



struct WindowInfo

{

    bool received = false;

    bool enabled = false;

    int mode = 0; // 0 level, 1 resets, 2 grand resets

    int entriesLeft = -1;

    int entriesPerDay = 0;

    int level = 0;

    int resets = 0;

    int grandResets = 0;

    unsigned powerScore = 0;

    int partyDungeon = -1;

    int partyDifficulty = -1;

    int partyState = 0;

    bool canStart = false;

    std::vector<DungeonInfo> dungeons;

    std::vector<DifficultyInfo> difficulties;

};



// The state of the run the hero is in (Text_Eng.xml 9450-9455 of MuDream).

enum class RunState : int

{

    None = 0,

    Entrance = 1,

    Prepare = 2,

    Play = 3,

    Finish = 4,

    Clear = 5,

};



struct RunInfo

{

    RunState state = RunState::None;

    int dungeon = 0;

    int difficulty = 0;

    int stage = 0;

    int stageCount = 0;

    int monstersLeft = 0;

    int players = 0;

    unsigned secondsLeft = 0;

    bool canStart = false;

    std::uint64_t receivedAt = 0;

};

} // namespace Paris::Dungeons



namespace mu::ui::window

{

class CDungeonWindow : public CObject

{

public:

    CDungeonWindow();

    ~CDungeonWindow() override;



    bool Create(CManager* pNewUIMng);

    void Release();



    void Show(bool bShow) override;



    bool UpdateMouseEvent() override;

    bool UpdateKeyEvent() override;

    bool Update() override;

    bool Render() override { return true; }

    float GetLayerDepth() override;

    float GetKeyEventOrder() override { return 9.0f; }



    void OpeningProcess();

    void ClosingProcess() {}



    void OnWindow(const Paris::Dungeons::WindowInfo& info);

    void OnMessage(int message, int argument);



private:

    struct DungeonEntry

    {

        int index = 0;

        int id = 0;

        Rml::String name;

        Rml::String buttonSrc;

        bool active = false;

        bool selected = false;

        bool party = false;

    };



    struct DifficultyEntry

    {

        int index = 0;

        int id = 0;

        Rml::String name;

        Rml::String iconSrc;

        bool selected = false;

        bool meets = false;

    };



    struct DungeonRmlModel

    {

        Rml::String title;

        Rml::String selectLabel;

        Rml::String difficultyLabel;

        Rml::String boostLabel;

        Rml::String requirementsLabel;

        Rml::String artSrc;

        Rml::String dungeonName;

        Rml::String description;

        Rml::String powerText;

        Rml::String healthText;

        Rml::String damageText;

        Rml::String defenseText;

        Rml::String experienceText;

        Rml::String minimumLevel;

        Rml::String resetsRange;

        Rml::String yourResets;

        Rml::String requirementText;

        Rml::String entriesText;

        Rml::String itemText;

        bool requirementMet = false;

        bool entriesMet = false;

        bool itemMet = false;

        Rml::String joinLabel;

        bool canJoin = false;

        Rml::String statusText;

        bool waiting = true;

        std::vector<DungeonEntry> dungeons;

        std::vector<DifficultyEntry> difficulties;

    };



    void BindRmlModel(Rml::DataModelConstructor& c, DungeonRmlModel& model);

    void SelectDungeon(int index);

    void SelectDifficulty(int index);

    void Join();

    void Rebuild();

    void UpdateParisPresentation();



    UI::RmlBridge::ThemedView<DungeonRmlModel> m_RmlView{

        "dungeon",

        [this](Rml::DataModelConstructor& c, DungeonRmlModel& model) { BindRmlModel(c, model); },

        {{"Data/Interface/RmlUi/dungeon.rml"}}};



    CManager* m_pNewUIMng = nullptr;

    Paris::Dungeons::WindowInfo m_Info;

    int m_SelectedDungeon = 0;

    int m_SelectedDifficulty = 0;

    std::vector<std::function<void()>> m_PendingActions;

};



class CDungeonTracker : public CObject

{

public:

    CDungeonTracker();

    ~CDungeonTracker() override;



    bool Create(CManager* pNewUIMng);

    void Release();



    void Show(bool bShow) override;



    bool UpdateMouseEvent() override;

    bool UpdateKeyEvent() override { return true; }

    bool Update() override;

    bool Render() override { return true; }

    float GetLayerDepth() override;



    void OnState(const Paris::Dungeons::RunInfo& run);

    void OnResult(bool success, int dungeon, int difficulty, unsigned seconds, int experiencePercent);



    // The name of a dungeon as the last window told it (or its default name).

    std::string DungeonName(int id) const;



private:

    struct DungeonTrackerRmlModel

    {

        bool shown = false;

        bool collapsed = false;

        Rml::String title;

        Rml::String stateText;

        Rml::String countText;

        Rml::String timeText;

        bool canStart = false;

        Rml::String startLabel;

        Rml::String leaveLabel;

        bool resultShown = false;

        bool resultSuccess = false;

        Rml::String resultTitle;

        Rml::String resultLine1;

        Rml::String resultLine2;

        Rml::String resultTime;

        Rml::String resultExperience;

        Rml::String okLabel;

    };



    void BindRmlModel(Rml::DataModelConstructor& c, DungeonTrackerRmlModel& model);

    void Refresh();

    void SyncVisibility();



    UI::RmlBridge::ThemedView<DungeonTrackerRmlModel> m_RmlView{

        "dungeon_tracker",

        [this](Rml::DataModelConstructor& c, DungeonTrackerRmlModel& model) { BindRmlModel(c, model); },

        {{"Data/Interface/RmlUi/dungeon_tracker.rml"}}};



    CManager* m_pNewUIMng = nullptr;

    Paris::Dungeons::RunInfo m_Run;

    bool m_DocumentShown = false;

    std::uint64_t m_LastSecondShown = ~0ull;

    std::vector<std::function<void()>> m_PendingActions;

};



CDungeonWindow* GetDungeonWindow();

CDungeonTracker* GetDungeonTracker();

} // namespace mu::ui::window

