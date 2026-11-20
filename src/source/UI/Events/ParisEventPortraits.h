#pragma once
#include <string_view>
namespace UI::Events
{
// Visual defaults only. The live detail takes precedence.
inline int GuidePortrait(std::string_view key)
{
    if (key == "timer-4") return 44;
    if (key == "timer-5") return 79;
    if (key == "timer-6") return 135;
    if (key == "timer-7") return 295;
    if (key == "timer-8") return 275;
    if (key == "timer-10") return 150;
    if (key == "timer-11") return 570;
    if (key == "timer-12") return 418;
    if (key == "timer-13") return 424;
    if (key == "timer-15") return 561;
    if (key == "timer-16") return 504;
    if (key == "raklion") return 459;
    if (key == "dungeon-0") return 701;
    if (key == "dungeon-1") return 750;
    if (key == "dungeon-3") return 698;
    if (key == "dungeon-4") return 719;
    return -1;
}
}
