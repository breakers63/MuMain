#include "stdafx.h"
#include "UI/NewUI/Dialogs/PersonalShopOfflineDialog.h"
#include "Audio/DSPlaySound.h"
#include "Network/Paris/PersonalShopActions.h"
#include "UI/NewUI/NewUISystem.h"

namespace SEASON3B
{
    bool PersonalShopOfflineDialog::SetLayout()
    {
        auto *box = GetMsgBox();
        if (!box || !box->Create(MSGBOX_COMMON_TYPE_OKCANCEL))
            return false;
        box->EnableGoldStyle();
        box->AddMsg(L"¿Activar tienda offline y desconectar al personaje?");
        box->AddCallbackFunc(Confirm, MSGBOX_EVENT_USER_COMMON_OK);
        box->AddCallbackFunc(Confirm, MSGBOX_EVENT_PRESSKEY_RETURN);
        box->AddCallbackFunc(Cancel, MSGBOX_EVENT_USER_COMMON_CANCEL);
        box->AddCallbackFunc(Cancel, MSGBOX_EVENT_PRESSKEY_ESC);
        return true;
    }

    CALLBACK_RESULT PersonalShopOfflineDialog::Confirm(CNewUIMessageBoxBase *owner, const leaf::xstreambuf &parameters)
    {
        if (Network::Paris::PersonalShopActions::RequestOfflineStore())
        {
            g_pNewUISystem->Hide(INTERFACE_MYSHOP_INVENTORY);
            g_pNewUISystem->Hide(INTERFACE_INVENTORY);
            g_pNewUISystem->Hide(INTERFACE_INVENTORY_EXT);
        }
        return Cancel(owner, parameters);
    }

    CALLBACK_RESULT PersonalShopOfflineDialog::Cancel(CNewUIMessageBoxBase *owner, const leaf::xstreambuf &)
    {
        PlayBuffer(SOUND_CLICK01);
        g_MessageBox->SendEvent(owner, MSGBOX_EVENT_DESTROY);
        return CALLBACK_BREAK;
    }
} // namespace SEASON3B
