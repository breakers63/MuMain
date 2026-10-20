#include "stdafx.h"
#include "Render/Models/PosedModelBounds.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Sprites/GlobalBitmap.h"
#include <algorithm>
#include <cmath>

namespace Render::PreviewBounds
{
namespace
{
bool VisibleMesh(const BMD& model, const Mesh_t& mesh, bool excludeSkinHair)
{
    if (!model.IndexTexture || mesh.Texture < 0 || mesh.Texture >= model.NumMeshs) return true;
    const auto index = model.IndexTexture[mesh.Texture];
    if (index == BITMAP_HIDE) return false;
    if (!excludeSkinHair) return true;
    const auto* texture = Bitmaps.GetTexture(index);
    return !texture || (!texture->IsSkin && !texture->IsHair);
}
}

bool AccumulatePosedBounds(const BMD& model, const float (*transforms)[3][4],
                          float minimum[3], float maximum[3], int hiddenMesh, bool excludeSkinHair)
{
    if (!model.Meshs || !transforms) return false;
    bool found = false;
    for (int meshIndex = 0; meshIndex < std::min<int>(model.NumMeshs, MAX_MESH); ++meshIndex)
    {
        const auto& mesh = model.Meshs[meshIndex];
        if (meshIndex == hiddenMesh || !mesh.Vertices || !VisibleMesh(model, mesh, excludeSkinHair)) continue;
        for (int index = 0; index < std::min<int>(mesh.NumVertices, MAX_VERTICES); ++index)
        {
            const auto& vertex = mesh.Vertices[index];
            if (vertex.Node < 0 || vertex.Node >= model.NumBones || vertex.Node >= MAX_BONES) continue;
            vec3_t point;
            VectorTransform(vertex.Position, transforms[vertex.Node], point);
            if (!std::isfinite(point[0]) || !std::isfinite(point[1]) || !std::isfinite(point[2])) continue;
            for (int axis = 0; axis < 3; ++axis)
            {
                minimum[axis] = std::min(minimum[axis], point[axis]);
                maximum[axis] = std::max(maximum[axis], point[axis]);
            }
            found = true;
        }
    }
    return found;
}
}
