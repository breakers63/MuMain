#pragma once

#include "Data/GameData/ItemData/ItemStructs.h"
#include "Core/Globals/_define.h"

namespace GameLogic::Items::ColorPotions
{
// Shared catalog contract with ParisColorPotions.cs on the server.
inline constexpr int FirstNumber = 450;
inline constexpr int ChannelCount = 3;
inline constexpr int FirstMix = 240;

inline int Channel(int itemType)
{
    const int channel = itemType - (ITEM_POTION + FirstNumber);
    return channel >= 0 && channel < ChannelCount ? channel : -1;
}

inline void ApplyDefinition(int itemType, ITEM_ATTRIBUTE& item)
{
    const int channel = Channel(itemType);
    if (channel < 0) return;
    constexpr const wchar_t* Names[] = { L"Pocion de color roja", L"Pocion de color verde", L"Pocion de color azul" };
    // Color potions are crafting reagents, not healing or mana consumables.
    item = {};
    wcscpy_s(item.Name, Names[channel]);
    item.Width = item.Height = 1;
    item.m_byItemSlot = 0xFF;
    item.Durability = 1;
}

inline void ApplyPreviewLight(int modelType, float* light)
{
    const int channel = modelType - (MODEL_POTION + FirstNumber);
    if (channel < 0 || channel >= ChannelCount) return;
    constexpr float MinimumComponent = 0.12f;
    for (int component = 0; component < ChannelCount; ++component)
        light[component] = component == channel ? 1.f : MinimumComponent;
}
}
