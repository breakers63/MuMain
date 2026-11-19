#include "stdafx.h"



#include "Render/Look/LookProfileSetting.h"

#include "Render/Look/LookFiles.h"



#include "Core/Utilities/Log/MuLogger.h"

#include "Render/Lighting/LightMoodSetting.h"

#include "Render/Look/LookTuning.h"

#include "World/MapInfra/MapManager.h"



#if defined(MU_ENABLE_CONTROL_SOCKET) || defined(_EDITOR)

#include "Core/Text/Utf8.h"

#include "Render/Look/LookPresetSetting.h"

#include "Render/Look/LookTuningCommand.h"

#endif



#include <limits>

#include <utility>

#include <algorithm>



namespace Render::Look

{

namespace

{

using Data::Maps::LookValue;

using Data::Maps::LookValues;



constexpr int NoWorld = std::numeric_limits<int>::min();



struct State

{

    // The live documents and values, and the files they were loaded from (for reset).

    LookFiles live;

    LookFiles loaded;

    std::string source = "shipped";

    std::uint32_t revision = 0;

    // The world and revision `values` belong to; NoWorld recomputes them.

    int world = NoWorld;

    std::uint32_t valuesRevision = 0;

    LookValues values = Data::Maps::BuiltInLookValues();

    bool moodTuningDirty = false;

};



State s_state;



void Changed()

{

    ++s_state.revision;

    s_state.world = NoWorld;

}



void LogIssues(const std::vector<Data::Items::ItemDataIssue>& issues)

{

    for (const Data::Items::ItemDataIssue& issue : issues)

    {

        mu::log::Get("render")->warn("Look profile: {}", issue.ToString());

    }

}



void UseFiles(LookFiles files, std::string source)

{

    s_state.live = files;

    s_state.loaded = std::move(files);

    s_state.source = std::move(source);

    Changed();

    Lighting::LightMood::SetMoodData(s_state.live.moodData);

}



LookFilesLoad LoadShipped(LookFileSystem& fileSystem)

{

    LookFilesLoad loaded = LoadLookFiles(fileSystem, Data::Maps::LookProfileFile, Data::Maps::LightMoodFile);

    LogIssues(loaded.issues);

    if (!loaded.files.lookUsable)

    {

        mu::log::Get("render")->warn("Look profile unusable; the built-in values apply");

    }

    if (!loaded.files.moodUsable)

    {

        mu::log::Get("render")->warn("Light mood data unusable; the world keeps its Classic light");

    }

    return loaded;

}

} // namespace



void LoadProfile()

{

    DiskLookFileSystem fileSystem;

    UseFiles(LoadShipped(fileSystem).files, "shipped");

}



const LookValues& Active()

{

    const int world = gMapManager.WorldActive;

    if (world != s_state.world || s_state.valuesRevision != s_state.revision)

    {

        s_state.world = world;

        s_state.valuesRevision = s_state.revision;

        s_state.values = s_state.live.lookData.ForWorld(world);

    }

    return s_state.values;

}



float Value(LookValue value)

{

    return static_cast<float>(Active()[value]);

}



std::uint32_t Revision()

{

    return s_state.revision;

}



std::optional<PlayerTuningState> DescribePlayerTuningValue(std::string_view name)

{

    const int world = gMapManager.WorldActive;

    const std::vector<LookValueState> described = DescribeLookValues(s_state.live, world, name);

    if (described.empty())

        return std::nullopt;

    const LookValueState& value = described.front();

    PlayerTuningState state{value.value, value.min, value.max};

    if (name == "mood.base")

        state.max = std::min(state.max, s_state.live.moodData.ForWorld(world).figures);

    else if (name == "mood.figures")

        state.min = std::max(state.min, s_state.live.moodData.ForWorld(world).base);

    return state;

}



bool ApplyPlayerTuningValue(std::string_view name, double newValue)

{

    const LookFiles previous = s_state.live;

    const LookEdit edit{std::string(name), newValue};

    const bool isMood = name.starts_with(MoodNamePrefix);

    const int world = gMapManager.WorldActive;

    const bool hasWorld = world >= 0 && world <= Data::Maps::MaxLookProfileWorld;

    const auto scope = isMood && hasWorld ? std::optional<int>(world) : std::nullopt;

    const LookEditResult result = SetLookValues(s_state.live, std::span<const LookEdit>(&edit, 1), scope);

    if (!result.applied)

    {

        s_state.live = previous;

        return false;

    }

    // Look-profile values are global defaults with a current-map override so existing map looks

    // remain visible. Light-mood values are map-specific, since they control baked terrain light.

    if (!isMood && hasWorld)

    {

        const LookEditResult mapResult = SetLookValues(s_state.live, std::span<const LookEdit>(&edit, 1), world);

        if (!mapResult.applied)

        {

            s_state.live = previous;

            return false;

        }

    }

    s_state.moodTuningDirty = s_state.moodTuningDirty || result.moodChanged;

    Changed();

    return true;

}



bool SavePlayerTuning()

{

    DiskLookFileSystem fileSystem;

    const LookSaveResult saved =

        SaveLookFiles(fileSystem, s_state.live, Data::Maps::LookProfileFile, Data::Maps::LightMoodFile);

    if (!saved.saved)

        return false;

    s_state.loaded = s_state.live;

    if (s_state.moodTuningDirty)

    {

        Lighting::LightMood::SetMoodData(s_state.live.moodData);

        s_state.moodTuningDirty = false;

    }

    return true;

}



bool SetPlayerTuningValue(Data::Maps::LookValue value, double newValue)

{

    return SetPlayerTuningValue(Data::Maps::Info(value).name, newValue);

}



bool ApplyPlayerTuningValue(Data::Maps::LookValue value, double newValue)

{

    return ApplyPlayerTuningValue(Data::Maps::Info(value).name, newValue);

}



bool SetPlayerTuningValue(std::string_view name, double newValue)

{

    const LookFiles previous = s_state.live;

    if (!ApplyPlayerTuningValue(name, newValue))

        return false;

    if (!SavePlayerTuning())

    {

        s_state.live = previous;

        s_state.moodTuningDirty = false;

        Changed();

        return false;

    }

    return true;

}



Lighting::LightHistorySettings LightHistorySettings()

{

    const LookValues& values = Active();

    Lighting::LightHistorySettings settings;

    settings.mapSmoothingSeconds = static_cast<float>(values[LookValue::DynamicMapSmoothing]);

    settings.effectGain = static_cast<float>(values[LookValue::DynamicEffectGain]);

    settings.effectRangeScale = static_cast<float>(values[LookValue::DynamicEffectRange]);

    settings.effectRangeMax = static_cast<float>(values[LookValue::DynamicEffectRangeMax]);

    settings.effectFadeSeconds = static_cast<float>(values[LookValue::DynamicEffectFade]);

    settings.effectCap = static_cast<float>(values[LookValue::DynamicEffectCap]);

    return settings;

}



#if defined(MU_ENABLE_CONTROL_SOCKET) || defined(_EDITOR)

namespace

{

// Applies an edit's result: the next frame uses the live values; mood edits rebuild the static

// map light as a mood change does.

bool Apply(const LookEditResult& result, const char* what)

{

    if (!result.applied)

    {

        mu::log::Get("render")->warn("$look {} refused: {}", what, result.error);

        return false;

    }

    Changed();

    if (result.moodChanged)

    {

        Lighting::LightMood::SetMoodData(s_state.live.moodData);

    }

    return true;

}



std::string MapName(int world)

{

    const wchar_t* name = gMapManager.GetMapName(world);

    return name ? Core::Text::ToUtf8(name) : std::string();

}



void LogValue(const LookValueState& state, int world)

{

    mu::log::Get("render")->info(

        "$look {} = {} ({}{}), range {} to {}", state.name, state.value, ToString(state.source),

        state.source == LookValueSource::Map ? " " + std::to_string(world) : std::string(), state.min, state.max);

}



bool Show(std::optional<std::string_view> name)

{

    const std::vector<LookValueState> states = DescribeLive(name);

    if (states.empty())

    {

        mu::log::Get("render")->warn("$look show refused: {}: is not a look value", *name);

        return false;

    }

    const int world = gMapManager.WorldActive;

    mu::log::Get("render")->info("$look values on world {} from {}; preset {}, overrides: {}", world, s_state.source,

                                 ActiveLookPreset() == LookPreset::Enhanced ? "Enhanced" : "Classic",

                                 DescribeFeatureOverrides());

    for (const LookValueState& state : states)

    {

        LogValue(state, world);

    }

    mu::log::Get("render")->flush();

    return true;

}

} // namespace



const LookFiles& LiveFiles()

{

    return s_state.live;

}



const std::string& LiveSource()

{

    return s_state.source;

}



std::vector<LookValueState> DescribeLive(std::optional<std::string_view> name)

{

    return DescribeLookValues(s_state.live, gMapManager.WorldActive, name);

}



bool SetLive(std::span<const LookEdit> edits, bool currentMap)

{

    const int world = gMapManager.WorldActive;

    if (currentMap && world < 0)

    {

        mu::log::Get("render")->warn("$look set map refused: no map is active");

        return false;

    }

    const std::optional<int> scope = currentMap ? std::optional<int>(world) : std::nullopt;

    const LookEditResult result =

        SetLookValues(s_state.live, edits, scope, currentMap ? MapName(world) : std::string());

    if (!Apply(result, "set"))

    {

        return false;

    }

    for (std::size_t index = 0; index < edits.size(); ++index)

    {

        mu::log::Get("render")->info("$look set {}{}: {} -> {}", currentMap ? "map " + std::to_string(world) + " " : "",

                                     edits[index].name, result.oldValues[index], edits[index].value);

    }

    return true;

}



bool ResetLive(std::optional<std::string_view> name)

{

    const LookEditResult result = ResetLookValues(s_state.live, s_state.loaded, name, gMapManager.WorldActive);

    if (!Apply(result, "reset"))

    {

        return false;

    }

    mu::log::Get("render")->info("$look reset {} to the values loaded from {}",

                                 name ? std::string(*name) : "every value", s_state.source);

    return true;

}



bool LoadLive(std::string_view candidate)

{

    DiskLookFileSystem fileSystem;

    if (candidate.empty())

    {

        UseFiles(LoadShipped(fileSystem).files, "shipped");

        mu::log::Get("render")->info("$look load: the shipped files");

        return true;

    }

    const std::filesystem::path folder = std::filesystem::path("Data/Render/Candidates") / std::string(candidate);

    std::vector<Data::Items::ItemDataIssue> issues;

    std::optional<LookFiles> files = LoadCandidateFiles(fileSystem, folder / "Look.json", folder / "LightMood.json",

                                                        Data::Maps::LightMoodFile, issues);

    LogIssues(issues);

    if (!files)

    {

        mu::log::Get("render")->warn("$look load refused: candidate {} is unusable; nothing changed", candidate);

        return false;

    }

    UseFiles(std::move(*files), std::string(candidate));

    mu::log::Get("render")->info("$look load: candidate {}", candidate);

    return true;

}



bool SaveLive()

{

    DiskLookFileSystem fileSystem;

    const LookSaveResult result =

        SaveLookFiles(fileSystem, s_state.live, Data::Maps::LookProfileFile, Data::Maps::LightMoodFile);

    if (!result.saved)

    {

        mu::log::Get("render")->error("$look save failed: {}", result.message);

        return false;

    }

    // What was saved is now what a reset goes back to.

    s_state.loaded = s_state.live;

    mu::log::Get("render")->info("$look save: {}", result.message);

    return true;

}



bool HandleDeveloperCommand(std::wstring_view command)

{

    if (const std::optional<LookTuningCommand> alias = ParseLookTuningCommand(command))

    {

        std::vector<LookEdit> edits;

        edits.reserve(alias->count);

        for (std::size_t index = 0; index < alias->count; ++index)

        {

            edits.push_back(

                {Data::Maps::Info(alias->assignments[index].value).name, alias->assignments[index].newValue});

        }

        (void)SetLive(edits, false);

        return true;

    }

    const std::optional<LookCommand> look = ParseLookCommand(command);

    if (!look)

    {

        return false;

    }

    const std::optional<std::string_view> name =

        look->name.empty() ? std::nullopt : std::optional<std::string_view>(look->name);

    switch (look->verb)

    {

    case LookVerb::Show:

        (void)Show(name);

        break;

    case LookVerb::Set:

    {

        const LookEdit edit{look->name, look->value};

        (void)SetLive(std::span<const LookEdit>(&edit, 1), look->map);

        break;

    }

    case LookVerb::Reset:

        (void)ResetLive(name);

        break;

    case LookVerb::Load:

        (void)LoadLive(look->name);

        break;

    case LookVerb::Save:

        (void)SaveLive();

        break;

    }

    return true;

}

#endif



} // namespace Render::Look

