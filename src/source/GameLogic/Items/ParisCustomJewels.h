#pragma once
#include "Data/GameData/ItemData/ItemStructs.h"
namespace GameLogic::Items::CustomJewels {
inline bool IsJewel(int type) {return type>=ITEM_POTION+244 && type<=ITEM_POTION+252;}
inline void ApplyDefinition(int type,ITEM_ATTRIBUTE& item) {
 if(!IsJewel(type))return;
 constexpr const wchar_t* names[]={L"Jewel of Level",L"Jewel of Skill",L"Jewel of Luck",L"Jewel of Additional",L"Jewel of Excellent",L"Jewel of Socket",L"Jewel of Ancient I",L"Jewel of Ancient II",L"Jewel of Full"};
 item={};wcscpy_s(item.Name,names[type-ITEM_POTION-244]);item.Width=item.Height=1;item.m_byItemSlot=0xff;item.Durability=1;
}
}
