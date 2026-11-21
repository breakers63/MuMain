#include "stdafx.h"
#include "ParisWindowGroup.h"
#include "GestureWheelWindow.h"
#include "GenericMenuDialog.h"
#include <array>
#include <algorithm>
#include "ParisStorePriceWindow.h"
#include "ParisJewelBankWindow.h"
#include "ParisMarketWindow.h"
#include "ParisAuctionWindow.h"
#include "ParisRankUserWindow.h"
#include "ParisRankingWindow.h"
#include "ParisActivityLogWindow.h"
#include "ParisResetWindow.h"
#include "ParisVipWindow.h"
#include "ParisBattlePassWindow.h"
#include "ParisEventsWindow.h"
#include "ParisAdvancedStatsWindow.h"
#include "ParisGmEventsWindow.h"
#include "ParisMentorWindow.h"
#include "ParisSoulWindow.h"

namespace mu::ui::window
{
    namespace
    {
        auto ParisPanels()
        {
        const std::array<CObject*, 15> panels = {
            g_pParisJewelBankWindow,
            g_pParisMarketWindow,
            g_pParisAuctionWindow,
            g_pParisRankUserWindow,
            g_pParisRankingWindow,
            g_pParisActivityLogWindow,
            g_pParisResetWindow,
            g_pParisVipWindow,
            g_pParisBattlePassWindow,
            g_pParisEventsWindow,
            g_pParisAdvancedStatsWindow,
            g_pParisGmEventsWindow,
            g_pParisMentorWindow,
            g_pParisSoulWindow, g_pGestureWheelWindow};
            return panels;
        }
    }

    bool HasOpenParisWindow()
    {
        if (g_pGenericMenuDialog && g_pGenericMenuDialog->IsVisible()) return true;
        if (g_pParisStorePriceWindow && g_pParisStorePriceWindow->IsVisible()) return true;
        const auto panels = ParisPanels();
        return std::any_of(panels.begin(), panels.end(), [](const CObject* panel)
            { return panel && panel->IsVisible(); });
    }

    void CloseOtherParisWindows(CObject* opening)
    {
        const auto panels = ParisPanels();
        for (auto* panel : panels)
            if (panel && panel != opening && panel->IsVisible()) panel->Show(false);
        if (g_pGenericMenuDialog && opening != g_pGenericMenuDialog)
            g_pGenericMenuDialog->CloseCustomMenu();
    }
}

