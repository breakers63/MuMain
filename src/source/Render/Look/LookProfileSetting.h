#pragma once



#include "Data/GameData/MapData/LookProfile.h"

#include "Render/Lighting/DynamicLightHistory.h"



#include <cstdint>

#include <optional>

#include <string_view>



// The effective look profile: the live values of Data/Render/Look.json for the active world

// (developer edits included, Render/Look/LookTuning.h).

namespace Render::Look

{



// Reads Data/Render/Look.json and Data/Maps/LightMood.json (their problems are logged once; an

// unusable look profile keeps the built-in values, an unusable light mood the Classic light) and

// hands the light mood its data. Call once at startup.

void LoadProfile();



// The values of the active world (gMapManager.WorldActive).

[[nodiscard]] const Data::Maps::LookValues& Active();

[[nodiscard]] float Value(Data::Maps::LookValue value);



// Changes whenever the live profile changes (not with the world), so a cache of derived values

// knows when to recompute.

[[nodiscard]] std::uint32_t Revision();



// Player-facing tuning: edits the default look profile, applies it immediately and saves it.

// Returns false when either shipped graphics profile cannot be edited or persisted.

[[nodiscard]] bool SetPlayerTuningValue(Data::Maps::LookValue value, double newValue);

// Applies a transient slider value; save once when the drag ends.

[[nodiscard]] bool ApplyPlayerTuningValue(Data::Maps::LookValue value, double newValue);

[[nodiscard]] bool SavePlayerTuning();



struct PlayerTuningState

{

    double value = 0.0;

    double min = 0.0;

    double max = 0.0;

};

[[nodiscard]] std::optional<PlayerTuningState> DescribePlayerTuningValue(std::string_view name);

[[nodiscard]] bool SetPlayerTuningValue(std::string_view name, double newValue);

[[nodiscard]] bool ApplyPlayerTuningValue(std::string_view name, double newValue);



// The light history's settings from the active profile.

[[nodiscard]] Lighting::LightHistorySettings LightHistorySettings();



} // namespace Render::Look

