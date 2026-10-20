#pragma once
namespace UI::Options::Gold
{
inline constexpr int Width = 430, Height = 423;
inline constexpr int CloseX = Width - 27, CloseY = 10, CloseSize = 15;
void Load();
void Unload();
void Frame(int x, int y);
void VisualSettings(int x, int y);
bool HandleVisualSettings(int x, int y);
void Volume(int x, int y, int level);
void EffectLevels(int x, int y, int level);
}
