#pragma once
#include <array>
namespace UI::Character::RankUser
{
extern std::array<GLuint, 32> Emblems;
void Load();
void Unload();
bool Receive(const BYTE* packet, int size);
void Render();
}
