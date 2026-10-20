#pragma once

#include "Render/Sprites/GlobalBitmap.h"

namespace UI::Inventory
{
    class VaultPanelArt
    {
      public:
        enum class Icon
        {
            Deposit,
            Withdraw,
            Locked,
            Unlocked,
            Expand
        };
        static constexpr int ButtonSize = 32;
        static constexpr int FooterTop = 378;
        static constexpr int CloseLeft = 14;
        static constexpr int FirstActionLeft = 46;

        void Load();
        void Unload();
        void Frame(const POINT &position) const;
        void Close(const POINT &position) const;
        void Button(const POINT &position, Icon icon, bool enabled) const;
        bool ClosePressed(const POINT &position) const;

      private:
        GLuint m_frame = BITMAP_UNKNOWN;
        GLuint m_buttons = BITMAP_UNKNOWN;
        GLuint m_close = BITMAP_UNKNOWN;
    };
} // namespace UI::Inventory
