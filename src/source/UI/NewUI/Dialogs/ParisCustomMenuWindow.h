#pragma once
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Quests/NewUICustomQuestCenter.h"
#include "UI/NewUI/Inventory/ParisJewelBankWindow.h"
#include "UI/NewUI/Character/ParisVipWindow.h"
#include "UI/NewUI/Character/NewUIAdvancedStatsWindow.h"
#include "UI/NewUI/Character/ParisResetWindow.h"
#include "UI/NewUI/Character/ParisAllocationWindow.h"
#include "UI/NewUI/Events/ParisEventsWindow.h"
#include <array>
#include <memory>
#include "UI/NewUI/HUD/CustomFeaturesWindow.h"
#include "UI/NewUI/Character/NewUICustomRanking.h"
#include "UI/NewUI/Inventory/NewUIMarketWindow.h"
#include "UI/NewUI/HUD/ParisBattlePassWindow.h"

namespace SEASON3B
{
class CParisCustomMenuWindow final : public CNewUIObj
{
public:
    ~CParisCustomMenuWindow() override;
    bool Create(CNewUIManager* manager);
    void Release();
    static void TogglePanel();
    static void CloseAllOverlays(CNewUIObj* keep = nullptr);
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;
    bool IsVisible() const override;
    float GetLayerDepth() override;
    float GetKeyEventOrder() override;

private:
    void Activate(int index);
    void CloseOverlays(CNewUIObj* keep = nullptr);
    void RenderCard(int index) const;
    void ActivateAdditional(int index);
    void RenderAdditionalIcon(int index, int x, int y) const;
    CNewUIManager* m_manager = nullptr;
    unsigned int m_icons = 0;
    unsigned int m_toggle = 0;
    unsigned int m_guidesIcon = BITMAP_UNKNOWN, m_eventsIcon = BITMAP_UNKNOWN;
    unsigned int m_infoIcon = BITMAP_UNKNOWN, m_extraIcons = BITMAP_UNKNOWN, m_featureIcons = BITMAP_UNKNOWN;
    CNewUICustomQuestCenter m_questWindow;
    CParisJewelBankWindow m_jewelBankWindow;
    CParisVipWindow m_vipWindow;
    CNewUIAdvancedStatsWindow m_infoWindow;
    CParisResetWindow m_resetWindow;
    CParisAllocationWindow m_allocationWindow;
    CParisEventsWindow m_eventsWindow;
    CNewUICustomRanking m_rankingWindow;
    CNewUIMarketWindow m_marketWindow;
    CParisBattlePassWindow m_battlePassWindow;
    std::unique_ptr<CParisFeatureWindow> m_featuresWindow;
};
} // namespace SEASON3B
