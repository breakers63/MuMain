#pragma once
#include "Engine/Object/ZzzInventory.h"
namespace Render::Items::ParisPreview
{
float Scale();
bool Centered();
}
void RenderMarketItemInfo(int left, int top, ITEM* item);
void RenderMarketPreviewItem3D(float x, float y, float width, float height,
    int type, int level, int excellent, int ancient, float scale);
