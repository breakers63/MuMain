#include "stdafx.h"
#include "UI/NewUI/Dialogs/StorePriceDialog.h"
#include "UI/NewUI/Character/ParisControls.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"
#include "UI/NewUI/Inventory/NewUIMarketWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/HUD/NewUIChatLogWindow.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "GameLogic/Items/StorePriceInput.h"
#include "GameLogic/Items/ItemCategories.h"
#include "Network/Paris/PersonalShopPricing.h"
#include "Audio/DSPlaySound.h"

namespace UI::Items
{
    namespace Prices = GameLogic::Items::StorePrice;
    namespace Pricing = Network::Paris::PersonalShopPricing;
    using namespace SEASON3B;

    StorePriceDialog::~StorePriceDialog() { Close(); }
    bool StorePriceDialog::IsVisible() const { return open; }

    void StorePriceDialog::Open(int x, int y)
    {
        Close();
        left = x; top = y; selected = 0;
        art.Load();
        eiffel = Bitmaps.LoadImage(L"Data\\Interface\\Paris_EiffelCoin.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        details = Bitmaps.LoadImage(L"Data\\Interface\\Paris_SellDetails.tga", GL_LINEAR, GL_CLAMP_TO_EDGE);
        for (int index = 0; index < Count; ++index)
        {
            auto& input = inputs[index];
            input = std::make_unique<CUITextInputBox>();
            input->Init(g_hWnd, AmountWidth, AmountHeight, 10, false);
            input->SetOption(UIOPTION_NUMBERONLY | UIOPTION_PAINTBACK);
            input->SetTextColor(255, 255, 255, 255);
            input->SetBackColor(255, 0, 0, 0);
            input->SetFont(g_hFont);
            input->SetPosition(left + AmountX, top + RowY + index * RowStep + AmountY);
            input->SetState(UISTATE_NORMAL);
            input->SetText(L"0");
        }
        open = true;
        inputs[0]->GiveFocus();
        if (g_pEtroMarketWindow) g_pEtroMarketWindow->AnnounceCapability(true);
        if (!g_pNewUI3DRenderMng) return;
        g_pNewUI3DRenderMng->Add3DRenderObj(this);
        registered = true;
    }

    void StorePriceDialog::Close()
    {
        open = false;
        if (registered && g_pNewUI3DRenderMng) g_pNewUI3DRenderMng->Remove3DRenderObj(this);
        registered = false;
        for (auto& input : inputs) input.reset();
        art.Unload();
        if (eiffel != BITMAP_UNKNOWN) DeleteBitmap(eiffel);
        if (details != BITMAP_UNKNOWN) DeleteBitmap(details);
        eiffel = details = BITMAP_UNKNOWN;
    }

    void StorePriceDialog::Text(int x, int y, int width, const wchar_t* label, bool gold)
    {
        g_pRenderText->SetFont(gold ? g_hFontBold : g_hFont);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->SetTextColor(255, gold ? 220 : 255, gold ? 135 : 255, 255);
        g_pRenderText->RenderText(x, y, label, width, 0, RT3_SORT_CENTER);
    }

    void StorePriceDialog::RenderHeader()
    {
        const int center = left + Width / 2;
        if (details != BITMAP_UNKNOWN)
        {
            RenderBitmap(details, center - 34, top - 19, 68, 40,
                650.f / 1024, 0, 152.f / 1024, 89.f / 256);
            RenderBitmap(details, center - 88, top + 27, 176, 20,
                0, 100.f / 256, 391.f / 1024, 44.f / 256);
        }
        else Text(left + 20, top + 28, Width - 40, L"VENDER ARTÍCULO", true);
        art.SpriteButton(left + Width - 34, top + 13, 22, 22, 0);
        UI::Theme::Bevel(left + PreviewX - 5, top + PreviewY - 5,
            PreviewSize + 10, PreviewSize + 10, L"");
    }

    void StorePriceDialog::RenderRow(int index)
    {
        const int y = top + RowY + index * RowStep;
        const bool active = selected == index || inputs[index]->HaveFocus();
        const bool enabled = Pricing::CanUse(Prices::Currencies[index]);
        RenderColorQuadARGB(left + RowX, y, RowWidth, RowHeight, active ? 0x993F2C0E : 0x88080808);
        RenderColorQuadARGB(left + RowX, y + RowHeight - 1, RowWidth, 1, 0xDD705220);
        Text(left + 60, y + 5, 76, Prices::Names[index], active);
        UI::Theme::Bevel(left + ArrowX, y + 3, ArrowWidth, ArrowHeight, L"▲", false, enabled);
        UI::Theme::Bevel(left + ArrowX, y + 16, ArrowWidth, ArrowHeight, L"▼", false, enabled);
        if (index == 1 && eiffel != BITMAP_UNKNOWN)
            RenderBitmap(eiffel, left + 29, y - 1, 24, 30, 0, 0, 1, 80.f / 128);
        if (index == 0 && details != BITMAP_UNKNOWN)
            RenderBitmap(details, left + 29, y + 3, 24, 22, 580.f / 1024, 0, 51.f / 1024, 45.f / 256);
        inputs[index]->Render();
    }

    void StorePriceDialog::RenderButtons()
    {
        constexpr int ButtonY = Height - 48;
        if (details == BITMAP_UNKNOWN)
        {
            UI::Theme::Bevel(left + 24, top + ButtonY, 124, 34, L"PUBLICAR");
            UI::Theme::Bevel(left + 164, top + ButtonY, 112, 34, L"CANCELAR");
            return;
        }
        UI::Paris::Color(1, 1, 1, 1);
        RenderBitmap(details, left + 24, top + ButtonY, 124, 34, 0, 0, 281.f / 1024, 75.f / 256);
        RenderBitmap(details, left + 164, top + ButtonY, 112, 34, 300.f / 1024, 0, 260.f / 1024, 75.f / 256);
    }

    void StorePriceDialog::Render()
    {
        if (!open) return;
        EnableAlphaTest();
        art.Frame(left, top, Width, Height);
        RenderHeader();
        for (int index = 0; index < Count; ++index) RenderRow(index);
        RenderButtons();
    }

    void StorePriceDialog::Update()
    {
        CUITextInputBox* focused = nullptr;
        for (const auto& input : inputs)
        {
            if (!input) continue;
            input->DoAction();
            if (input->HaveFocus()) focused = input.get();
        }
        const auto handle = focused ? focused->GetHandle() : g_hWnd;
        if (g_MessageBox->GetRelatedWnd() != handle) g_MessageBox->SetRelatedWnd(handle);
    }

    void StorePriceDialog::Adjust(int index, int delta)
    {
        wchar_t text[32]{};
        inputs[index]->GetText(text);
        int value = 0;
        Prices::ParseAmount(text, value);
        value = static_cast<int>(std::clamp(static_cast<long long>(value) + delta, 0LL,
            static_cast<long long>(INT_MAX)));
        swprintf_s(text, L"%d", value);
        inputs[index]->SetText(text);
        inputs[index]->GiveFocus();
        selected = index;
    }

    StorePriceDialog::Action StorePriceDialog::Click()
    {
        if (SEASON3B::CheckMouseIn(left + Width - 38, top + 12, 26, 24)) return Action::Cancel;
        if (SEASON3B::CheckMouseIn(left + 24, top + Height - 48, 124, 34)) return Action::Confirm;
        if (SEASON3B::CheckMouseIn(left + 164, top + Height - 48, 112, 34)) return Action::Cancel;
        for (int index = 0; index < Count; ++index)
        {
            const int y = top + RowY + index * RowStep;
            const bool up = SEASON3B::CheckMouseIn(left + ArrowX, y + 3, ArrowWidth, ArrowHeight);
            const bool down = SEASON3B::CheckMouseIn(left + ArrowX, y + 16, ArrowWidth, ArrowHeight);
            if ((up || down) && Pricing::CanUse(Prices::Currencies[index])) Adjust(index, up ? 1 : -1);
            if (SEASON3B::CheckMouseIn(left + 25, y - 1, 111, 30)) selected = index;
            if (up || down) return Action::Handled;
        }
        return Action::None;
    }

    bool StorePriceDialog::ReadPrice(int& amount, BYTE& currency) const
    {
        std::array<std::array<wchar_t, 32>, Count> texts{};
        std::array<std::wstring_view, Count> values{};
        for (int index = 0; index < Count; ++index)
        {
            if (inputs[index]) inputs[index]->GetText(texts[index].data());
            values[index] = texts[index].data();
        }
        return Prices::Select(values, Pricing::CanUse(1), amount, currency);
    }

    void StorePriceDialog::RenderPreview()
    {
        ITEM* item = nullptr;
        if (auto* picked = CNewUIInventoryCtrl::GetPickedItem()) item = picked->GetItem();
        else if (g_pMyShopInventory) item = g_pMyShopInventory->FindItem(g_pMyShopInventory->GetSourceIndex());
        if (!item) return;
        constexpr int Inset = 8;
        const float scale = std::min(.65f, (PreviewSize - Inset * 2) /
            (36.f * std::max(1, static_cast<int>(ItemAttribute[item->Type].Height))));
        RenderMarketPreviewItem3D(left + PreviewX + Inset, top + PreviewY + Inset,
            PreviewSize - Inset * 2, PreviewSize - Inset * 2, item->Type, item->Level,
            item->ExcellentFlags, item->AncientDiscriminator, scale);
    }

    void StorePriceDialog::Render3D()
    {
        if (!open) return;
        UI::Scaling::ScopedActiveTransform layout(UI::Scaling::TransformForLayout(
            UI::Scaling::LayoutMode::Dialog, WindowWidth, WindowHeight));
        RenderPreview();
        constexpr int jewels[]{ITEM_JEWEL_OF_BLESS, ITEM_JEWEL_OF_SOUL, ITEM_JEWEL_OF_LIFE,
            ITEM_JEWEL_OF_CREATION, ITEM_JEWEL_OF_CHAOS};
        for (int index = 2; index < Count; ++index)
            RenderItem3D(left + 25, top + RowY + index * RowStep - 1, 32, 30, jewels[index - 2], 0, 0, 0, true);
    }
}
