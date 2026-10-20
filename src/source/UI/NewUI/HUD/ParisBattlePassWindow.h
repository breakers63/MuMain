#pragma once
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
namespace SEASON3B
{
class CParisBattlePassWindow final : public CNewUIObj, public INewUI3DRenderObj
{
public:
    ~CParisBattlePassWindow() override;
    bool Create(CNewUIManager* manager);
    void Release();
    void ToggleWindow();
    void CloseWindow();
    static bool ReceivePacket(const BYTE* packet, int size);
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
