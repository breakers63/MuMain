#pragma once
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUI3DRenderMng.h"

namespace SEASON3B
{
class CParisFeatureWindow final : public CNewUIObj, public INewUI3DRenderObj
{
public:
    ~CParisFeatureWindow() override;
    bool Create(CNewUIManager* manager, CNewUI3DRenderMng* renderManager);
    void Release();
    void Open(int action);
    void Close();
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
    static bool AnyOpen();
    CNewUIManager* m_manager = nullptr;
    CNewUI3DRenderMng* m_renderManager = nullptr;
};
}
