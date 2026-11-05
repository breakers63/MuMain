#include "stdafx.h"
#include "PuddleSurface.h"
#include "PuddleShape.h"
#include "SnowSurface.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/RenderUtils.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include <array>
#include <vector>
namespace Render::Weather::Puddles {
namespace {
constexpr int RadiusCells=7;
constexpr float CellSize=220.f, GroundOffset=1.7f, MaximumSlopeHeight=8.f;
std::vector<mu::Vertex3D> vertices;
struct Point {float x,y,z;};
Point Edge(float x,float y,float size,float angle,float rotation,float stretch,std::uint32_t seed,float ring)
{
    const float radius=EdgeRadius(angle,seed)*size*ring;
    const float px=std::cos(angle)*radius,py=std::sin(angle)*radius*stretch;
    const float wx=x+px*std::cos(rotation)-py*std::sin(rotation);
    const float wy=y+px*std::sin(rotation)+py*std::cos(rotation);
    return {wx,wy,RequestTerrainHeight(wx,wy)+GroundOffset};
}
void Vertex(const Point& p,std::uint32_t color)
{
    vertices.push_back({p.x,p.y,p.z,0,0,1,0,0,color});
}
void Patch(int cellX,int cellY,int map,float wetness,float night,float lightning)
{
    const auto seed=Seed(cellX,cellY,map);
    if(Hash(seed)<.48f)return;
    const float x=(cellX+.18f+Hash(seed+1)*.64f)*CellSize;
    const float y=(cellY+.18f+Hash(seed+2)*.64f)*CellSize;
    const int tx=static_cast<int>(x/TERRAIN_SCALE),ty=static_cast<int>(y/TERRAIN_SCALE);
    if(tx<1||ty<1||tx>=TERRAIN_SIZE-1||ty>=TERRAIN_SIZE-1)return;
    if((TerrainWall[TERRAIN_INDEX(tx,ty)]&TW_NOGROUND)!=0||!SnowSurface::IsExposed(x,y))return;
    const float size=38.f+Hash(seed+3)*55.f,stretch=.45f+Hash(seed+4)*.4f,rotation=Hash(seed+5)*Tau;
    const float ground=RequestTerrainHeight(x,y);
    const auto points=SamplePoints([&](float angle,float ring) {
        return Edge(x,y,size,angle,rotation,stretch,seed,ring);
    });
    // Water pools on nearly level terrain, never stretching up cliffs or through roofs.
    for(int i=0;i<Segments;++i) {
        const auto& p=points.back()[i];
        if(std::abs(p.z-GroundOffset-ground)>MaximumSlopeHeight||!SnowSurface::IsExposed(p.x,p.y))return;
    }
    const float distance=std::hypot(x-Hero->Object.Position[0],y-Hero->Object.Position[1]);
    const float fade=std::clamp((RadiusCells*CellSize-distance)/CellSize,0.f,1.f);
    const float sky=.13f*(1-night)+lightning*.45f;
    for(size_t ring=0;ring+1<Rings.size();++ring)for(int i=0;i<Segments;++i) {
        const int next=(i+1)%Segments;
        const auto inner=mu::PackABGR(.10f+sky,.16f+sky,.21f+sky,Opacity(wetness,Rings[ring])*fade);
        const auto outer=mu::PackABGR(.10f+sky,.16f+sky,.21f+sky,Opacity(wetness,Rings[ring+1])*fade);
        const auto& p0=points[ring][i];
        const auto& p1=points[ring][next];
        const auto& p2=points[ring+1][next];
        const auto& p3=points[ring+1][i];
        Vertex(p0,inner);Vertex(p1,inner);Vertex(p2,outer);
        Vertex(p0,inner);Vertex(p2,outer);Vertex(p3,outer);
    }
}
}
void Draw(int map,float wetness,float night,float lightning)
{
    if(!Hero||wetness<=.12f)return;
    vertices.clear();
    const int cx=static_cast<int>(Hero->Object.Position[0]/CellSize),cy=static_cast<int>(Hero->Object.Position[1]/CellSize);
    for(int y=-RadiusCells;y<=RadiusCells;++y)for(int x=-RadiusCells;x<=RadiusCells;++x)
        if(x*x+y*y<RadiusCells*RadiusCells)Patch(cx+x,cy+y,map,wetness,night,lightning);
    if(vertices.empty())return;
    auto& renderer=mu::GetRenderer();
    renderer.SetTexture2D(false);
    renderer.RenderTriangles(vertices,0);
    renderer.SetTexture2D(true);
}
}
