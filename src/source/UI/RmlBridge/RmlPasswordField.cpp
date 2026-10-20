#include "UI/RmlBridge/RmlPasswordField.h"

#include <RmlUi/Core/Element.h>

namespace UI::RmlBridge
{
void SetPasswordVisible(Rml::Element* maskedField, Rml::Element* visibleField, bool visible)
{
    if (!maskedField || !visibleField)
        return;
    maskedField->SetClass("password-hidden", visible);
    visibleField->SetClass("password-hidden", !visible);
}
}
