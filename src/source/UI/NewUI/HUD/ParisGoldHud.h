#pragma once
#include "UI/Scaling/UITransform.h"

namespace UI::Hud::Gold
{
    inline constexpr float CurrentX = 303.5f, CurrentY = 446.5f;
    inline constexpr float CurrentWidth = 19.f, CurrentHeight = 24.f;
    inline constexpr int PotionX = 204, PotionY = 446, PotionWidth = 17, PotionHeight = 25;
    inline constexpr int PotionStep = 34, PotionCount = 3;
    inline constexpr int ButtonX = 333, ButtonY = 443, ButtonWidth = 28, ButtonHeight = 30, ButtonStep = 34;
    inline constexpr int PopupColumns = 10, PopupWidth = 248, PopupX = 196;
    inline constexpr float PopupIconWidth = 20.f, PopupIconHeight = 28.f;
    void Load();
    void Unload();
    bool Ready();
    void Frame();
    void Vitals();
    void Gauges();
    void Experience();
    bool RenderableSkill(int skill);
    int SkillIndex(int ordinal);
    UI::Scaling::Position SkillPosition(int index);
    void SkillSocket(float x, float y, bool selected);
}
