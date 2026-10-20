#pragma once
#include "UI/NewUI/Character/ClassicPanelFrame.h"
namespace UI::Theme
{
enum class Symbol { Close, Shop, StopShop, Offline, Repair, Expand, Info, Ranking, Vip };
void HoverTip(int x,int y,int w,int h,const wchar_t* text);
void Bevel(int x, int y, int w, int h, const wchar_t* text, bool active = false, bool enabled = true);
void IconButton(int x, int y, int w, int h, Symbol symbol, bool enabled=true);
void Check(int x, int y, bool checked);
}
