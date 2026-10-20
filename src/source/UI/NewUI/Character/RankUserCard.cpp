#include "stdafx.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "RankUserCard.h"
#include "ClassPortraits.h"
#include "NewUICustomRanking.h"
#include "ParisFlatStyle.h"
#include "ParisPanelStyle.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzTexture.h"
#include <algorithm>
#include <cstdint>
namespace UI::Character::RankCard
{
namespace
{
constexpr int ReferenceWidth = 500, ReferenceHeight = 440;
GLuint background = BITMAP_UNKNOWN, scene = BITMAP_UNKNOWN;
HFONT normal = nullptr, bold = nullptr, nameFont = nullptr;
int fontSize = 0, originX = 0, originY = 0, cardWidth = 0, cardHeight = 0;
int X(int v)
{
    return originX + v * cardWidth / ReferenceWidth;
}
int Y(int v)
{
    return originY + v * cardHeight / ReferenceHeight;
}
int W(int v)
{
    return v * cardWidth / ReferenceWidth;
}
int H(int v)
{
    return v * cardHeight / ReferenceHeight;
}
void ReleaseFonts()
{
    g_pRenderText->SetFont(g_hFont);
    for (auto f : {normal, bold, nameFont})
        if (f)
            DeleteObject(f);
    normal = bold = nameFont = nullptr;
    fontSize = 0;
}
void SetFonts()
{
    const int size = std::max(9, static_cast<int>(16 * cardWidth * g_fScreenRate_x / ReferenceWidth));
    if (fontSize == size)
        return;
    ReleaseFonts();
    fontSize = size;
    auto make = [](int pixels, int weight, const wchar_t *family) {
        return CreateFontW(pixels, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, family);
    };
    normal = make(size, FW_NORMAL, L"Segoe UI");
    bold = make(size, FW_BOLD, L"Segoe UI");
    nameFont = make(size * 3 / 2, FW_BOLD, L"Georgia");
}
void Text(int x, int y, int width, const wchar_t *text, bool gold = false, int alignment = RT3_SORT_LEFT,
          HFONT font = nullptr)
{
    UI::Paris::Color(1.f, 1.f, 1.f, 1.f);
    g_pRenderText->SetFont(font ? font : normal);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(gold ? 238 : 255, gold ? 211 : 255, gold ? 139 : 255, 255);
    g_pRenderText->RenderText(X(x), Y(y), text, W(width), 0, alignment);
}
void Value(int x, int y, int width, std::int64_t value, bool available, int alignment = RT3_SORT_CENTER)
{
    wchar_t text[64]{};
    if (available)
        swprintf_s(text, L"%lld", value);
    else
        wcscpy_s(text, L"—");
    Text(x, y, width, text, true, alignment, bold);
}
void RenderIdentity(const RankUserCardData &data)
{
    Text(202, 58, 204, data.Fields & 1 ? data.Name : L"Nombre privado", true, RT3_SORT_LEFT, nameFont);
    Text(202, 89, 204, data.Fields & 4 ? data.Class : L"Clase privada");
    if ((data.Fields & 2) && data.Rank[0])
        Text(202, 113, 204, data.Rank, true, RT3_SORT_LEFT, bold);
    if ((data.Fields & 128) && data.Emblem != BITMAP_UNKNOWN)
    {
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(data.Emblem, X(414), Y(52), W(70), H(70), 0, 0, 80.f / 128.f, 80.f / 128.f);
    }
    else if (data.GlobalRank > 0)
        Value(414, 77, 70, data.GlobalRank, true);
}
void RenderSummary(const RankUserCardData &data)
{
    constexpr int columns[]{201, 290, 381}, widths[]{87, 89, 103};
    const wchar_t *labels[]{L"Nivel", L"Reset", L"Total Stats"};
    for (int col = 0; col < 3; ++col)
        Text(columns[col], 144, widths[col], labels[col], false, RT3_SORT_CENTER);
    std::int64_t total = data.GuildMember ? data.Values[3] : 0;
    if (!data.GuildMember)
        for (int i = 3; i < 8; ++i)
            total += data.Values[i];
    Value(columns[0], 167, widths[0], data.Values[0], data.Fields & 8);
    Value(columns[1], 167, widths[1], data.Values[1], data.Fields & 16);
    Value(columns[2], 167, widths[2], total, data.Fields & 64);
}
void RenderAttributes(const RankUserCardData &data)
{
    const bool publicStats = (data.Fields & 64) != 0;
    ParisStyle::Solid(X(332), Y(202), W(152), H(22), publicStats ? .1f : .3f, publicStats ? .35f : .2f, .12f);
    ParisStyle::Solid(X(334), Y(204), W(148), H(18), .03f, .07f, .04f);
    Text(334, 206, 148, publicStats ? L"STATS PUBLICOS" : L"STATS PRIVADOS", true, RT3_SORT_CENTER, bold);
    const wchar_t *labels[]{L"Fuerza", L"Agilidad", L"Vitalidad", L"Energia", L"Comando"};
    for (int row = 0; row < 5; ++row)
    {
        Text(244, 235 + row * 30, 150, labels[row]);
        Value(393, 235 + row * 30, 86, data.Values[3 + row], publicStats && !data.GuildMember, RT3_SORT_RIGHT);
    }
    Text(244, 389, 235,
         data.Fields & 32 ? (data.Values[2] ? L"Cuenta VIP" : L"Cuenta normal") : L"Cuenta no publicada");
}
void RenderCharacter(const RankUserCardData &data)
{
    if (!(data.Fields & 4))
        return;
    if (scene != BITMAP_UNKNOWN)
    {
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(scene, X(11), Y(48), W(177), H(362));
    }
    if (data.Character && g_pCustomRankingWindow)
        g_pCustomRankingWindow->RenderInspectionCharacter(*data.Character, X(12), Y(49), W(175), H(360));
    else
        Portraits::RenderPortrait(data.BaseClass, X(12), Y(49), W(175), H(360));
}
} // namespace
void Load()
{
    if (background == BITMAP_UNKNOWN)
        background = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankUserCardV20.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    if (scene == BITMAP_UNKNOWN)
        scene = Bitmaps.LoadImage(L"Data\\Interface\\Paris_RankCardSceneV20.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
}
void Unload()
{
    for (auto image : {background, scene})
        if (image != BITMAP_UNKNOWN)
            DeleteBitmap(image);
    background = scene = BITMAP_UNKNOWN;
    ReleaseFonts();
}
void Render(const RankUserCardData &data, int x, int y, int width, int height)
{
    originX = x;
    originY = y;
    cardWidth = width;
    cardHeight = height;
    SetFonts();
    EnableAlphaTest();
    UI::Paris::Color(1, 1, 1, 1);
    if (background != BITMAP_UNKNOWN)
        RenderBitmap(background, x, y, width, height);
    else
        ParisFlat::Box(x, y, width, height, true);
    RenderCharacter(data);
    DisableDepthTest();
    DisableDepthMask();
    mu::GetRenderer().SetAlphaTest(true);
    EnableAlphaTest();
    RenderIdentity(data);
    RenderSummary(data);
    RenderAttributes(data);
    g_pRenderText->SetFont(g_hFont);
}
} // namespace UI::Character::RankCard
