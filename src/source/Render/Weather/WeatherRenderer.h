#pragma once
#include "WeatherCycle.h"
namespace mu
{
struct FogParams;
struct SceneLighting;
} // namespace mu
namespace Render::Weather
{
void UpdateWorld(int map);
void SuspendWorld();
const State& Current();
void DrawSky();
void DrawWorld();
bool WantsFog();
void AdjustFog(mu::FogParams& fog);
void AdjustLighting(mu::SceneLighting& lighting);
} // namespace Render::Weather
