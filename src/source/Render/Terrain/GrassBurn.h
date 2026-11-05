#pragma once
namespace Render::Terrain::GrassBurn
{
void BeginFrame();
void Ignite(float worldX, float worldY);
float Amount(int tileX, int tileY);
}
