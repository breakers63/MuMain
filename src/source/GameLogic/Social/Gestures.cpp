#include "stdafx.h"
#include "GameLogic/Social/Gestures.h"
#include "Engine/AI/ZzzAI.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzObject.h"
namespace GameLogic::Social
{
bool PerformGesture(int index)
{
    if (!Hero || index < 0 || index >= static_cast<int>(Gestures.size())) return false;
    auto& object = Hero->Object;
    const int current = object.CurrentAction;
    const bool idle = (current >= PLAYER_STOP_MALE && current <= PLAYER_STOP_RIDE_WEAPON)
        || current == PLAYER_STOP_TWO_HAND_SWORD_TWO || current == PLAYER_RAGE_UNI_STOP_ONE_RIGHT
        || current == PLAYER_STOP_RAGEFIGHTER;
    if (!idle || Hero->Dead || Hero->Movement) return false;
    const auto& gesture = Gestures[index];
    // Rock/paper/scissors and the final four clips have no adjacent female variant.
    if (index >= 18)
    {
        SetAction(&object, gesture.animation);
        SendRequestAction(object, static_cast<BYTE>(gesture.serverAction));
    }
    else SetActionClass(Hero, &object, gesture.animation, gesture.serverAction);
    return true;
}
}
