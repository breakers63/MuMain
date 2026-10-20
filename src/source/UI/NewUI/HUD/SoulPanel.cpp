#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "SoulPanel.h"
#include "SoulVisualEffects51.h"
#include "SoulMilestoneState52.h"
#include "ParisFeatureSkin49.h"
#include "Network/Paris/SoulProtocol.h"
#include "UI/NewUI/HUD/ParisOverlayInput.h"
#include "UI/NewUI/Character/PanelAspectScale.h"
#include "UI/NewUI/NPCs/ParisNpcSkin56.h"
#include "UI/NewUI/Widgets/ParisTintOverlay.h"
#include "UI/NewUI/NewUISystem.h"
#include <array>
#include <cmath>

namespace UI::Soul
{
    namespace
    {
        using namespace Network::Paris::Soul;
        constexpr int Width = 800, Height = 577;
        constexpr int ColumnX[] = { 42, 225, 408, 591 };
        UI::Theme::ParisFeatureSkin49 skin;
        UI::Theme::ParisFrameAssets frame;
        GLuint atlas = BITMAP_UNKNOWN, coloredButtons = BITMAP_UNKNOWN, soulArt = BITMAP_UNKNOWN;
        bool visible = false, pending = false, confirmReset = false;
        int category = 0, ox = 0, oy = 0;
        float scale = 1;
        DWORD observed = 0, sent = 0;
        std::array<int, AbilityCount> draft{};
        std::array<std::wstring, AbilityCount> captions{};
        std::wstring remaining;

        int W(int value) { return UI::Character::PanelAspect::Axis(value, scale, g_fScreenRate_x); }
        int H(int value) { return UI::Character::PanelAspect::Axis(value, scale, g_fScreenRate_y); }
        int X(int value) { return ox + W(value); }
        int Y(int value) { return oy + H(value); }
        bool Hit(int x, int y, int w, int h) { return SEASON3B::CheckMouseIn(X(x), Y(y), W(w), H(h)); }

        void Layout()
        {
            scale = UI::Character::PanelAspect::Fit(WindowWidth, WindowHeight * .90f, Width, Height);
            ox = static_cast<int>((WindowWidth - Width * scale) / (2 * g_fScreenRate_x));
            oy = static_cast<int>((WindowHeight - Height * scale) / (2 * g_fScreenRate_y));
        }

        void Text(int x, int y, int width, const std::wstring& text, bool gold = true)
        {
            EnableAlphaTest(); UI::Paris::Color(1, 1, 1, 1);
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->SetTextColor(gold ? RGBA(255, 220, 130, 255) : RGBA(235, 240, 250, 255));
            g_pRenderText->RenderText(X(x), Y(y), text.c_str(), W(width), 0, RT3_SORT_CENTER);
        }

        void Button(int x, int y, int width, int height, const std::wstring& caption, bool enabled = true, int color = -1, bool active = false)
        {
            if(color>=0 && coloredButtons!=BITMAP_UNKNOWN)
            {
                EnableAlphaTest();const float brightness=enabled?(Hit(x,y,width,height)?1.15f:1.f):.5f;
                UI::Paris::Color(brightness,brightness,brightness,1);
                RenderBitmap(coloredButtons,X(x),Y(y),W(width),H(height),0.f,color*64.f/256.f,200.f/256.f,56.f/256.f);
                UI::Paris::Color(1,1,1,1);
                if(active) UI::Theme::TintOverlay(X(x+width/8),Y(y+height-3),W(width*3/4),H(1),.92f,.72f,.25f,1.f);
            }
            else skin.Base(X(x), Y(y), W(width), H(height), enabled, enabled && Hit(x, y, width, height));
            skin.Caption(X(x), Y(y), W(width), H(height), caption.c_str(), enabled);
        }

        long long DraftCost()
        {
            long long result = 0;
            for (int index = 0; index < AbilityCount; ++index)
            {
                const auto paid = Data.Paid[index];
                result += draft[index] >= Data.Ranks[index] ? paid + static_cast<long long>(draft[index] - Data.Ranks[index]) * Data.Abilities[index].Cost
                    : draft[index] == 0 ? 0 : paid - paid * (Data.Ranks[index] - draft[index]) / Data.Ranks[index];
            }
            return result;
        }

        void UpdateCaptions()
        {
            remaining = Label("Remaining") + L": " + std::to_wstring(std::max(0LL, Data.Total - DraftCost()));
            for (int index = 0; index < AbilityCount; ++index)
            {
                const auto& rule = Data.Abilities[index];
                const double value = rule.Enabled ? rule.MaximumBenefit * std::min(draft[index], rule.MaximumRank) / rule.MaximumRank : 0;
                wchar_t number[48]{};
                swprintf_s(number, L"+%.2f%s", value, index == 3 || index == 11 ? L"" : L"%");
                captions[index] = number;
            }
        }

        void Synchronize()
        {
            if (observed == Received) return;
            observed = Received; pending = confirmReset = false;
            draft = Data.Ranks;
            UpdateCaptions();
        }

        void Sprite(int x, int y, int width, int height, int sourceX, int sourceY, int sourceWidth, int sourceHeight, float brightness = 1)
        {
            if (atlas == BITMAP_UNKNOWN) return;
            EnableAlphaTest(); UI::Paris::Color(brightness, brightness, brightness, 1);
            RenderBitmap(atlas, X(x), Y(y), W(width), H(height), sourceX / 1024.f, sourceY / 1024.f, sourceWidth / 1024.f, sourceHeight / 1024.f);
            UI::Paris::Color(1, 1, 1, 1);
        }

        void Art(int x,int y,int width,int height,int column,int row,float brightness=1.f)
        {
            if(soulArt==BITMAP_UNKNOWN)return;
            EnableAlphaTest();UI::Paris::Color(brightness,brightness,brightness,1);
            // The generated illustrations have different padding. Sample their
            // actual artwork instead of a whole padded atlas cell.
            constexpr int bounds[3][4][4]={
                {{45,289,193,191},{297,290,191,190},{540,290,193,190},{787,289,193,191}},
                {{45,524,192,190},{296,524,192,190},{540,524,193,190},{787,523,194,191}},
                {{44,755,192,191},{296,755,192,191},{540,755,193,191},{787,754,194,192}}
            };
            if(row>0)
            {
                const auto& rect=bounds[row-1][column];
                RenderBitmap(soulArt,X(x),Y(y),W(width),H(height),(rect[0]+.5f)/1024,(rect[1]+.5f)/1024,(rect[2]-1.f)/1024,(rect[3]-1.f)/1024);
            }
            else RenderBitmap(soulArt,X(x),Y(y),W(width),H(height),column*.25f,0,.25f,.25f);
            UI::Paris::Color(1,1,1,1);
        }

        int CaptionWidth(const std::wstring& caption)
        {
            g_pRenderText->SetFont(g_hFontBold);
            const SIZE size = g_pRenderText->MeasureText(caption.c_str(), static_cast<int>(caption.size()));
            return static_cast<int>(size.cx/std::max(.01f,g_fScreenRate_x));
        }

        void CategoryTab(int tab,const std::wstring& caption)
        {
            constexpr int Left=36,Step=121,Width=116,Top=55,Height=30,IconSize=18,Gap=6;
            constexpr int icons[4][4]={{64,60,182,192},{312,63,154,191},{544,63,181,182},{775,64,201,195}};
            const int x=Left+tab*Step;const auto& icon=icons[tab];
            Button(x,Top,Width,Height,L"",true,tab==0?0:tab==2?2:1,category==tab);
            const float longest=static_cast<float>(std::max(icon[2],icon[3]));
            const int iw=W(static_cast<int>(IconSize*icon[2]/longest)),ih=H(static_cast<int>(IconSize*icon[3]/longest));
            const int textWidth=std::min(CaptionWidth(caption),W(Width-IconSize-Gap-10));
            const int left=X(x)+(W(Width)-iw-W(Gap)-textWidth)/2;
            if(soulArt!=BITMAP_UNKNOWN)
            {
                EnableAlphaTest();UI::Paris::Color(1,1,1,1);
                RenderBitmap(soulArt,left,Y(Top)+(H(Height)-ih)/2,iw,ih,(icon[0]+.5f)/1024,(icon[1]+.5f)/1024,(icon[2]-1.f)/1024,(icon[3]-1.f)/1024);
            }
            skin.Caption(left+iw+W(Gap),Y(Top),textWidth,H(Height),caption.c_str(),true);
        }

        constexpr int RankControlX[]={9,43,103,133};
        constexpr int RankControlWidth[]={30,26,26,30};
        constexpr int RankControlDelta[]={-10,-1,1,10};
        constexpr int RankControlY=231,RankControlHeight=25;

        void AllocationControls(int column,int index)
        {
            const int x=ColumnX[column];const auto& rule=Data.Abilities[index];
            const wchar_t* labels[]={L"-10",L"-1",L"+1",L"+10"};
            for(int control=0;control<4;++control)
            {
                const bool adding=RankControlDelta[control]>0;
                const bool enabled=Data.Enabled && !pending && (adding?rule.Enabled && draft[index]<rule.MaximumRank:
                    Data.AllowReallocation && draft[index]>0);
                Button(x+RankControlX[control],RankControlY,RankControlWidth[control],RankControlHeight,labels[control],enabled,adding?2:0);
            }
            skin.Fill(X(x+72),Y(RankControlY),W(28),H(RankControlHeight),.018f,.025f,.023f);
            skin.Outline(X(x+72),Y(RankControlY),W(28),H(RankControlHeight));
            skin.Caption(X(x+72),Y(RankControlY),W(28),H(RankControlHeight),std::to_wstring(draft[index]).c_str(),true);
        }

        void Lock(int x,int y)
        {
            constexpr int Width=12,Height=9;
            skin.Fill(X(x-Width/2),Y(y),W(Width),H(Height),.12f,.14f,.16f);
            skin.Outline(X(x-Width/2),Y(y),W(Width),H(Height));
            for(int step=0;step<12;++step)
            {
                const float angle=3.14159265f+step*3.14159265f/11;
                UI::Theme::TintOverlay(X(x+static_cast<int>(std::cos(angle)*4)),Y(y+static_cast<int>(std::sin(angle)*5)),W(1),H(2),.8f,.83f,.86f,.95f);
            }
            skin.Fill(X(x-1),Y(y+3),W(2),H(4),.02f,.02f,.02f);
        }

        void Milestone(int column,int index,int tier)
        {
            const int x=ColumnX[column];const auto& rule=Data.Abilities[index];
            const int y=277+static_cast<int>(215.f*rule.Milestones[tier]/rule.MaximumRank);
            const bool unlocked=IsMilestoneUnlocked(Data.Enabled,rule.Enabled,Data.Ranks[index],rule.Milestones[tier]);
            Text(x+20,y-6,42,std::to_wstring(rule.Milestones[tier]),unlocked);
            Art(x+110,y-20,38,38,column,tier+1,unlocked?1.f:.45f);
            skin.Outline(X(x+108),Y(y-22),W(42),H(42),unlocked);
            if(unlocked)Effects::Milestone(X(x+108),Y(y-22),W(42),H(42),(timeGetTime()%600000)/1000.f);
            else Lock(x+129,y-3);
            if(Hit(x+105,y-23,48,48))
            {
                wchar_t benefit[64]{};swprintf_s(benefit,L"+%.2f",rule.Bonuses[tier]);
                Text(x-6,506,180,Label("Milestone")+L" "+benefit,unlocked);
            }
        }

        void Orb(int column, int index)
        {
            const int x = ColumnX[column];
            const auto& rule = Data.Abilities[index];
            const float progress = std::clamp(static_cast<float>(draft[index]) / rule.MaximumRank, 0.f, 1.f);
            const int filled = static_cast<int>(68 * progress);
            const int sourceX = 92 + column * 183;
            Sprite(x + 49, 113, 68, 68, sourceX, 115, 68, 68, .78f);
            if (filled > 0) Sprite(x + 49, 181 - filled, 68, filled, sourceX, 183 - filled, 68, filled);
            
            Effects::Orb(X(x+49),Y(113),W(68),H(68),progress,category,column,
                (timeGetTime()%600000)/1000.f,Hit(x+49,113,68,68));
            Text(x + 3, 191, 160, rule.Name);
            Text(x + 3, 210, 160, captions[index], false);
        }

        void Column(int column)
        {
            const int x = ColumnX[column], index = category * 4 + column;
            const auto& rule = Data.Abilities[index];
            Sprite(x,94,170,421,x,94,170,421,.95f);
            // Cover only the static reference captions; retain its diamond,
            // textured columns, meter rail and milestone artwork.
            skin.Fill(X(x+3),Y(188),W(164),H(74),.025f,.042f,.035f);
            skin.Fill(X(x+14),Y(277),W(51),H(221),.025f,.042f,.035f);
            // Erase baked silver frames and locks before drawing native state.
            skin.Fill(X(x+103),Y(277),W(65),H(237),.015f,.023f,.018f);
            if(column==0)Sprite(x+148,110,20,37,365,110,20,37,.95f);
            Orb(column, index);
            AllocationControls(column,index);
            skin.Fill(X(x+80),Y(277),W(9),H(215),.03f,.04f,.04f);
            skin.Outline(X(x+78),Y(275),W(13),H(219));
            const int height = static_cast<int>(215.f * std::clamp(static_cast<float>(draft[index]) / rule.MaximumRank, 0.f, 1.f));
            skin.Fill(X(x+82),Y(277),W(5),H(height),.95f,.72f,.23f);
            for(int tier=0;tier<3;++tier)Milestone(column,index,tier);
        }

        bool Send(BYTE action)
        {
            if (!Request(action, static_cast<BYTE>(category), draft)) return false;
            sent = timeGetTime(); pending = action != 0;
            return true;
        }

        void ChangeRank(int index, int delta)
        {
            if (pending || !Data.Enabled || delta > 0 && (!Data.Abilities[index].Enabled || draft[index] >= Data.Abilities[index].MaximumRank)
                || delta < 0 && !Data.AllowReallocation) return;
            const int old = draft[index];
            draft[index] = std::clamp(old + delta, 0, std::max(Data.Ranks[index], Data.Abilities[index].MaximumRank));
            if (delta > 0 && DraftCost() > Data.Total) draft[index] = old;
            UpdateCaptions();
        }
    }

    void Load()
    {
        skin.Load(); frame.Load();
        if(soulArt==BITMAP_UNKNOWN)soulArt=Bitmaps.LoadImage(L"Data\\Interface\\Paris_SoulArt52.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
        if(coloredButtons==BITMAP_UNKNOWN)coloredButtons=Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankingColorsV14.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);
        if (atlas == BITMAP_UNKNOWN) atlas = Bitmaps.LoadImage(L"Data\\Interface\\Paris_SoulAtlas48.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void Close() { visible = pending = confirmReset = false; UI::OverlayInput::Capture(); }
    void Unload()
    {
        Close(); skin.Unload(); frame.Unload();
        if(soulArt!=BITMAP_UNKNOWN)DeleteBitmap(soulArt);soulArt=BITMAP_UNKNOWN;
        if(coloredButtons!=BITMAP_UNKNOWN)DeleteBitmap(coloredButtons);coloredButtons=BITMAP_UNKNOWN;
        if (atlas != BITMAP_UNKNOWN) DeleteBitmap(atlas);
        atlas = BITMAP_UNKNOWN; observed = 0; Network::Paris::Soul::Clear();
    }

    bool IsOpen() { return visible; }
    void Toggle()
    {
        if (visible) { Close(); return; }
        visible = true; pending = confirmReset = false; observed = 0; Layout(); Synchronize(); Send(0);
        UI::OverlayInput::Capture();
    }

    void Render()
    {
        if (!visible) return;
        Layout(); Synchronize();
        if (pending && timeGetTime() - sent > 5000) { pending = false; Send(0); }
        skin.Frame(X(0),Y(0),W(Width),H(Height));
        Text(190, 25, 420, Label("Title"));
        skin.Close(X(748), Y(20), W(28), H(28));
        const char* categories[] = { "Attack", "Defense", "Support", "Misc" };
        for(int tab=0;tab<4;++tab)CategoryTab(tab,Label(categories[tab]));
        Text(535, 64, 240, remaining, false);
        if (!Received) { Text(80, 280, 640, Label("Loading")); return; }
        for (int column = 0; column < 4; ++column) Column(column);
        Button(44,529,140,28, Label("Reset"), Data.Enabled && Data.AllowReset && Data.Spent > 0 && !pending,0);
        Button(415,529,108,28, Label("Cancel"), !pending,1);
        Button(533,529,108,28, Label("OK"), Data.Enabled && !pending && draft != Data.Ranks,2);
        Button(651,529,108,28, Label("Close"),true,0);
        if (confirmReset)
        {
            frame.Stone(X(170), Y(260), W(460), H(120), false, true); frame.Frame(X(170), Y(260), W(460), H(120), true);
            Text(190, 283, 420, Label("ConfirmReset") + L" " + std::to_wstring(Data.ResetFee));
            Button(210, 326, 165, 30, Label("OK"),true,2); Button(425, 326, 165, 30, Label("Cancel"),true,1);
        }
        else if (!Data.Enabled) Text(70, 519, 660, Label("Disabled"));
        else if (pending) Text(70, 519, 660, Label("Pending"));
        else if (Data.Decision > 0) { const auto key = "Error" + std::to_string(Data.Decision); Text(70, 519, 660, Label(key.c_str()), false); }
    }

    bool Mouse()
    {
        if (!visible) return false;
        UI::OverlayInput::Scope owner; Layout();
        if (!SEASON3B::IsPress(VK_LBUTTON)) return true;
        if (Hit(748, 20, 28, 28) || Hit(651,529,108,28)) { Close(); return true; }
        if (confirmReset)
        {
            if (Hit(210, 326, 165, 30)) { confirmReset = false; Send(2); }
            else if (Hit(425, 326, 165, 30)) confirmReset = false;
            return true;
        }
        for (int tab = 0; tab < 4; ++tab) if (Hit(36 + tab * 121,55,116,30)) { category = tab; return true; }
        if (!Received || pending) return true;
        if (Hit(44,529,140,28) && Data.Enabled && Data.AllowReset && Data.Spent > 0) confirmReset = true;
        else if (Hit(415,529,108,28)) { draft = Data.Ranks; UpdateCaptions(); }
        else if (Hit(533,529,108,28) && Data.Enabled && draft != Data.Ranks) Send(1);
        else for (int column = 0; column < 4; ++column)
        {
            const int x = ColumnX[column], index = category * 4 + column;
            for(int control=0;control<4;++control)
                if(Hit(x+RankControlX[control],RankControlY,RankControlWidth[control],RankControlHeight))
                {ChangeRank(index,RankControlDelta[control]);break;}
        }
        return true;
    }
}
