#pragma once

namespace Render::Weather::SnowSurface
{
void Reset(int map);
void Update(float seconds, float snowfall);
void Draw();
float Cover();
bool IsExposed(float x, float y);
}
