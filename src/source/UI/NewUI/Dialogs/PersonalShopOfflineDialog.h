#pragma once
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"

namespace SEASON3B
{
    class PersonalShopOfflineDialog : public TMsgBoxLayout<CNewUICommonMessageBox>
    {
      public:
        bool SetLayout();
        static CALLBACK_RESULT Confirm(CNewUIMessageBoxBase *owner, const leaf::xstreambuf &parameters);
        static CALLBACK_RESULT Cancel(CNewUIMessageBoxBase *owner, const leaf::xstreambuf &parameters);
    };
} // namespace SEASON3B
