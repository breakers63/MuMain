#include "stdafx.h"
#include "GoldOptionPanel.h"
#include "HdOptionControls.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "Data/GameConfig/GameConfig.h"
#include "Render/Renderer/MuRenderer.h"
#ifdef _WIN32
extern void MuSetVSyncPreference(bool enabled);
#endif

namespace UI::Options::Gold
{
namespace
{
Theme::ParisFrameAssets art;
bool shaderTab = false;
constexpr int RightX = 206, RightWidth = 204, RowTop = 69, RowStep = 37, CheckboxX = 390;
constexpr int SyncY = 272, ResetY = 340;
constexpr const wchar_t* Names[]{L"Reducir particulas y destellos", L"Reducir efectos del ambiente",
    L"Ocultar aves y peces", L"Reducir sombras", L"Reducir brillo del equipo"};
void Label(int x, int y, const wchar_t* text, int width = RightWidth, bool gold = false)
{
    g_pRenderText->SetFont(gold ? g_hFontBold : g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, gold ? 210 : 255, gold ? 90 : 255, 255);
    g_pRenderText->RenderText(x, y, text, width);
}
}
void Load() { art.Load(); }
void Unload() { art.Unload(); }
void Frame(int x, int y)
{
    art.Frame(x, y, Width, Height);
    Character::ParisStyle::Label(x + 18, y + 12, Width - 54, L"OPCIONES - MU PARIS");
    Character::ParisStyle::Solid(x + 190, y + 39, 1, Height - 56, .45f, .35f, .17f);
    Theme::Bevel(x + CloseX, y + CloseY, CloseSize, CloseSize, L"X");
}
void VisualSettings(int x, int y)
{
    auto& settings = GameConfig::GetInstance();
    Theme::Bevel(x + RightX, y + 39, 99, 20, L"Rendimiento", !shaderTab);
    Theme::Bevel(x + RightX + 105, y + 39, 99, 20, L"Shaders HD", shaderTab);
    if (shaderTab) { Hd::Render(x, y); return; }
    for (std::size_t index = 0; index < Data::Graphics::VisualReductionCount; ++index)
    {
        const int top = y + RowTop + static_cast<int>(index) * RowStep;
        Label(x + RightX, top + 3, Names[index], CheckboxX - RightX - 6);
        Theme::Check(x + CheckboxX, top, settings.IsVisualReductionEnabled(
            static_cast<Data::Graphics::VisualReduction>(index)));
    }
    Label(x + RightX, y + SyncY + 3, L"Sincronizacion vertical", CheckboxX - RightX - 6);
    Theme::Check(x + CheckboxX, y + SyncY, settings.GetVSyncEnabled());
    Label(x + RightX, y + 309, L"Los cambios se aplican al instante.");
    Theme::Bevel(x + RightX, y + ResetY, RightWidth, 24, L"Restaurar efectos");
}
bool HandleVisualSettings(int x, int y)
{
    if (SEASON3B::IsPress(VK_LBUTTON) && SEASON3B::CheckMouseIn(x + RightX, y + 39, RightWidth, 20))
    {
        shaderTab = MouseX >= x + RightX + 105;
        return true;
    }
    if (shaderTab) return Hd::Mouse(x, y);
    if (!SEASON3B::IsPress(VK_LBUTTON)) return false;
    auto& settings = GameConfig::GetInstance();
    for (std::size_t index = 0; index < Data::Graphics::VisualReductionCount; ++index)
    {
        if (!SEASON3B::CheckMouseIn(x + RightX, y + RowTop + static_cast<int>(index) * RowStep, RightWidth, 23)) continue;
        const auto option = static_cast<Data::Graphics::VisualReduction>(index);
        settings.SetVisualReductionEnabled(option, !settings.IsVisualReductionEnabled(option));
        return true;
    }
    if (SEASON3B::CheckMouseIn(x + RightX, y + SyncY, RightWidth, 23))
    {
#ifdef _WIN32
        MuSetVSyncPreference(!settings.GetVSyncEnabled());
#else
        settings.SetVSyncEnabled(!settings.GetVSyncEnabled());
        (void)mu::GetRenderer().SetVSyncEnabled(settings.GetVSyncEnabled());
#endif
        settings.Save();
        return true;
    }
    if (!SEASON3B::CheckMouseIn(x + RightX, y + ResetY, RightWidth, 24)) return false;
    for (std::size_t index = 0; index < Data::Graphics::VisualReductionCount; ++index)
        settings.SetVisualReductionEnabled(static_cast<Data::Graphics::VisualReduction>(index), false);
    return true;
}
void Volume(int x, int y, int level)
{
    constexpr int Segments = 10;
    constexpr float Step = 12.4f;
    for (int segment = 0; segment < Segments; ++segment)
        Character::ParisStyle::Solid(x + segment * Step, y, 11, 14,
            segment < level ? .84f : .16f, segment < level ? .63f : .15f, .13f);
}
void EffectLevels(int x, int y, int level)
{
    constexpr const wchar_t* Labels[]{L"5", L"7", L"9", L"11", L"13"};
    for (int index = 0; index < 5; ++index)
        Theme::Bevel(x + index * 28, y, 27, 20, Labels[index], index == level);
}
}
