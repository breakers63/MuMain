#pragma once
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
namespace SEASON3B
{
    constexpr DWORD INTERFACE_PARIS_JEWEL_BANK = 0x504A424Bu;
    class CParisJewelBankWindow final : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        struct Jewel
        {
            int ItemIndex = -1, Level = 0, Inventory = 0, Maximum = 0;
            long long Balance = 0;
            wchar_t Name[48] = {}, InventoryText[24] = {}, BalanceText[24] = {};
        };
        ~CParisJewelBankWindow() override;
        bool Create(CNewUIManager* manager);
        void Release();
        static bool ReceiveText(const wchar_t* text);
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;
        void Render3D() override;
        bool IsVisible() const override;
        CNewUIObj* GetLayoutOwner() const override;
        float GetLayerDepth() override;
        float GetKeyEventOrder() override;
    private:
        CNewUIManager* m_manager = nullptr;
    };
}
