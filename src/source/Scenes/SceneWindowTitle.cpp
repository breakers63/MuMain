#include "stdafx.h"
#include "Scenes/SceneWindowTitle.h"
#include "Scenes/SceneCore.h"
#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"
#include "Core/Platform/GameWindow.h"
#include "Core/Text/Utf8.h"
#include <SDL3/SDL_video.h>
#include <ctime>
#include <string>

namespace Scenes::WindowTitle
{
namespace
{
std::string ClockText()
{
    const auto now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char clock[16]{};
    std::strftime(clock, sizeof(clock), "%H:%M:%S", &local);
    return clock;
}
}

void Update()
{
    static std::uint64_t nextUpdate = 0;
    const auto now = GetTickCount64();
    if (now < nextUpdate) return;
    constexpr unsigned UpdateMilliseconds = 1000;
    nextUpdate = now + UpdateMilliseconds;
    SDL_Window* window = Core::Platform::GameWindow();
    if (!window) return;
    std::string title = Core::Platform::GameWindowTitle;
    if (SceneFlag == MAIN_SCENE && CharacterAttribute && Hero)
    {
        title += " | " + Core::Text::ToUtf8(CharacterAttribute->Name);
        title += " | Nivel: " + std::to_string(CharacterAttribute->Level);
        title += " | Master: " + std::to_string(gCharacterManager.IsMasterLevel(Hero->Class) ? Master_Level_Data.nMLevel : 0);
        title += " | Resets: " + std::to_string(CharacterAttribute->Resets);
        title += " | " + ClockText();
    }
    if (title != SDL_GetWindowTitle(window))
        SDL_SetWindowTitle(window, title.c_str());
}
}
