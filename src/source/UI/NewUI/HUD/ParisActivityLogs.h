#include "UI/NewUI/HUD/ParisOverlayInput.h"
#pragma once
#include "GameLogic/Social/ActivityHistory.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "UI/NewUI/HUD/ActivityLogView.h"
#include "World/MapInfra/MapManager.h"
#include "UI/NewUI/Dialogs/ParisCustomMenuWindow.h"
#include "ParisOverlayInput.h"

namespace UI::ActivityLogs
{
    enum class Kind { None, Kills, Posts };
    inline Kind Visible = Kind::None;
    inline int Page = 0;
    inline constexpr int X = 125, Y = 55, Width = 390, Height = 335;
    inline constexpr int Rows = 8, RowHeight = 31, RowTop = 50;
    inline UI::Theme::ParisFrameAssets Art;
    inline auto& Current()
    {
        return Visible == Kind::Kills ? GameLogic::Social::ActivityHistory::Kills
            : GameLogic::Social::ActivityHistory::Posts;
    }
    inline std::wstring Time()
    {
        SYSTEMTIME now{}; GetLocalTime(&now);
        wchar_t text[16]; swprintf_s(text, L"%02u:%02u:%02u", now.wHour, now.wMinute, now.wSecond);
        return text;
    }
    inline void AddKill(const wchar_t* victim, int map)
    {
        if (!victim || !*victim) return;
        GameLogic::Social::ActivityHistory::Kills.Add({Time(), victim,
            L"Derrotado", L"Muerte recibida en el area visible.",-1,0,0,0,map});
    }
    inline void AddVictory(const wchar_t* killer,const wchar_t* victim,int map)
    {
        if(!killer||!*killer||!victim||!*victim)return;
        GameLogic::Social::ActivityHistory::Kills.Add({Time(),killer,L"Ganador",std::wstring(L"Derroto a ")+victim,-1,0,0,0,map,true});
    }
    inline void AddPost(const std::wstring& author, const std::wstring& item, const std::wstring& text,
        int type, int level, int excellent, int ancient, int luck = 0, int additionalOptionLevel = 0)
    {
        GameLogic::Social::ActivityHistory::Posts.Add({Time(), author, item, text, type, level, excellent, ancient, -1, false, luck, additionalOptionLevel});
    }
    inline void Load() { Art.Load(); ActivityLog().Load(); }
    inline void Close() { Visible=Kind::None; ActivityLog().SetOpen(false); }
    inline void Unload()
    {
        Art.Unload(); ActivityLog().Unload(); Visible = Kind::None; Page = 0;
        GameLogic::Social::ActivityHistory::Kills.Clear();
        GameLogic::Social::ActivityHistory::Posts.Clear();
    }
    inline void Toggle(Kind kind)
    {
        const bool opening = Visible != kind;

        Visible = opening ? kind : Kind::None; Page = 0;
        ActivityLog().SetOpen(opening,kind==Kind::Kills);
    }
    inline void Render() { if(Visible!=Kind::None)ActivityLog().Render(); }
    inline bool Mouse() { if(Visible==Kind::None||!ActivityLog().CapturesMouse())return false; UI::OverlayInput::Scope mouseOwner; if(ActivityLog().Mouse())Close(); return true; }
}
