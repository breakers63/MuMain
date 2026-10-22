#include "stdafx.h"
#include "Render/Terrain/GrassBurn.h"
#include "Render/Terrain/GrassBurnState.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Render/Textures/ZzzTexture.h"
#include "Render/Effects/ZzzEffect.h"
#include "World/MapInfra/MapManager.h"
#include "Scenes/SceneCore.h"
extern bool TerrainGrassEnable;
namespace Render::Terrain::GrassBurn
{
namespace
{
constexpr float WorldUnitsPerTile=100.f;
constexpr DWORD EmberEveryMs=180;
constexpr int EmberPoints=6;
constexpr int MaximumEmissionsPerTick=12;
constexpr float GroundFlameScale=.38f;
constexpr float EmberSparkScale=.45f;
constexpr float EmberEdgeRadius=.85f;
constexpr double Tau=6.283185307179586;
GrassBurnState state;
int map=-1;
DWORD clock=0,nextEmber=0;
bool Fuel(int x,int y)
{
    if(x<0 || y<0 || x>=TERRAIN_SIZE || y>=TERRAIN_SIZE || !TerrainGrassEnable)return false;
    const int index=y*TERRAIN_SIZE+x;
    const float blend=TerrainMappingAlpha[index];
    const bool firstGrass=blend<1.f && Bitmaps.FindTexture(BITMAP_MAPGRASS+TerrainMappingLayer1[index])!=nullptr;
    const bool secondGrass=blend>0.f && Bitmaps.FindTexture(BITMAP_MAPGRASS+TerrainMappingLayer2[index])!=nullptr;
    return firstGrass || secondGrass;
}
void SyncMap()
{
    if(map==gMapManager.WorldActive)return;
    state.Reset();map=gMapManager.WorldActive;nextEmber=0;
}
}
void Ignite(float x,float y)
{
    if(SceneFlag!=MAIN_SCENE)return;
    SyncMap();
    state.Ignite(x/WorldUnitsPerTile,y/WorldUnitsPerTile,timeGetTime(),Fuel);
}
float Amount(int x,int y) { return SceneFlag==MAIN_SCENE ? state.Amount(x,y,clock) : 0.f; }
void BeginFrame()
{
    SyncMap();clock=timeGetTime();
    if(SceneFlag!=MAIN_SCENE)return;
    state.Step(clock,Fuel);
    if(nextEmber && static_cast<LONG>(clock-nextEmber)<0)return;
    nextEmber=clock+EmberEveryMs;
    int emissions=0;
    for(const auto& fire:state.Fires())
    {
        if(!fire.active)continue;
        for(int point=0;point<EmberPoints;++point)
        {
            const double angle=clock*.004+point*Tau/EmberPoints;
            const float x=fire.x+static_cast<float>(std::sin(angle))*fire.radius*EmberEdgeRadius;
            const float y=fire.y+static_cast<float>(std::cos(angle))*fire.radius*EmberEdgeRadius;
            if(emissions>=MaximumEmissionsPerTick)return;
            if(!Fuel(static_cast<int>(x),static_cast<int>(y)))continue;
            vec3_t position={x*WorldUnitsPerTile,y*WorldUnitsPerTile,0.f};
            position[2]=RequestTerrainHeight(position[0],position[1])+5.f;
            vec3_t direction={0.f,0.f,0.f},light={1.f,.35f,.04f};
            // This emitter is already clock-throttled; FPS sampling would
            // randomly discard its sparse ground flames a second time.
            CreateParticle(BITMAP_FIRE,position,direction,light,0,GroundFlameScale);
            if(emissions%3==0)
                CreateParticle(BITMAP_SPARK+1,position,direction,light,0,EmberSparkScale);
            ++emissions;
        }
    }
}
}
