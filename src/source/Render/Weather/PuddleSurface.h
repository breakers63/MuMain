#pragma once
namespace Render::Weather::Puddles {
// Ground-only wet patches, stable in map coordinates and faded by accumulated wetness.
void Draw(int map,float wetness,float night,float lightning);
}
