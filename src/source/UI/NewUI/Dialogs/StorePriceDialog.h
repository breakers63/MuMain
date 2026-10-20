#pragma once
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/Character/ParisFrameAssets.h"
#include "UI/Legacy/UIControls.h"
#include <array>
#include <memory>

namespace UI::Items
{
    class StorePriceDialog final : public SEASON3B::INewUI3DRenderObj
    {
    public:
        enum class Action { None, Handled, Confirm, Cancel };
        static constexpr int Width = 300, Height = 430;
        ~StorePriceDialog();
        void Open(int x, int y);
        void Close();
        bool IsVisible() const override;
        void Render3D() override;
        void Render();
        void Update();
        Action Click();
        bool ReadPrice(int& amount, BYTE& currency) const;
    private:
        static constexpr int Count = 7, RowY = 164, RowStep = 29;
        static constexpr int PreviewX = 102, PreviewY = 60, PreviewSize = 96;
        static constexpr int RowX = 12, RowWidth = Width - 24, RowHeight = 27;
        static constexpr int ArrowX = 254, ArrowWidth = 19, ArrowHeight = 11;
        static constexpr int AmountX = 149, AmountY = 9, AmountWidth = 98, AmountHeight = 13;
        bool open = false, registered = false;
        int left = 0, top = 0, selected = 0;
        GLuint eiffel = BITMAP_UNKNOWN, details = BITMAP_UNKNOWN;
        UI::Theme::ParisFrameAssets art;
        std::array<std::unique_ptr<CUITextInputBox>, Count> inputs;
        void RenderHeader();
        void RenderRow(int index);
        void RenderButtons();
        void RenderPreview();
        void Adjust(int index, int delta);
        static void Text(int x, int y, int width, const wchar_t* label, bool gold = false);
    };
}
