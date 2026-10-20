#pragma once
namespace UI::Character::Portraits
{
void Load();
void Unload();
void RenderPortrait(int baseClass, int x, int y, int width, int height);
void Render(int baseClass, int x, int y, int width, int height);
} // namespace UI::Character::Portraits
