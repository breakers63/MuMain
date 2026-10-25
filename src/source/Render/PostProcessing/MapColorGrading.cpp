#include "stdafx.h"

#include "Render/PostProcessing/MapColorGrading.h"

#include "Core/Utilities/Log/MuLogger.h"
#include "Render/PostProcessing/ColorGradingTable.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Weather/WeatherSettings.h"
#include "Render/Weather/WeatherRenderer.h"

#include <optional>
#include <tuple>

namespace Render::PostProcessing
{
namespace
{
std::optional<std::tuple<int, bool, int>> s_selection;

void ApplyTable(const GradingTableLoad& table, int mapNumber, const std::filesystem::path& path)
{
    const auto logger = mu::log::Get("render");
    switch (table.status)
    {
    case GradingTableStatus::Missing:
        logger->info("Colour grading: map {} has no table ({}); not graded", mapNumber, path.string());
        break;
    case GradingTableStatus::Invalid:
        logger->warn("Colour grading: {} is invalid ({}); map {} not graded", path.string(), table.error, mapNumber);
        break;
    case GradingTableStatus::Loaded:
        if (mu::GetRenderer().SetColorGradingTable(table.rgba))
        {
            logger->info("Colour grading: map {} graded by {}", mapNumber, path.string());
            return;
        }
        logger->warn("Colour grading: {} could not be uploaded; map {} not graded", path.string(), mapNumber);
        break;
    }
    (void)mu::GetRenderer().SetColorGradingTable({});
}
} // namespace

void SelectMapColorGrading(int mapNumber)
{
    const auto& options = Render::Weather::Options();
    const bool pegasus = options.naturalColors && Render::Weather::Current().outdoor;
    const auto selection = std::tuple{mapNumber, pegasus, options.quality};
    if (s_selection == selection)
    {
        return;
    }
    s_selection = selection;

    const auto path = pegasus
        ? std::filesystem::path("Data/Grading/Pegasus") / (options.quality == 2 ? "ultra.png" : "modern.png")
        : GetGradingTablePath(mapNumber);
    ApplyTable(LoadGradingTable(path), mapNumber, path);
}

} // namespace Render::PostProcessing
