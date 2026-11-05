#pragma once
struct tagITEM;
namespace UI::RmlBridge::Tooltip { struct Config; }
namespace UI::Items::RarityTooltip
{
void Decorate(UI::RmlBridge::Tooltip::Config& tooltip, const tagITEM& item);
}
