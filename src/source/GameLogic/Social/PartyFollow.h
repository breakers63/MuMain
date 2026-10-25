#pragma once
#include <string>
namespace GameLogic::Social::PartyFollow
{
bool Active();
bool IsFollowing(const wchar_t* name);
void Toggle(int partyIndex);
void Cancel();
void Poll();
void Update();
}
