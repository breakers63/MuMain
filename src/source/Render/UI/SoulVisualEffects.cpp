#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/HUD/SoulVisualEffects51.h"

#include "Render/Textures/ZzzOpenglUtil.h"
#include <algorithm>
#include <cmath>

namespace UI::Soul::Effects
{
    namespace
    {
        constexpr float Tau = 6.283185307f;
        constexpr int CircleSteps = 32, ArcSteps = 20, SoulParticles = 7;
        constexpr float Colors[][3] = {{1.f,.63f,.2f},{.35f,.65f,1.f},{.4f,1.f,.72f},{.8f,.55f,1.f}};

        class AdditiveState
        {
            GLboolean texture, blend, alpha;
            GLint source, destination;
            GLfloat color[4];
        public:
            AdditiveState()
            {
                texture=glIsEnabled(GL_TEXTURE_2D);blend=glIsEnabled(GL_BLEND);alpha=glIsEnabled(GL_ALPHA_TEST);
                glGetIntegerv(GL_BLEND_SRC,&source);glGetIntegerv(GL_BLEND_DST,&destination);glGetFloatv(GL_CURRENT_COLOR,color);
                glDisable(GL_TEXTURE_2D);glDisable(GL_ALPHA_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);
            }
            ~AdditiveState()
            {
                glBlendFunc(source,destination);glColor4fv(color);
                if(texture)glEnable(GL_TEXTURE_2D);else glDisable(GL_TEXTURE_2D);
                if(blend)glEnable(GL_BLEND);else glDisable(GL_BLEND);
                if(alpha)glEnable(GL_ALPHA_TEST);else glDisable(GL_ALPHA_TEST);
            }
        };

        void Vertex(float x,float y){glVertex2f(x*WindowWidth/REFERENCE_WIDTH,WindowHeight-y*WindowHeight/REFERENCE_HEIGHT);}

        void Glow(float x,float y,float rx,float ry,const float* color,float opacity)
        {
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(color[0],color[1],color[2],opacity);Vertex(x,y);
            glColor4f(color[0],color[1],color[2],0);
            for(int step=0;step<=CircleSteps;++step){const float angle=Tau*step/CircleSteps;Vertex(x+std::cos(angle)*rx,y+std::sin(angle)*ry);}
            glEnd();
        }

        void Arc(float x,float y,float rx,float ry,float angle,float length,float thickness,const float* color,float opacity)
        {
            glBegin(GL_TRIANGLE_STRIP);
            for(int step=0;step<=ArcSteps;++step)
            {
                const float fraction=static_cast<float>(step)/ArcSteps,a=angle+length*fraction;
                const float fade=std::sin(fraction*Tau*.5f);
                glColor4f(color[0],color[1],color[2],opacity*fade);
                Vertex(x+std::cos(a)*(rx-thickness),y+std::sin(a)*(ry-thickness));
                Vertex(x+std::cos(a)*(rx+thickness),y+std::sin(a)*(ry+thickness));
            }
            glEnd();
        }

        void Souls(float x,float y,float rx,float ry,float seconds,float strength,const float* color)
        {
            for(int particle=0;particle<SoulParticles;++particle)
            {
                const float age=std::fmod(seconds*.18f+particle/static_cast<float>(SoulParticles),1.f);
                const float a=seconds*.65f+particle*Tau/SoulParticles;
                const float px=x+std::cos(a)*rx*(.35f+.45f*age),py=y+ry*(.65f-1.25f*age);
                const float opacity=std::sin(age*Tau*.5f)*strength;
                Glow(px,py,rx*.105f,ry*.16f,color,opacity*.6f);
                Glow(px,py,rx*.045f,ry*.05f,color,opacity);
            }
        }
    }

    void Orb(float x,float y,float width,float height,float progress,int category,int column,float seconds,bool hovered)
    {
        if(width<=0 || height<=0)return;
        AdditiveState state;
        const float* color=Colors[std::clamp(category,0,3)];
        const float cx=x+width*.5f,cy=y+height*.5f,rx=width*.5f,ry=height*.5f;
        const float time=seconds+column*1.37f,strength=.18f+.48f*std::clamp(progress,0.f,1.f)+(hovered?.12f:0);
        const float pulse=.85f+.15f*std::sin(time*1.7f);
        Glow(cx,cy,rx*.98f,ry*.98f,color,strength*.35f*pulse);
        Arc(cx,cy,rx*.79f,ry*.79f,time*.48f,Tau*.65f,rx*.012f,color,strength);
        Arc(cx,cy,rx*.60f,ry*.38f,-time*.72f,Tau*.72f,rx*.018f,color,strength*.8f);
        Souls(cx,cy,rx,ry,time,strength,color);
        constexpr float Gold[]={1.f,.78f,.33f};
        Arc(cx,cy,rx*.94f,ry*.94f,time*.35f,Tau*.27f,rx*.018f,Gold,strength*pulse);
        Glow(cx+std::cos(time*.35f)*rx*.94f,cy+std::sin(time*.35f)*ry*.94f,rx*.10f,ry*.10f,Gold,strength*.9f);
    }

    void Milestone(float x,float y,float width,float height,float seconds)
    {
        if(width<=0 || height<=0)return;
        AdditiveState state;
        constexpr float Gold[]={1.f,.78f,.33f};
        const float pulse=.18f+.08f*std::sin(seconds*2.f);
        Glow(x+width*.5f,y+height*.5f,width*.65f,height*.65f,Gold,pulse);
        const float phase=std::fmod(seconds*.2f,1.f);
        const float px=x+width*phase,py=y;
        Glow(px,py,width*.08f,height*.12f,Gold,.5f);
    }
}
