#pragma once
#include "WeatherCycle.h"
namespace Render::Weather
{
Settings& Options();
void SaveOptions();
void RestartCycle();
std::uint64_t ElapsedMs();
} // namespace Render::Weather
