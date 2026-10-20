#include "stdafx.h"
#include "ParisHudStatus.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Scaling/UITransform.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Dotnet/ConnectionLatency.h"
#include "MUHelper/MuHelper.h"
namespace UI::Hud::Status
{
    namespace
    {
        constexpr int HelperX = 78, SettingsY = 433, StartY = 454, HelperSize = 17;
        int ping = -1;
        DWORD sample = 0;
        void Label(int x, int y, int width, const wchar_t* text)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetBgColor(0);
            g_pRenderText->SetTextColor(255, 227, 150, 255);
            g_pRenderText->RenderText(x, y, text, width, 0, RT3_SORT_CENTER);
        }
        void Indicators()
        {
            const DWORD now = GetTickCount();
            constexpr DWORD SampleInterval = 1000;
            if (now - sample >= SampleInterval) { sample = now; ping = DotNetBridge::TcpPingMilliseconds(SocketClient); }
            const bool known[]{FPS_AVG > 0, ping >= 0};
            const bool good[]{FPS_AVG >= 50, ping >= 0 && ping < 100};
            const bool fair[]{FPS_AVG >= 25, ping >= 0 && ping <= 250};
            constexpr int left[]{436, 451};
            for (int i = 0; i < 2; ++i)
            {
                const float red = !known[i] ? .4f : good[i] ? .2f : 1.f;
                const float green = !known[i] ? .4f : good[i] ? 1.f : fair[i] ? .55f : .15f;
                Character::ParisStyle::Solid(left[i], 445, 6, 25, red, green, .15f);
            }
            wchar_t tip[64]{};
            swprintf_s(tip, L"FPS: %.0f", FPS_AVG);
            Theme::HoverTip(434, 443, 10, 29, tip);
            if (ping < 0) wcscpy_s(tip, L"Ping: --"); else swprintf_s(tip, L"Ping: %d ms", ping);
            Theme::HoverTip(449, 443, 10, 29, tip);
        }
    }
    bool MenuHit()
    {
        Scaling::ScopedActiveTransform hud(Scaling::BottomHudCenterTransform(WindowWidth, WindowHeight), true);
        return SEASON3B::CheckMouseIn(MenuX, MenuY, MenuWidth, MenuHeight);
    }
    void RenderMenu()
    {
        Scaling::ScopedActiveTransform hud(Scaling::BottomHudCenterTransform(WindowWidth, WindowHeight));
        Label(MenuX, MenuY, MenuWidth, L"MENU");
    }
    void Render()
    {
        wchar_t coordinates[32]{};
        swprintf_s(coordinates, L"%d  %d", Hero->PositionX, Hero->PositionY);
        Label(9, 460, 58, coordinates);
        Theme::Bevel(HelperX, SettingsY, HelperSize, HelperSize, L"H");
        Theme::Bevel(HelperX, StartY, HelperSize, HelperSize, MUHelper::g_MuHelper.IsActive() ? L"[]" : L">");
        Theme::HoverTip(HelperX, SettingsY, HelperSize, HelperSize, L"Configurar MU Helper");
        Theme::HoverTip(HelperX, StartY, HelperSize, HelperSize, L"Iniciar / detener MU Helper");
        Indicators();
    }
    bool Mouse()
    {
        const bool settings = SEASON3B::CheckMouseIn(HelperX, SettingsY, HelperSize, HelperSize);
        const bool start = SEASON3B::CheckMouseIn(HelperX, StartY, HelperSize, HelperSize);
        if (!settings && !start) return false;
        if (SEASON3B::IsPress(VK_LBUTTON))
        {
            if (settings) g_pNewUISystem->Toggle(SEASON3B::INTERFACE_MUHELPER);
            else MUHelper::g_MuHelper.Toggle();
        }
        Paris::ConsumeMouse();
        return true;
    }
}
