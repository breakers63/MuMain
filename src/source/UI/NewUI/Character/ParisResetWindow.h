#pragma once
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
namespace SEASON3B
{
class CParisResetWindow final : public CNewUIObj
{
public:
    ~CParisResetWindow() override;
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
} // namespace SEASON3B
