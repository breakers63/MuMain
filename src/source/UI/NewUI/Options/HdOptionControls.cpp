#include "stdafx.h"
#include <algorithm>
#include "HdOptionControls.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "Data/GameConfig/GameConfig.h"
#include "Render/Renderer/SdlGpuHdPostProcessor.h"

namespace UI::Options::Hd
{
namespace
{
constexpr int Left = 206, Width = 204, CheckX = 390;
constexpr int ProfileTop = 72, CheckTop = 112, RowStep = 29, SliderTop = 242, SliderStep = 45;
constexpr const wchar_t* CheckLabels[]{L"Activar shaders HD", L"Color y contraste", L"Nitidez", L"Bloom / resplandor"};
constexpr const wchar_t* SliderLabels[]{L"Intensidad de nitidez", L"Intensidad de bloom", L"Exposicion"};
void Text(int x, int y, const wchar_t* text)
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 235, 170, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText(x, y, text, Width);
}
}
void Render(int x, int y)
{
    const auto& settings = GameConfig::GetInstance().GetHdSettings();
    constexpr const wchar_t* Profiles[]{L"Clasico", L"Moderno", L"HD"};
    for (int profile = 0; profile < 3; ++profile)
        Theme::Bevel(x + Left + profile * 69, y + ProfileTop, 65, 22, Profiles[profile], settings.Profile == profile);
    const bool checked[]{settings.Enabled, settings.ColorGrading, settings.Sharpen, settings.Bloom};
    for (int row = 0; row < 4; ++row)
    {
        Text(x + Left, y + CheckTop + row * RowStep + 3, CheckLabels[row]);
        Theme::Check(x + CheckX, y + CheckTop + row * RowStep, checked[row]);
    }
    const int values[]{settings.Sharpness, settings.BloomStrength, (settings.Exposure - 75) * 2};
    for (int row = 0; row < 3; ++row)
    {
        const int top = y + SliderTop + row * SliderStep;
        wchar_t label[96]{};
        swprintf_s(label, L"%ls: %d%%", SliderLabels[row], row == 2 ? settings.Exposure : values[row]);
        Text(x + Left, top - 13, label);
        Character::ParisStyle::Solid(x + Left, top + 4, Width, 4, .16f, .14f, .10f);
        Character::ParisStyle::Solid(x + Left, top + 4, Width * values[row] / 100.f, 4, .84f, .63f, .13f);
        Theme::Bevel(x + Left + (Width - 5) * values[row] / 100, top, 5, 13, L"");
    }
    Text(x + Left, y + 383, Render::Hd::Available() ? L"SDL3: shaders HD disponibles" : L"Shaders HD no disponibles");
}
bool Mouse(int x, int y)
{
    auto& config = GameConfig::GetInstance();
    auto settings = config.GetHdSettings();
    if (SEASON3B::IsPress(VK_LBUTTON))
    {
        for (int profile = 0; profile < 3; ++profile)
            if (SEASON3B::CheckMouseIn(x + Left + profile * 69, y + ProfileTop, 65, 22))
            {
                config.SetHdSettings(Data::Graphics::HdProfile(profile));
                return true;
            }
        bool* targets[]{&settings.Enabled, &settings.ColorGrading, &settings.Sharpen, &settings.Bloom};
        for (int row = 0; row < 4; ++row)
            if (SEASON3B::CheckMouseIn(x + Left, y + CheckTop + row * RowStep, Width, 23))
            {
                *targets[row] = !*targets[row];
                config.SetHdSettings(settings);
                return true;
            }
    }
    if (!MouseLButtonPop) return false;
    for (int row = 0; row < 3; ++row)
    {
        if (!SEASON3B::CheckMouseIn(x + Left, y + SliderTop + row * SliderStep, Width, 15)) continue;
        const int value = std::clamp((MouseX - x - Left) * 100 / Width, 0, 100);
        if (row == 0) settings.Sharpness = value;
        else if (row == 1) settings.BloomStrength = value;
        else settings.Exposure = 75 + value / 2;
        config.SetHdSettings(settings);
        return true;
    }
    return false;
}
}
