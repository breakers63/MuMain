#include "stdafx.h"
#include "GameLogic/Social/PartyFollow.h"
#include "Network/Paris/PartyFinderProtocol.h"
#include "Network/Server/WSclient.h"
#include "Core/Input/UiInputRouter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInventory.h"
#include "Engine/AI/ZzzAI.h"
#include "World/MapInfra/MapManager.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Core/WindowCommon.h"
#include <algorithm>
#include <cmath>
namespace GameLogic::Social::PartyFollow
{
namespace
{
std::wstring target;
DWORD lastPoll = 0, lastPath = 0, started = 0, lastValid = 0;
int request = 0, anchorMap = -1, anchorX = 0, anchorY = 0;
constexpr DWORD PollInterval = 750, PathInterval = 400, ResponseTimeout = 6000;
constexpr int ArrivalDistance = 2;
void StopWalking()
{
    if (!Hero) return;
    if (Hero->Movement && !Hero->Dead && Hero->Object.Teleport == TELEPORT_NONE
        && SocketClient && SocketClient->IsConnected()) LetHeroStop(Hero, TRUE);
    Hero->Movement = false;
    Hero->Path.PathNum = 0;
}
bool MemberPresent()
{
    for (int i = 0; i < PartyNumber; ++i)
        if (target == Party[i].Name) return true;
    return false;
}
}
bool Active() { return !target.empty(); }
bool IsFollowing(const wchar_t* name) { return Active() && name && target == name; }
void Cancel()
{
    const bool wasActive = Active();
    target.clear(); request = 0; lastValid = 0; anchorMap = -1;
    if (wasActive) StopWalking();
}
void Toggle(int partyIndex)
{
    if (!Hero || partyIndex < 0 || partyIndex >= PartyNumber || !wcscmp(Party[partyIndex].Name, Hero->ID)) return;
    if (IsFollowing(Party[partyIndex].Name)) { Cancel(); return; }
    if (!Network::Paris::PartyFinder::Data.followEnabled) return;
    Cancel();
    target = Party[partyIndex].Name;
    started = timeGetTime(); lastPoll = started - PollInterval; lastPath = started - PathInterval;
    g_iFollowCharacter = -1;
}
void Poll()
{
    if (!Active())
    {
        static DWORD lastSettingsPoll = 0;
        if (Hero && PartyNumber > 0 && timeGetTime() - lastSettingsPoll >= 3000)
        {
            lastSettingsPoll = timeGetTime();
            Network::Paris::PartyFinder::Request(0);
        }
        return;
    }
    if (!Network::Paris::PartyFinder::Data.followEnabled) { Cancel(); return; }
    if (!Hero || Hero->Dead || PartyNumber == 0 || !MemberPresent()
        || mu::ui::window::IsPress(VK_ESCAPE)
        || (mu::ui::window::IsPress(VK_RBUTTON) && !Core::Input::IsMouseOverUI())) { Cancel(); return; }
    const DWORD now = timeGetTime();
    if (Hero->Object.Teleport != TELEPORT_NONE)
    {
        request = 0; lastValid = 0; started = now; lastPoll = now;
        return;
    }
    const auto& data = Network::Paris::PartyFinder::Data;
    if (request != 0 && data.request == request)
    {
        request = 0;
        if (data.status != 0 || !data.hasTarget || data.targetName != StringUtils::WideToNarrow(target.c_str()))
        { Cancel(); return; }
        lastValid = now; anchorMap = data.targetMap; anchorX = data.targetX; anchorY = data.targetY;
    }
    if (now - (lastValid ? lastValid : started) > ResponseTimeout) { Cancel(); return; }
    if (now - lastPoll >= PollInterval)
    {
        lastPoll = now;
        request = Network::Paris::PartyFinder::Request(6, StringUtils::WideToNarrow(target.c_str()));
    }
}
void Update()
{
    if (!Active() || !Hero || Hero->Dead) return;
    const DWORD now = timeGetTime();
    OBJECT* object = &Hero->Object;
    if (lastValid && anchorMap == gMapManager.WorldActive && now - lastPath >= PathInterval)
    {
        lastPath = now;
        const int distance = std::max(std::abs(static_cast<int>(Hero->PositionX) - anchorX),
                                      std::abs(static_cast<int>(Hero->PositionY) - anchorY));
        if (distance <= ArrivalDistance) StopWalking();
        else if (!g_isCharacterBuff(object, eDeBuff_Stun) && !g_isCharacterBuff(object, eDeBuff_Sleep)
                 && Hero->Object.Teleport == TELEPORT_NONE)
        {
            Hero->MovementType = MOVEMENT_MOVE;
            if (PathFinding2(Hero->PositionX, Hero->PositionY, anchorX, anchorY, &Hero->Path))
                SendMoveTo(Hero, &Hero->Object, anchorX, anchorY);
            else { Cancel(); return; }
        }
    }
}

}
