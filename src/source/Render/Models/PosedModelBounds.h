#pragma once
class BMD;

namespace Render::PreviewBounds
{
bool AccumulatePosedBounds(const BMD& model, const float (*transforms)[3][4],
                          float minimum[3], float maximum[3], int hiddenMesh = -1, bool excludeSkinHair = false);
}
