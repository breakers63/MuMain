#pragma once
namespace GameLogic::Items::JewelryUpgrades {
inline bool IsRing(int type) {
    return type == ITEM_HELPER + 8 || type == ITEM_HELPER + 9
        || (type >= ITEM_HELPER + 21 && type <= ITEM_HELPER + 24);
}
inline bool IsJewelry(int type) {
    return IsRing(type) || type == ITEM_HELPER + 12 || type == ITEM_HELPER + 13
        || (type >= ITEM_HELPER + 25 && type <= ITEM_HELPER + 28);
}
inline bool IsRestrictedJewel(int type) { return type == ITEM_POTION + 247 || type == ITEM_POTION + 248; }
}
