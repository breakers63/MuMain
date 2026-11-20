#pragma once
#include "Network/Paris/EventGuideProtocol.h"
#include <array>
#include <algorithm>
#include <string_view>
#include <vector>
#include <utility>

namespace UI::Events
{
inline constexpr int EventsPerGuidePage = 9;
struct GuidePage
{
    std::string_view category;
    std::vector<const Network::Paris::EventGuide::Event*> events;
};

inline std::string_view GuideCategory(const Network::Paris::EventGuide::Event& event)
{
    if (event.key.starts_with("dungeon-")) return "dungeon";
    return event.category;
}

inline const char* GuideCategoryName(std::string_view category)
{
    if (category == "classic") return "CLÁSICOS";
    if (category == "invasion") return "INVASIONES";
    if (category == "boss") return "JEFES";
    if (category == "bonus") return "BONOS";
    if (category == "dungeon") return "MAZMORRAS · CUSTOMS";
    return "CUSTOMS";
}

inline std::string GuideEventName(const Network::Paris::EventGuide::Event& event)
{
    if (event.key == "mini-1") return "Devil Square";
    if (event.key == "mini-2") return "Blood Castle";
    if (event.key == "mini-3") return "Chaos Castle";
    if (event.key == "mini-4") return "Illusion Temple";
    if (event.key == "mini-5") return "Doppelganger";
    if (event.key == "mini-6") return "Kanturu Refinery Tower";
    if (event.key == "mini-7") return "Imperial Guardian";
    return event.name;
}

inline std::pair<int, int> GuideEventPriority(const Network::Paris::EventGuide::Event& event)
{
    constexpr int Running = 0, Registration = 1, Scheduled = 2, AlwaysAvailable = 3, Inactive = 4;
    if (event.phase == "running") return {Running, 0};
    if (event.phase == "open" || event.phase == "registration" || event.phase == "prepare")
        return {Registration, std::max(0, event.seconds)};
    if (event.phase == "disabled" || event.phase == "unknown") return {Inactive, 0};
    if (event.phase == "available") return {AlwaysAvailable, 0};
    if (event.seconds >= 0) return {Scheduled, event.seconds};
    return {Inactive, 0};
}

inline std::vector<const Network::Paris::EventGuide::Event*> OrderedGuideEvents(
    const std::vector<Network::Paris::EventGuide::Event>& events, std::string_view category)
{
    std::vector<const Network::Paris::EventGuide::Event*> ordered;
    for (const auto& event : events)
        if (GuideCategory(event) == category) ordered.push_back(&event);
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto* left, const auto* right)
    {
        return GuideEventPriority(*left) < GuideEventPriority(*right);
    });
    return ordered;
}

inline std::vector<GuidePage> BuildGuidePages(const std::vector<Network::Paris::EventGuide::Event>& events)
{
    std::vector<GuidePage> pages;
    constexpr std::array<std::string_view, 6> categories{"classic", "invasion", "boss", "bonus", "dungeon", "custom"};
    for (const auto category : categories)
    {
        GuidePage page{category, {}};
        for (const auto* event : OrderedGuideEvents(events, category))
        {
            page.events.push_back(event);
            if (page.events.size() == EventsPerGuidePage)
            {
                pages.push_back(std::move(page));
                page = {category, {}};
            }
        }
        if (!page.events.empty()) pages.push_back(std::move(page));
    }
    return pages;
}
}
