#include "stdafx.h"
#include "SnowSurface.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/RenderUtils.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include <algorithm>
#include <cmath>
#include <vector>

extern vec3_t TerrainNormal[];

namespace Render::Weather::SnowSurface
{
namespace
{
constexpr float AccumulationSeconds = 7.f, MeltSeconds = 180.f;
constexpr float TrackSpacing = 42.f, TrackLifetime = 34.f;
constexpr float TrackWidth = 18.f, TrackLength = 30.f;
constexpr float GroundOffset = 1.4f, TrackOffset = 2.2f;
constexpr int CoverRadius = 20, TrackSegments = 14;
constexpr std::size_t MaximumTracks = 320;
struct Footprint
{
    float x, y, angle;
    std::uint64_t born;
};
std::vector<Footprint> tracks;
std::vector<mu::Vertex3D> vertices;
int world = -1;
float cover = 0.f, snowfall = 0.f, anchorX = 0.f, anchorY = 0.f;
bool haveAnchor = false, nextFoot = false;

float BaseCover()
{
    if (world == 2) return .34f;
    if (world == 57 || world == 58) return .42f;
    if (world == 62) return .48f;
    return 0.f;
}

float SmoothStep(float low, float high, float value)
{
    const float t = std::clamp((value - low) / (high - low), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float TileVariation(int x, int y)
{
    unsigned value = static_cast<unsigned>(x) * 73856093U ^ static_cast<unsigned>(y) * 19349663U;
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return .76f + (value & 0xFFFFFFU) / 16777215.f * .32f;
}

void Vertex(float x, float y, float offset, std::uint32_t color)
{
    vertices.push_back({x, y, RequestTerrainHeight(x, y) + offset, 0, 0, 1, 0, 0, color});
}

void SnowTile(int x, int y)
{
    if (x < 0 || y < 0 || x >= TERRAIN_SIZE_MASK || y >= TERRAIN_SIZE_MASK ||
        !IsExposed(x * TERRAIN_SCALE, y * TERRAIN_SCALE))
        return;
    const int index = TERRAIN_INDEX(x, y);
    if ((TerrainWall[index] & TW_NOGROUND) != 0)
        return;
    const float mask = std::clamp(cover * SmoothStep(.26f, .82f, TerrainNormal[index][2]) * TileVariation(x, y), 0.f, 1.f);
    if (mask < .025f)
        return;
    constexpr float SnowOpacity = .82f;
    const auto color = mu::PackABGR(.96f, .98f, 1.f, SnowOpacity * mask);
    const float px = x * TERRAIN_SCALE, py = y * TERRAIN_SCALE;
    Vertex(px, py, GroundOffset, color);
    Vertex(px + TERRAIN_SCALE, py, GroundOffset, color);
    Vertex(px + TERRAIN_SCALE, py + TERRAIN_SCALE, GroundOffset, color);
    Vertex(px, py + TERRAIN_SCALE, GroundOffset, color);
}

void DrawCover()
{
    if (cover < .02f)
        return;
    vertices.clear();
    const int cx = static_cast<int>(Hero->Object.Position[0] / TERRAIN_SCALE);
    const int cy = static_cast<int>(Hero->Object.Position[1] / TERRAIN_SCALE);
    for (int y = -CoverRadius; y <= CoverRadius; ++y)
        for (int x = -CoverRadius; x <= CoverRadius; ++x)
            if (x * x + y * y <= CoverRadius * CoverRadius)
                SnowTile(cx + x, cy + y);
    if (!vertices.empty())
        mu::GetRenderer().RenderQuad3D(vertices, 0);
}

float TrackLife()
{
    return TrackLifetime / (1.f + snowfall * 2.5f);
}

void UpdateTracks()
{
    const auto now = GetTickCount64();
    std::erase_if(tracks, [&](const Footprint& track) { return (now - track.born) * .001f >= TrackLife(); });
    const float x = Hero->Object.Position[0], y = Hero->Object.Position[1];
    if (!haveAnchor)
    {
        anchorX = x;
        anchorY = y;
        haveAnchor = true;
        return;
    }
    const float distance = std::hypot(x - anchorX, y - anchorY);
    if (distance < TrackSpacing)
        return;
    anchorX = x;
    anchorY = y;
    constexpr float TeleportDistance = 500.f, MaximumGroundClearance = 85.f;
    if (distance > TeleportDistance || cover < .12f || !IsExposed(x, y) ||
        std::fabs(Hero->Object.Position[2] - RequestTerrainHeight(x, y)) > MaximumGroundClearance)
        return;
    const float angle = Hero->Object.Angle[2] * Q_PI / 180.f;
    const float side = nextFoot ? 1.f : -1.f;
    nextFoot = !nextFoot;
    if (tracks.size() == MaximumTracks)
        tracks.erase(tracks.begin());
    tracks.push_back({x + std::cos(angle) * side * TrackWidth * .48f,
                      y + std::sin(angle) * side * TrackWidth * .48f, angle, now});
}

void TrackEdge(const Footprint& track, float theta, std::uint32_t color)
{
    const float sn = std::sin(theta);
    const float localX = std::cos(theta) * TrackWidth * .5f * (sn > 0.f ? 1.08f : .82f);
    const float localY = sn * TrackLength * .5f;
    Vertex(track.x + std::cos(track.angle) * localX + std::sin(track.angle) * localY,
           track.y + std::sin(track.angle) * localX - std::cos(track.angle) * localY, TrackOffset, color);
}

void DrawTracks()
{
    vertices.clear();
    const auto now = GetTickCount64();
    for (const auto& track : tracks)
    {
        const float fade = std::clamp(1.f - (now - track.born) * .001f / TrackLife(), 0.f, 1.f);
        const auto color = mu::PackABGR(.18f, .22f, .28f, .50f * fade * std::clamp(cover * 1.4f, 0.f, 1.f));
        for (int i = 0; i < TrackSegments; ++i)
        {
            Vertex(track.x, track.y, TrackOffset, color);
            TrackEdge(track, i * Q_PI * 2.f / TrackSegments, color);
            TrackEdge(track, (i + 1) * Q_PI * 2.f / TrackSegments, color);
        }
    }
    if (!vertices.empty())
        mu::GetRenderer().RenderTriangles(vertices, 0);
}
} // namespace

bool IsExposed(float x, float y)
{
    const int tx = static_cast<int>(x / TERRAIN_SCALE), ty = static_cast<int>(y / TERRAIN_SCALE);
    if (tx < 0 || ty < 0 || tx >= TERRAIN_SIZE || ty >= TERRAIN_SIZE)
        return false;
    if (world == 2)
    {
        const auto layer = TerrainMappingLayer1[TERRAIN_INDEX(tx, ty)];
        return layer != 3 && layer < 10;
    }
    if (world != 0)
        return true;
    constexpr int roofX[] = {119, 128, 129, 120}, roofY[] = {119, 123, 137, 137};
    bool inside = true;
    for (int i = 0; i < 4; ++i)
    {
        const int next = (i + 1) % 4;
        if ((roofX[next] - roofX[i]) * (ty - roofY[i]) - (roofY[next] - roofY[i]) * (tx - roofX[i]) < 0)
            inside = false;
    }
    const bool secondRoof = (tx >= 138 && tx <= 141 && ty >= 142 && ty <= 154) ||
                            (tx == 142 && ty >= 144 && ty <= 153);
    return !inside && !secondRoof;
}

void Reset(int map)
{
    world = map;
    cover = BaseCover();
    snowfall = 0.f;
    tracks.clear();
    tracks.reserve(MaximumTracks);
    vertices.reserve(std::max(MaximumTracks * TrackSegments * 3, std::size_t((CoverRadius * 2 + 1) * (CoverRadius * 2 + 1) * 4)));
    haveAnchor = false;
    nextFoot = false;
}

void Update(float seconds, float amount)
{
    snowfall = std::clamp(amount, 0.f, 1.f);
    const float base = BaseCover();
    const float target = std::clamp(base + (1.f - base) * snowfall, 0.f, 1.f);
    const float time = target > cover ? AccumulationSeconds : MeltSeconds;
    cover += (target - cover) * std::clamp(seconds / time, 0.f, 1.f);
    if (Hero)
        UpdateTracks();
}

float Cover()
{
    return cover;
}

void Draw()
{
    if (!Hero || cover < .001f)
        return;
    auto& renderer = mu::GetRenderer();
    renderer.SetTexture2D(false);
    DrawCover();
    DrawTracks();
    renderer.SetTexture2D(true);
}
} // namespace Render::Weather::SnowSurface
