#pragma once

namespace Rml { class Element; }

namespace UI::RmlBridge
{
// Two permanently typed inputs share one data-model value. Visibility changes never
// recreate either input's widget or replace its real text with display asterisks.
void SetPasswordVisible(Rml::Element* maskedField, Rml::Element* visibleField, bool visible);
}
