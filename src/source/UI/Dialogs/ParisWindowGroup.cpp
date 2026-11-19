#include "stdafx.h"

#include "ParisWindowGroup.h"

#include "GestureWheelWindow.h"

#include "GenericMenuDialog.h"

#include <array>

#include "ParisJewelBankWindow.h"

#include "ParisMarketWindow.h"

#include "ParisAuctionWindow.h"

#include "ParisRankUserWindow.h"

#include "ParisRankingWindow.h"

#include "ParisActivityLogWindow.h"

#include "ParisBountyBoardWindow.h"

#include "ParisResetWindow.h"

#include "ParisVipWindow.h"

#include "ParisBattlePassWindow.h"

#include "ParisEventsWindow.h"

#include "ParisAdvancedStatsWindow.h"

#include "ParisGmEventsWindow.h"

#include "ParisMentorWindow.h"

#include "ParisSoulWindow.h"

#include "ParisItemDyeWindow.h"

#include "UI/Core/WindowSystem.h"



namespace mu::ui::window

{

    void CloseOtherParisWindows(CObject* opening)

    {

        const bool needsInventory = opening == g_pParisItemDyeWindow || opening == g_pParisJewelBankWindow;

        if (g_pNewUISystem && !needsInventory)

        {

            if (g_pNewUISystem->IsVisible(INTERFACE_INVENTORY_EXT))

                g_pNewUISystem->Hide(INTERFACE_INVENTORY_EXT);

            if (g_pNewUISystem->IsVisible(INTERFACE_INVENTORY))

                g_pNewUISystem->Hide(INTERFACE_INVENTORY);

        }

        const std::array<CObject*, 17> panels = {

            g_pParisJewelBankWindow,

            g_pParisMarketWindow,

            g_pParisAuctionWindow,

            g_pParisRankUserWindow,

            g_pParisRankingWindow,

            g_pParisActivityLogWindow,

            g_pParisBountyBoardWindow,

            g_pParisResetWindow,

            g_pParisVipWindow,

            g_pParisBattlePassWindow,

            g_pParisEventsWindow,

            g_pParisAdvancedStatsWindow,

            g_pParisGmEventsWindow,

            g_pParisMentorWindow,

            g_pParisSoulWindow, g_pParisItemDyeWindow, g_pGestureWheelWindow};

        for (auto* panel : panels)

            if (panel && panel != opening && panel->IsVisible()) panel->Show(false);

        if (g_pGenericMenuDialog && opening != g_pGenericMenuDialog)

            g_pGenericMenuDialog->CloseCustomMenu();

    }

}

