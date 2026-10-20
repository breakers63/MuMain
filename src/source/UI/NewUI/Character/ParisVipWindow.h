#pragma once
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
namespace SEASON3B
{
    constexpr DWORD INTERFACE_PARIS_VIP = 0x50564950u;
    class CParisVipWindow final : public CNewUIObj
    {
    public:
        ~CParisVipWindow() override;
        bool Create(CNewUIManager* manager);
        void Release();
        void ToggleWindow();
        static bool ReceivePacket(const BYTE* packet, int size);
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;
        float GetLayerDepth() override;
        float GetKeyEventOrder() override;
    private:
        CNewUIManager* m_manager = nullptr;
    };
}
