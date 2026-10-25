#include "stdafx.h"
#include "PegasusSky.h"
#include "SkyPalette.h"
#include "WeatherSettings.h"
#include "WeatherRenderer.h"
#include "Camera/CameraState.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/RenderUtils.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>

namespace Render::Weather
{
namespace
{
constexpr int FaceCount = 6, StarCount = 80;
std::array<GLuint, FaceCount> textures{};
std::array<mu::Vertex3D, StarCount * 4> starVertices{};
int loadedMap = -1;
bool attempted = false;

void LoadSky(int map)
{
    if (loadedMap == map && attempted)
        return;
    for (auto& texture : textures)
    {
        if (texture != 0 && texture != BITMAP_UNKNOWN)
            Bitmaps.UnloadImage(texture, true);
        texture = 0;
    }
    loadedMap = map;
    attempted = true;
    constexpr const wchar_t* Names[] = {L"Back", L"Bottom", L"Front", L"Left", L"Right", L"Top"};
    const auto folder = std::filesystem::path(L"Data/Effect/ParisSky") / (L"World" + std::to_wstring(map + 1));
    for (int face = 0; face < FaceCount; ++face)
    {
        if (!std::filesystem::exists(folder / (std::wstring(Names[face]) + L".OZT")))
            continue;
        textures[face] = Bitmaps.LoadImageExclusive((folder / (std::wstring(Names[face]) + L".tga")).wstring(),
                                                   GL_LINEAR, GL_CLAMP_TO_EDGE, false, false);
    }
}

std::array<float, 3> CameraCenter(const float view[16])
{
    std::array<float, 3> center{};
    for (int axis = 0; axis < 3; ++axis)
        center[axis] = -view[12] * view[axis * 4] - view[13] * view[axis * 4 + 1] - view[14] * view[axis * 4 + 2];
    return center;
}

void DrawFaces(const std::array<float, 3>& center, const SkyPalette& palette)
{
    // Pegasus face order and UV orientation. OZT texels are top-down on SDL_gpu.
    constexpr float Faces[6][4][5] = {
        {{-1,-1,1,0,1},{1,-1,1,1,1},{1,-1,-1,1,0},{-1,-1,-1,0,0}},
        {{-1,1,-1,1,1},{1,1,-1,0,1},{1,-1,-1,0,0},{-1,-1,-1,1,0}},
        {{-1,1,1,1,1},{1,1,1,0,1},{1,1,-1,0,0},{-1,1,-1,1,0}},
        {{-1,-1,1,1,1},{-1,1,1,0,1},{-1,1,-1,0,0},{-1,-1,-1,1,0}},
        {{1,-1,1,0,1},{1,1,1,1,1},{1,1,-1,1,0},{1,-1,-1,0,0}},
        {{-1,1,1,1,0},{1,1,1,0,0},{1,-1,1,0,1},{-1,-1,1,1,1}}};
    const bool complete = std::all_of(textures.begin(), textures.end(), [](GLuint id) { return id != 0 && id != BITMAP_UNKNOWN; });
    const float halfSize = std::max(400.f, g_Camera.ViewFar * .55f);
    mu::GetRenderer().SetTexture2D(complete);
    for (int face = 0; face < FaceCount; ++face)
    {
        std::array<mu::Vertex3D, 4> quad{};
        for (int vertex = 0; vertex < 4; ++vertex)
        {
            const auto& corner = Faces[face][vertex];
            const auto& rgb = complete ? palette.textureTint : (corner[2] > 0.f ? palette.top : palette.horizon);
            quad[vertex] = {center[0] + corner[0] * halfSize, center[1] + corner[1] * halfSize,
                            center[2] + halfSize * (corner[2] > 0.f ? .5f : -.116667f),
                            0, 0, 1, corner[3], 1.f - corner[4], mu::PackABGR(rgb[0], rgb[1], rgb[2], 1.f)};
        }
        mu::GetRenderer().RenderQuad3D(quad, complete ? textures[face] : 0);
    }
}

float StarRandom(unsigned seed)
{
    seed ^= seed >> 16;
    seed *= 0x7feb352dU;
    seed ^= seed >> 15;
    return (seed & 0xFFFFFFU) / 16777215.f;
}

void DrawStars(const float view[16], const std::array<float, 3>& center, const SkyPalette& palette)
{
    if (!Options().stars || palette.stars < .01f)
        return;
    EnableAlphaBlend3();
    mu::GetRenderer().SetTexture2D(false);
    mu::GetRenderer().SetFogEnabled(false);
    const float radius = std::max(200.f, g_Camera.ViewFar * .20f);
    constexpr float Tau = 6.2831853f;
    constexpr float Corners[4][2] = {{-1,-1},{1,-1},{1,1},{-1,1}};
    const float seconds = static_cast<float>(ElapsedMs() % 60000) * .001f;
    for (int star = 0; star < StarCount; ++star)
    {
        const float azimuth = StarRandom(star + 71) * Tau, altitude = .15f + StarRandom(star + 171) * 1.35f;
        const float x = center[0] + radius * std::cos(altitude) * std::cos(azimuth);
        const float y = center[1] + radius * std::cos(altitude) * std::sin(azimuth);
        const float z = center[2] + radius * std::sin(altitude);
        const float size = .8f + StarRandom(star + 271) * 1.5f;
        const float opacity = palette.stars * (.78f + .22f * std::sin(seconds + azimuth));
        for (int corner = 0; corner < 4; ++corner)
        {
            auto& vertex = starVertices[star * 4 + corner];
            vertex = {x, y, z, 0, 0, 1, 0, 0, mu::PackABGR(.85f, .9f, 1.f, opacity)};
            vertex.x += size * (view[0] * Corners[corner][0] + view[1] * Corners[corner][1]);
            vertex.y += size * (view[4] * Corners[corner][0] + view[5] * Corners[corner][1]);
            vertex.z += size * (view[8] * Corners[corner][0] + view[9] * Corners[corner][1]);
        }
    }
    mu::GetRenderer().RenderQuad3D(starVertices, 0);
    DisableAlphaBlend();
}
}

void DrawPegasusSky(int map)
{
    if (!Options().sky || !Current().outdoor)
        return;
    LoadSky(map);
    auto& renderer = mu::GetRenderer();
    float view[16]{};
    renderer.GetMatrix(GL_MODELVIEW_MATRIX, view);
    const auto center = CameraCenter(view);
    const auto palette = SampleSkyPalette(SkyHour(Options(), ElapsedMs()), Current(), map);
    const mu::ScopedBloomExclusion noBloom;
    DisableAlphaBlend();
    DisableDepthTest();
    DisableDepthMask();
    DisableCullFace();
    renderer.SetFogEnabled(false);
    DrawFaces(center, palette);
    DrawStars(view, center, palette);
    EnableTexture2D();
    EnableDepthTest();
    EnableDepthMask();
    EnableCullFace();
    renderer.SetFogEnabled(FogEnable || WantsFog());
}
}
