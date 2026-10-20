#include "stdafx.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"
namespace
{
thread_local float PreviewScale = 1.f;
thread_local bool PreviewCentered = false;
}
namespace Render::Items::ParisPreview
{
float Scale() { return PreviewScale; }
bool Centered() { return PreviewCentered; }
}
void RenderMarketItemInfo(int left, int top, ITEM* item)
{
    if (item) RenderItemInfo(left, top, item, false);
}
void RenderMarketPreviewItem3D(float x, float y, float width, float height,
    int type, int level, int excellent, int ancient, float scale)
{
    const float previousScale = PreviewScale;
    const bool previousCentered = PreviewCentered;
    PreviewScale = scale;
    PreviewCentered = true;
    RenderItem3D(x, y, width, height, type, level, excellent, ancient, false);
    PreviewScale = previousScale;
    PreviewCentered = previousCentered;
}
