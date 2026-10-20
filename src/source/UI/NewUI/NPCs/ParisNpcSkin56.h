#pragma once
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
namespace UI::Theme
{
    class ParisNpcSkin56
    {
      public:
        enum class ActionIcon
        {
            Mix = 5
        };
        bool Loaded() const;
        void Load();
        void Unload();
        void Panel(float x, float y, float width = 190, float height = 429, bool showClose = true,
                   bool showCrest = true) const;
        void Base(float x, float y, float width, float height, bool enabled = true, bool hover = false) const;
        void Caption(float x, float y, float width, float height, const wchar_t *text, bool enabled = true) const;
        void Close(float x, float y, float width = 32, float height = 32) const;
        void NativeButton(SEASON3B::CNewUIButton &native, bool exit = false) const;
        void ActionButton(SEASON3B::CNewUIButton &native, ActionIcon icon) const;
        void Separator(float x, float y, float width) const;

      private:
        GLuint frame = BITMAP_UNKNOWN, button = BITMAP_UNKNOWN, close = BITMAP_UNKNOWN;
        GLuint crest = BITMAP_UNKNOWN, headerClose = BITMAP_UNKNOWN;
        GLuint actionIcons = BITMAP_UNKNOWN;
    };
} // namespace UI::Theme
