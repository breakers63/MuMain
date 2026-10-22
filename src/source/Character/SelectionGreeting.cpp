#include "stdafx.h"
#include "Character/SelectionGreeting.h"
#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/AI/ZzzAI.h"
#include "Render/Models/ZzzBMD.h"
#include "Scenes/SceneCore.h"
#include "Scenes/SceneCommon.h"
#include <array>
#include <cstdlib>
namespace CharacterSelection::Greeting
{
namespace
{
constexpr std::array<int, 7> Clips = {PLAYER_SMILE1, PLAYER_WIN1, PLAYER_RESPECT1,
    PLAYER_GREETING1, PLAYER_CLAP1, PLAYER_ROCK, PLAYER_GESTURE1};
constexpr DWORD MaximumGreetingMs = 5000;
int selected = -1, lastClip = -1, playing = -1;
DWORD started = 0;
bool Valid(int slot)
{
    return slot >= 0 && slot < MAX_CHARACTERS_PER_ACCOUNT && CharactersClient[slot].Object.Live
        && CharactersClient[slot].ID[0] != L'\0';
}
void Stop()
{
    if (Valid(playing)) SetPlayerStop(&CharactersClient[playing]);
    playing = -1;
}
void Play(int slot)
{
    if (!Valid(slot)) return;
    auto& character = CharactersClient[slot];
    const int pet = character.Helper.Type;
    if (pet == MODEL_HORN_OF_FENRIR || pet == MODEL_DARK_HORSE_ITEM
        || pet == MODEL_HORN_OF_UNIRIA || pet == MODEL_HORN_OF_DINORANT) return;
    int candidate = rand() % (static_cast<int>(Clips.size()) - 1);
    int clip = Clips[candidate];
    if (lastClip >= 0)
    {
        for (int value : Clips)
        {
            if (value == lastClip) continue;
            if (candidate-- == 0) { clip = value; break; }
        }
    }
    else clip = Clips[rand() % Clips.size()];
    lastClip = clip;
    if (gCharacterManager.IsFemale(character.Class) && clip != PLAYER_RESPECT1 && clip != PLAYER_ROCK) ++clip;
    auto& object = character.Object;
    const auto& model = Models[object.Type];
    if (!model.Actions || clip >= model.NumActions || model.Actions[clip].NumAnimationKeys <= 1) return;
    SetAction(&object, clip);
    object.AnimationFrame = 0;
    playing = slot; started = timeGetTime();
}
}
void Reset() { Stop(); selected = -1; lastClip = -1; }
void Click(int slot)
{
    Stop(); selected = slot; Play(slot);
}
void Update()
{
    if (SceneFlag != CHARACTER_SCENE) { Reset(); return; }
    if (selected != SelectedHero) Click(SelectedHero);
    if (playing >= 0 && timeGetTime() - started > MaximumGreetingMs) Stop();
}
}
