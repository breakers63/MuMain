#include "stdafx.h"
#include "UI/NewUI/Character/ParisStatsReference41.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "Render/Textures/ZzzTexture.h"
namespace UI::Character
{
    void ParisStatsReference41::Draw(float x, float y, float width, float height,
            int sx, int sy, int sw, int sh) const
{
            if (!Ready()) return;
            EnableAlphaTest();UI::Paris::Color(1,1,1,1);
            RenderBitmap(texture,x,y,width,height,(sx+.5f)/512.f,(sy+.5f)/1024.f,
                (sw-1.f)/512.f,(sh-1.f)/1024.f);
        }
    void ParisStatsReference41::Border(float x,float y,float width,float height,
            int sx,int sy,int sw,int sh,int corner,float targetCorner) const
{
            constexpr int edge=3;
            constexpr float targetEdge=1.5f;
            Draw(x,y,targetCorner,targetCorner,sx,sy,corner,corner);
            Draw(x+width-targetCorner,y,targetCorner,targetCorner,sx+sw-corner,sy,corner,corner);
            Draw(x,y+height-targetCorner,targetCorner,targetCorner,sx,sy+sh-corner,corner,corner);
            Draw(x+width-targetCorner,y+height-targetCorner,targetCorner,targetCorner,sx+sw-corner,sy+sh-corner,corner,corner);
            Draw(x+targetCorner,y,width-2*targetCorner,targetEdge,sx+corner,sy,sw-2*corner,edge);
            Draw(x+targetCorner,y+height-targetEdge,width-2*targetCorner,targetEdge,sx+corner,sy+sh-edge,sw-2*corner,edge);
            Draw(x,y+targetCorner,targetEdge,height-2*targetCorner,sx,sy+corner,edge,sh-2*corner);
            Draw(x+width-targetEdge,y+targetCorner,targetEdge,height-2*targetCorner,sx+sw-edge,sy+corner,edge,sh-2*corner);
        }
    bool ParisStatsReference41::Ready() const
{ return texture!=BITMAP_UNKNOWN; }
    void ParisStatsReference41::Load()
{
            if(!Ready()) texture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_StatsParts41.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
        }
    void ParisStatsReference41::Unload()
{
            if(Ready()) DeleteBitmap(texture);
            texture=BITMAP_UNKNOWN;
        }
    void ParisStatsReference41::Information(int x,int y) const
{
            ParisStyle::Solid(x,y,165,61,.015f,.015f,.015f);
            Border(x,y,165,61,23,89,342,139,12,6);
        }
    void ParisStatsReference41::Attribute(int x,int y) const
{
            ParisStyle::Solid(x,y,147,19,.015f,.015f,.015f);
            Border(x,y,70,19,23,238,146,36,12,6);
            Border(x+70,y,77,19,165,238,158,36,12,6);
        }
    void ParisStatsReference41::Plus(int x,int y) const
{ Draw(x,y,19,19,322,237,41,41); }
    void ParisStatsReference41::Points(int x,int y,int width,int height) const
{
            ParisStyle::Solid(x,y,width,height,.015f,.015f,.015f);
            Border(x,y,width,height,204,103,148,29,6,3);
        }
    void ParisStatsReference41::FooterBackground(int x,int y) const
{ Draw(x,y,164,42,50,690,290,25); }
    void ParisStatsReference41::Footer(int x,int y,int index) const
{
            constexpr int sourceX[]={292,30,117,205}; // X, Quest, Pet, Options.
            Draw(x,y,32,32,sourceX[index],732,74,64);
        }
}
