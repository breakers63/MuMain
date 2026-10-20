#pragma once
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
namespace UI::Theme
{
    class ParisCompanionArt46
    {
      public:
        void Load();
        void Unload();
        void Tab(int x, int y, int width, int index, const wchar_t *label, bool selected) const;

      private:
        GLuint icons = BITMAP_UNKNOWN;
        void Border(int x, int y, int width) const;
    };
} // namespace UI::Theme
