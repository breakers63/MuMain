#pragma once
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
namespace SEASON3B
{
class CParisEventsWindow final : public CNewUIObj
{
public:
    ~CParisEventsWindow() override;
    bool Create(CNewUIManager* manager);
    void Release();
    void ToggleWindow();
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    float GetLayerDepth() override;
    float GetKeyEventOrder() override;

private:
    CNewUIManager* m_manager = nullptr;
    int m_page = 0;
};
} // namespace SEASON3B
