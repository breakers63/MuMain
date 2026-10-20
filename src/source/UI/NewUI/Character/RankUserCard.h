#pragma once
#include "RankUserCardData.h"
namespace UI::Character::RankCard {
    void Load();
    void Unload();
    void Render(const RankUserCardData& data,int x,int y,int width,int height);
}
