#include "stdafx.h"
#include "UI/NewUI/HUD/ParisGoldHud.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/NewUI/Character/ExperienceProgress.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/WSclient.h"
#include "I18N/All.h"

namespace UI::Hud::Gold
{
    namespace
    {
        GLuint background = BITMAP_UNKNOWN, life = BITMAP_UNKNOWN, mana = BITMAP_UNKNOWN;
        constexpr float PanelY = 420.f, PanelWidth = 640.f, PanelHeight = 60.f, PanelV = .75f;
        constexpr float HpX = 101.f, HpY = 425.f, HpWidth = 54.f, HpHeight = 51.f;
        constexpr float MpX = 484.f, MpY = 421.f, MpWidth = 62.f, MpHeight = 53.f;
        constexpr float OrbPixels = 144.f, ExpX = 195.f, ExpY = 431.f, ExpWidth = 225.f, ExpHeight = 7.f;
        constexpr float GaugeY = 434.f, GaugeWidth = 9.f, GaugeHeight = 35.f, AgX = 555.f, SdX = 449.f;
        constexpr int TooltipGap = 14, PopupStepX = 24, PopupStepY = 34, PopupTop = 380;

        void Solid(float x, float y, float width, float height, float r, float g, float b)
        {
            if (width <= 0 || height <= 0) return;
            EnableAlphaTest();
            UI::Paris::Color(r, g, b, 1);
            RenderColor(x, y, width, height);
            EndRenderColor();
            UI::Paris::Color(1, 1, 1, 1);
        }

        void Orb(GLuint texture, float x, float y, float width, float height, DWORD value, DWORD maximum)
        {
            const auto* bitmap = Bitmaps.FindTexture(texture);
            if (!bitmap || bitmap->Width <= 0 || bitmap->Height <= 0 || maximum == 0) return;
            const float ratio = static_cast<float>(std::min(value, maximum)) / maximum;
            const float hidden = 1.f - ratio;
            if (ratio <= 0) return;
            UI::Paris::Color(1, 1, 1, 1);
            RenderBitmap(texture, x, y + hidden * height, width, height * ratio, 0,
                hidden * OrbPixels / bitmap->Height, OrbPixels / bitmap->Width, ratio * OrbPixels / bitmap->Height);
        }

        void Gauge(float x, DWORD value, DWORD maximum, bool shield)
        {
            const float ratio = maximum ? static_cast<float>(std::min(value, maximum)) / maximum : 0;
            Solid(x, GaugeY + GaugeHeight * (1 - ratio), GaugeWidth, GaugeHeight * ratio,
                shield ? .95f : .55f, shield ? .8f : .2f, shield ? .12f : .85f);
        }
    }

    void Load()
    {
        if (background == BITMAP_UNKNOWN)
            background = Bitmaps.LoadImage(L"Data\\Interface\\Paris_HudReference35.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        if (life == BITMAP_UNKNOWN)
            life = Bitmaps.LoadImage(L"Data\\Interface\\paris_hp.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        if (mana == BITMAP_UNKNOWN)
            mana = Bitmaps.LoadImage(L"Data\\Interface\\paris_mp.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void Unload()
    {
        for (auto texture : {background, life, mana})
            if (texture != BITMAP_UNKNOWN) DeleteBitmap(texture);
        background = life = mana = BITMAP_UNKNOWN;
    }

    bool Ready() { return background != BITMAP_UNKNOWN && life != BITMAP_UNKNOWN && mana != BITMAP_UNKNOWN; }

    void Frame()
    {
        EnableAlphaTest();
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(background, 0, PanelY, PanelWidth, PanelHeight, 0, 0, 1, PanelV);
    }

    void Vitals()
    {
        const DWORD hp = std::min(CharacterAttribute->Life, CharacterAttribute->LifeMax);
        const DWORD mp = std::min(CharacterAttribute->Mana, CharacterAttribute->ManaMax);
        Orb(life, HpX, HpY, HpWidth, HpHeight, hp, CharacterAttribute->LifeMax);
        Orb(mana, MpX, MpY, MpWidth, MpHeight, mp, CharacterAttribute->ManaMax);
        SEASON3B::RenderNumber(128, 460, hp);
        SEASON3B::RenderNumber(504, 460, mp);
        wchar_t text[256];
        if (SEASON3B::CheckMouseIn(HpX, HpY, HpWidth, HpHeight))
        {
            mu_swprintf(text, I18N::Game::LifeDD, hp, CharacterAttribute->LifeMax);
            RenderTipText(HpX, HpY - TooltipGap, text);
        }
        if (SEASON3B::CheckMouseIn(MpX, MpY, MpWidth, MpHeight))
        {
            mu_swprintf(text, I18N::Game::ManaDD359, mp, CharacterAttribute->ManaMax);
            RenderTipText(MpX, MpY - TooltipGap, text);
        }
    }

    void Gauges()
    {
        Gauge(AgX, CharacterAttribute->SkillMana, CharacterAttribute->SkillManaMax, false);
        Gauge(SdX, CharacterAttribute->Shield, CharacterAttribute->ShieldMax, true);
    }

    void Experience()
    {
        const bool master = gCharacterManager.IsMasterExperienceActive(CharacterAttribute->Class, CharacterAttribute->Level);
        const long long current = master ? Master_Level_Data.lMasterLevel_Experince : CharacterAttribute->Experience;
        const long long next = master ? Master_Level_Data.lNext_MasterLevel_Experince : CharacterAttribute->NextExperience;
        const long long level = master ? Master_Level_Data.nMLevel : CharacterAttribute->Level;
        const float width = static_cast<float>(UI::Character::ExperienceProgress(current, next, level, master) * ExpWidth);
        Solid(ExpX, ExpY, width, ExpHeight, 1.f, .82f, .04f);
        Solid(ExpX, ExpY, width, 1.f, 1.f, .97f, .46f);
        if (!SEASON3B::CheckMouseIn(ExpX, ExpY - 3, ExpWidth, ExpHeight + 6)) return;
        wchar_t text[256];
        mu_swprintf(text, I18N::Game::EXPI64dI64d, current, next);
        RenderTipText(ExpX, ExpY - TooltipGap, text);
    }

    bool RenderableSkill(int skill)
    {
        if (skill <= 0 || skill >= MAX_SKILLS || (skill >= AT_SKILL_STUN && skill <= AT_SKILL_REMOVAL_BUFF)) return false;
        const auto type = SkillAttribute[skill].SkillUseType;
        return type != SKILL_USE_TYPE_MASTER && type != SKILL_USE_TYPE_MASTERLEVEL;
    }

    int SkillIndex(int ordinal)
    {
        if (ordinal < MAX_MAGIC)
            return RenderableSkill(CharacterAttribute->Skill[ordinal]) ? ordinal : -1;
        const int command = AT_PET_COMMAND_DEFAULT + ordinal - MAX_MAGIC;
        return Hero->m_pPet && command < AT_PET_COMMAND_END ? command : -1;
    }

    UI::Scaling::Position SkillPosition(int index)
    {
        return {PopupX + 5.f + (index % PopupColumns) * PopupStepX,
            static_cast<float>(PopupTop - (index / PopupColumns) * PopupStepY)};
    }

    void SkillSocket(float x, float y, bool selected)
    {
        Solid(x - 2, y - 2, PopupIconWidth + 4, PopupIconHeight + 4, .82f, .60f, .14f);
        Solid(x - 1, y - 1, PopupIconWidth + 2, PopupIconHeight + 2, .025f, .025f, .025f);
        if (selected) Solid(x, y + PopupIconHeight, PopupIconWidth, 1, 1.f, .85f, .2f);
    }
}
