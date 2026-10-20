#pragma once
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
namespace UI::Character::ParisStyle
{
void Solid(float x, float y, float w, float h, float r, float g, float b);
void Box(int x, int y, int w, int h, bool selected = false);
void ScrollBar(int x, int y, int width, int height, int thumbY, int thumbHeight, bool hovered);
void Label(int x, int y, int w, const wchar_t* text);
void Panel(GLuint texture, const POINT& position);
void Footer(GLuint atlas, int x, int y, int tile, bool enabled = true);
}
