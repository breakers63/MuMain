#include "stdafx.h"
#include "Render/Items/ItemPreviewFit.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Models/PosedModelBounds.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Camera/CameraProjection.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Render::Items::Preview
{
namespace
{
thread_local std::optional<std::array<float, 4>> rectangle;
constexpr float ModelOccupancy = .82f, BootsOccupancy = .72f;
constexpr int FitIterations = 3;

bool ProjectBounds(const OBJECT& object, const vec3_t minimum, const vec3_t maximum, std::array<float, 4>& bounds)
{
    constexpr float Infinity = std::numeric_limits<float>::infinity();
    bounds = {Infinity, Infinity, -Infinity, -Infinity};
    for (int corner = 0; corner < 8; ++corner)
    {
        vec3_t point;
        for (int axis = 0; axis < 3; ++axis)
            point[axis] = object.Position[axis] + object.Scale * ((corner & (1 << axis)) ? maximum[axis] : minimum[axis]);
        float x, y;
        if (!CameraProjection::WorldToWindowPixel(g_Camera, point, &x, &y)) return false;
        bounds[0] = std::min(bounds[0], x); bounds[1] = std::min(bounds[1], y);
        bounds[2] = std::max(bounds[2], x); bounds[3] = std::max(bounds[3], y);
    }
    return bounds[2] - bounds[0] > .001f && bounds[3] - bounds[1] > .001f;
}

void CenterModel(OBJECT& object, const vec3_t minimum, const vec3_t maximum, const std::array<float, 4>& bounds)
{
    vec3_t center, view;
    for (int axis = 0; axis < 3; ++axis) center[axis] = object.Position[axis] + object.Scale * (minimum[axis] + maximum[axis]) * .5f;
    VectorTransform(center, g_Camera.Matrix, view);
    const float errorX = (*rectangle)[0] + (*rectangle)[2] * .5f - (bounds[0] + bounds[2]) * .5f;
    const float errorY = (*rectangle)[1] + (*rectangle)[3] * .5f - (bounds[1] + bounds[3]) * .5f;
    vec3_t shift{-view[2] * g_Camera.PerspectiveX * errorX, view[2] * g_Camera.PerspectiveY * errorY, 0.f}, worldShift;
    VectorIRotate(shift, g_Camera.Matrix, worldShift);
    VectorAdd(object.Position, worldShift, object.Position);
}
}

ScopedFit::ScopedFit(float x, float y, float width, float height) : m_previous(rectangle)
{
    rectangle = std::array{x, y, width, height};
}
ScopedFit::~ScopedFit() { rectangle = m_previous; }

void FitPosedModel(BMD* model, OBJECT* object)
{
    if (!rectangle || !model || !model->Meshs || !object) return;
    constexpr float Infinity = std::numeric_limits<float>::infinity();
    vec3_t minimum{Infinity, Infinity, Infinity}, maximum{-Infinity, -Infinity, -Infinity};
    if (!Render::PreviewBounds::AccumulatePosedBounds(*model, BoneTransform, minimum, maximum, -1, true)) return;
    const float occupancy = (object->Type - MODEL_ITEM) / MAX_ITEM_INDEX == 8 ? BootsOccupancy : ModelOccupancy;
    for (int iteration = 0; iteration < FitIterations; ++iteration)
    {
        std::array<float, 4> bounds;
        if (!ProjectBounds(*object, minimum, maximum, bounds)) return;
        CenterModel(*object, minimum, maximum, bounds);
        const float ratio = std::min((*rectangle)[2] * occupancy / (bounds[2] - bounds[0]), (*rectangle)[3] * occupancy / (bounds[3] - bounds[1]));
        if (!std::isfinite(ratio) || ratio <= 0.f) return;
        object->Scale *= std::clamp(ratio, .05f, 20.f);
    }
}
}
