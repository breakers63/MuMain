#pragma once
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"

namespace UI::Theme
{
    // Original reference fragments. Text and game objects are rendered separately.
    class ParisMarketAssets
    {
        GLuint stats = BITMAP_UNKNOWN, market = BITMAP_UNKNOWN, scroll = BITMAP_UNKNOWN;
        static void Draw(GLuint texture, int tw, int th, int x, int y, int w, int h,
            int sx, int sy, int sw, int sh)
        {
            if (texture == BITMAP_UNKNOWN || w <= 0 || h <= 0) return;
            EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
            RenderBitmap(texture,x,y,w,h,(sx+.5f)/tw,(sy+.5f)/th,(sw-1.f)/tw,(sh-1.f)/th);
        }
        static void Border(GLuint texture,int tw,int th,int x,int y,int w,int h,
            int sx,int sy,int sw,int sh,int corner,int edge)
        {
            edge=std::min(edge,std::min(w,h)/3);
            const int xs[]={sx,sx+corner,sx+sw-corner,sx+sw};
            const int ys[]={sy,sy+corner,sy+sh-corner,sy+sh};
            const int dx[]={x,x+edge,x+w-edge,x+w},dy[]={y,y+edge,y+h-edge,y+h};
            for(int row=0;row<3;++row) for(int col=0;col<3;++col)
            {
                if(row==1 && col==1) continue;
                const bool horizontal=row!=1 && col==1;
                const bool vertical=row==1 && col!=1;
                const int sourceEdge=std::min(corner,8), targetEdge=corner>20?3:edge;
                if(horizontal)
                    Draw(texture,tw,th,dx[col],row==0?y:y+h-targetEdge,dx[col+1]-dx[col],targetEdge,
                        xs[col],row==0?sy:sy+sh-sourceEdge,xs[col+1]-xs[col],sourceEdge);
                else if(vertical)
                    Draw(texture,tw,th,col==0?x:x+w-targetEdge,dy[row],targetEdge,dy[row+1]-dy[row],
                        col==0?sx:sx+sw-sourceEdge,ys[row],sourceEdge,ys[row+1]-ys[row]);
                else Draw(texture,tw,th,dx[col],dy[row],dx[col+1]-dx[col],dy[row+1]-dy[row],
                    xs[col],ys[row],xs[col+1]-xs[col],ys[row+1]-ys[row]);
            }
        }
        static void Tile(GLuint texture,int tw,int th,int x,int y,int w,int h,
            int sx,int sy,int sourceSize,int tileSize)
        {
            for(int top=0;top<h;top+=tileSize) for(int left=0;left<w;left+=tileSize)
            {
                const int width=std::min(tileSize,w-left),height=std::min(tileSize,h-top);
                Draw(texture,tw,th,x+left,y+top,width,height,sx,sy,
                    sourceSize*width/tileSize,sourceSize*height/tileSize);
            }
        }
    public:
        void Load()
        {
            if(stats==BITMAP_UNKNOWN) stats=Bitmaps.LoadImage(L"Data\\Interface\\Paris_ExactStats.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
            if(market==BITMAP_UNKNOWN) market=Bitmaps.LoadImage(L"Data\\Interface\\Paris_ExactMarket.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
            if(scroll==BITMAP_UNKNOWN) scroll=Bitmaps.LoadImage(L"Data\\Interface\\Paris_ExactScroll.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
        }
        void Unload()
        {
            for(auto texture : {stats,market,scroll}) if(texture!=BITMAP_UNKNOWN) DeleteBitmap(texture);
            stats=market=scroll=BITMAP_UNKNOWN;
        }
        void StatsFrame(int x,int y,int w,int h) const
        {
            Tile(stats,512,1024,x,y,w,h,120,840,75,30);
            Border(stats,512,1024,x,y,w,h,0,0,482,1017,30,12);
            // Clean vertical strips avoid the cursor captured near the reference's left edge.
            Draw(stats,512,1024,x,y+21,8,h-42,0,250,20,300);
            Draw(stats,512,1024,x+w-8,y+21,8,h-42,462,250,20,300);
        }
        void StatBox(int x,int y,int w,int h) const
        {
            Character::ParisStyle::Solid(x,y,w,h,.025f,.025f,.025f);
            Border(stats,512,1024,x,y,w,h,51,320,151,51,13,5);
        }
        void ExperienceBox(int x,int y,int w,int h) const
        {
            Character::ParisStyle::Solid(x,y,w,h,.025f,.025f,.025f);
            Border(stats,512,1024,x,y,w,h,45,123,400,105,18,7);
        }
        void Divider(int x,int y,int w) const {Draw(stats,512,1024,x,y,w,6,98,296,254,18);}
        void Plus(int x,int y,int w,int h) const {Draw(stats,512,1024,x,y,w,h,401,320,51,52);}
        void Footer(int x,int y,int size,int index) const
        {
            constexpr int sourceX[]={62,164,266,367};
            Draw(stats,512,1024,x,y,size,size,sourceX[index],924,67,66);
        }
        void Experience(int x,int y,int w,int h,double ratio) const
        {
            // Empty right-hand section, then the actual progress portion from the source.
            Draw(stats,512,1024,x,y,w,h,247,174,169,14);
            const int fill=static_cast<int>(std::clamp(ratio,0.0,1.0)*(w-6));
            Draw(stats,512,1024,x+3,y+2,fill,h-4,80,179,130,5);
            Border(stats,512,1024,x,y,w,h,59,169,373,24,10,4);
        }
        void MarketFrame(int x,int y,int w,int h) const
        {
            Tile(market,2048,2048,x,y,w,h,64,846,160,60);
            Border(market,2048,2048,x,y,w,h,0,0,1516,1038,72,28);
        }
        void InnerFrame(int x,int y,int w,int h) const
        {
            Character::ParisStyle::Solid(x,y,w,h,.02f,.02f,.02f);
            Border(market,2048,2048,x,y,w,h,39,326,1093,513,40,20);
        }
        void PreviewFrame(int x,int y,int w,int h) const
        {
            Draw(market,2048,2048,x,y,w,h,1364,200,85,550);
            Border(market,2048,2048,x,y,w,h,1140,89,343,750,36,13);
        }
        void Button(int x,int y,int w,int h) const
        {
            Character::ParisStyle::Solid(x,y,w,h,.025f,.025f,.025f);
            Border(market,2048,2048,x,y,w,h,856,201,259,50,12,4);
        }
        void MarketHeader(int x,int y,int w,int h) const
        {
            Character::ParisStyle::Solid(x,y,w,h,.025f,.025f,.025f);
            Border(market,2048,2048,x,y,w,h,56,382,1057,47,14,4);
        }
        void MarketTitleDecoration(int x,int y,int w) const
        {
            Draw(market,2048,2048,x+w/2-116,y+10,40,13,480,27,105,34);
            Draw(market,2048,2048,x+w/2+76,y+10,40,13,932,27,105,34);
        }
        void Coin(int x,int y,int w,int h) const {Draw(market,2048,2048,x,y,w,h,814,453,37,38);}
        void SearchIcon(int x,int y,int size) const {Draw(market,2048,2048,x,y,size,size,901,209,36,33);}
        void RefreshIcon(int x,int y,int size) const {Draw(market,2048,2048,x,y,size,size,902,272,38,36);}
        void Arrow(int x,int y,int size) const {Draw(market,2048,2048,x,y,size,size/2,348,134,26,18);}
        void Scroll(int x,int y,int w,int h,int thumbY,int thumbHeight) const
        {
            constexpr int arrowSize=12, thumbWidth=17;
            const int center=x+w/2;
            // Repeat only a section without the reference thumb.
            Draw(scroll,128,2048,x,y,w,h,14,72,42,230);
            Draw(scroll,128,2048,x,y,w,4,14,48,42,12);
            Draw(scroll,128,2048,x,y+h-4,w,4,14,1373,42,13);
            Draw(scroll,128,2048,center-arrowSize/2,y-arrowSize-1,arrowSize,arrowSize,12,0,46,48);
            Draw(scroll,128,2048,center-arrowSize/2,y+h+1,arrowSize,arrowSize,12,1388,46,49);
            Draw(scroll,128,2048,center-thumbWidth/2,thumbY,thumbWidth,thumbHeight,1,244,67,120);
        }
        static int ScrollArrow(int x,int y,int w,int h)
        {
            constexpr int size=12;
            if(SEASON3B::CheckMouseIn(x+w/2-size/2,y-size-1,size,size)) return -1;
            if(SEASON3B::CheckMouseIn(x+w/2-size/2,y+h+1,size,size)) return 1;
            return 0;
        }
    };
}
