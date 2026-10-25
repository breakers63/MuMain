#include "stdafx.h"
#include "WeatherRenderer.h"
#include "WeatherSettings.h"
#include "WeatherAudio.h"
#include "SnowSurface.h"
#include "PuddleSurface.h"
#include "SnowParticle.h"
#include "PegasusSky.h"
#include "Camera/CameraState.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/RenderUtils.h"
#include "Render/Renderer/SceneLighting.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "World/MapInfra/MapManager.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <vector>
extern unsigned int WindowWidth, WindowHeight;
namespace Render::Weather
{
namespace
{
State current;
int currentMap = -1;
std::uint64_t lastTick = 0;
float wetness = 0;
std::array<GLuint, 6> textures{};
std::vector<mu::Vertex3D> vertices;
float Hash(unsigned value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    return (value & 0xFFFFFF) / 16777215.f;
}
float Wrap(float value, float range)
{
    return value - std::floor(value / range) * range;
}
void LoadTextures()
{
    constexpr const wchar_t* files[] = {L"rain", L"snow", L"splash", L"white", L"footprint", L"mist"};
    for (std::size_t i = 0; i < textures.size(); ++i)
        if (textures[i] == 0 || (textures[i] != BITMAP_UNKNOWN && !Bitmaps.FindTexture(textures[i])))
        {
            const auto path = std::wstring(L"Data/Effect/ParisWeather/") + files[i] + L".tga";
            textures[i] = Bitmaps.LoadImageExclusive(path, GL_LINEAR, GL_CLAMP_TO_EDGE, false, false);
        }
}
void Quad(float x, float y, float z, const float right[3], const float up[3], float w, float h, std::uint32_t color)
{
    constexpr float signs[4][4] = {{-1, -1, 0, 1}, {1, -1, 1, 1}, {1, 1, 1, 0}, {-1, 1, 0, 0}};
    for (const auto& s : signs)
        vertices.push_back({x + right[0] * w * s[0] + up[0] * h * s[1], y + right[1] * w * s[0] + up[1] * h * s[1],
                            z + right[2] * w * s[0] + up[2] * h * s[1], 0, 0, 1, s[2], s[3], color});
}
void Submit(int texture)
{
    if (!vertices.empty() && textures[texture] != BITMAP_UNKNOWN)
        mu::GetRenderer().RenderQuad3D(vertices, textures[texture]);
    vertices.clear();
}
void SnowViewBasis(float right[3], float up[3])
{
    float view[16]{};
    mu::GetRenderer().GetMatrix(GL_MODELVIEW_MATRIX, view);
    for (int axis = 0; axis < 3; ++axis)
    {
        right[axis] = view[axis * 4];
        up[axis] = view[axis * 4 + 1];
    }
    const auto normalize = [](float vector[3])
    {
        const float length = std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
        if (length > .0001f)
            for (int axis = 0; axis < 3; ++axis)
                vector[axis] /= length;
    };
    normalize(right);
    normalize(up);
}
void SnowParticles()
{
    if (current.snow < .01f)
        return;
    const bool blizzard = current.kind == Kind::Blizzard;
    constexpr float QualityDensity[] = {.75f, 1.f, 1.f};
    const bool nativeMap = currentMap == 2 || currentMap == 57 || currentMap == 58 || currentMap == 62;
    const float density = QualityDensity[std::clamp(Options().quality, 0, 2)] * (nativeMap ? 2.2f : 1.f);
    const int count = std::min(2200, static_cast<int>((blizzard ? 1050 : 650) * density * current.snow));
    const auto now = GetTickCount64();
    float right[3], up[3];
    SnowViewBasis(right, up);
    vertices.reserve(static_cast<std::size_t>(count) * 4);
    for (int i = 0; i < count; ++i)
    {
        const auto flake = SampleSnowParticle(i, now, blizzard, current.snow, blizzard ? current.wind : .30f);
        const float x = Hero->Object.Position[0] + flake.x, y = Hero->Object.Position[1] + flake.y;
        if (!SnowSurface::IsExposed(x, y))
            continue;
        const float cs = std::cos(flake.rotation), sn = std::sin(flake.rotation);
        float rotatedRight[3], rotatedUp[3];
        for (int axis = 0; axis < 3; ++axis)
        {
            rotatedRight[axis] = right[axis] * cs + up[axis] * sn;
            rotatedUp[axis] = -right[axis] * sn + up[axis] * cs;
        }
        const auto color = mu::PackABGR(.94f, .98f, 1.f, flake.opacity);
        Quad(x, y, Hero->Object.Position[2] + flake.z, rotatedRight, rotatedUp,
             flake.size * (blizzard ? 1.10f : 1.f), flake.size * (blizzard ? 1.35f : 1.f), color);
    }
    Submit(1);
}
void RainDrop(unsigned index, double seconds, bool storm, const float forward[3])
{
    constexpr float VerticalRange = 1250.f;
    const float variation = SnowHash(index * 31U + 211U);
    const float travel = static_cast<float>(std::fmod(seconds * (storm ? 1150.f : 900.f) +
                                    SnowHash(index * 7U + 131U) * VerticalRange, VerticalRange));
    const float drift = travel / VerticalRange, radius = storm ? 1250.f : 1100.f;
    const float wind = storm ? current.wind : .30f;
    const float x = Hero->Object.Position[0] + (SnowHash(index * 3U + 17U) * 2.f - 1.f) * radius +
                    wind * (storm ? 255.f : 165.f) * drift;
    const float y = Hero->Object.Position[1] + (SnowHash(index * 5U + 71U) * 2.f - 1.f) * radius + wind * 90.f * drift;
    if (!SnowSurface::IsExposed(x, y))
        return;
    const float segment[3] = {-wind * (storm ? 42.f : 30.f), -wind * (storm ? 14.f : 8.f),
                              (storm ? 38.f : 27.f) * (.72f + variation * .55f)};
    float side[3] = {segment[1] * forward[2] - segment[2] * forward[1],
                     segment[2] * forward[0] - segment[0] * forward[2],
                     segment[0] * forward[1] - segment[1] * forward[0]};
    const float length = std::sqrt(side[0] * side[0] + side[1] * side[1] + side[2] * side[2]);
    if (length < .0001f)
        return;
    float halfSegment[3];
    for (int axis = 0; axis < 3; ++axis)
    {
        side[axis] /= length;
        halfSegment[axis] = segment[axis] * .5f;
    }
    const float width = 1.10f * .95f * (storm ? 1.10f : 1.f) * (.82f + variation * .24f);
    const auto color = mu::PackABGR(.86f, .93f, 1.f, (storm ? 1.f : .96f) * current.rain * (.78f + variation * .22f));
    Quad(x + halfSegment[0], y + halfSegment[1],
         Hero->Object.Position[2] - 120.f + VerticalRange - travel + halfSegment[2], side, halfSegment, width, 1.f, color);
}
void RainParticles()
{
    if (current.rain < .01f)
        return;
    const bool storm = current.kind == Kind::Storm;
    constexpr float QualityDensity[] = {.75f, 1.f, 1.35f};
    const int count = static_cast<int>((storm ? 2000 : 1650) * current.rain * QualityDensity[std::clamp(Options().quality, 0, 2)]);
    float view[16]{};
    mu::GetRenderer().GetMatrix(GL_MODELVIEW_MATRIX, view);
    const float forward[3] = {-view[2], -view[6], -view[10]};
    const double seconds = GetTickCount64() * .001;
    vertices.reserve(static_cast<std::size_t>(count) * 4);
    for (int i = 0; i < count; ++i)
        RainDrop(i, seconds, storm, forward);
    Submit(0);
}
void GroundQuad(float x, float y, float size, float angle, std::uint32_t color)
{
    const float cs = std::cos(angle), sn = std::sin(angle);
    constexpr float corners[4][4] = {{-1, -1, 0, 1}, {1, -1, 1, 1}, {1, 1, 1, 0}, {-1, 1, 0, 0}};
    for (const auto& c : corners)
    {
        const float px = x + size * (cs * c[0] - sn * c[1]), py = y + size * (sn * c[0] + cs * c[1]);
        vertices.push_back({px, py, RequestTerrainHeight(px, py) + 1.4f, 0, 0, 1, c[2], c[3], color});
    }
}
void Surface()
{
    const float x = Hero->Object.Position[0], y = Hero->Object.Position[1];
    if (current.rain > .02f)
    {
        const float seconds = static_cast<float>(ElapsedMs() % 3600000) / 1000.f;
        for (unsigned i = 0; i < 96; ++i)
        {
            const float phase = Wrap(seconds * 1.7f + Hash(i + 300), 1);
            const float sx = x + (Hash(i + 701) - .5f) * 1700, sy = y + (Hash(i + 901) - .5f) * 1700;
            GroundQuad(sx, sy, 3 + phase * 12, 0, mu::PackABGR(.7f, .82f, 1.f, (1 - phase) * current.rain * .3f));
        }
        Submit(2);
    }
    SnowSurface::Draw();
}
void Overlay(float r, float g, float b, float alpha)
{
    if (alpha < .001f)
        return;
    auto& renderer = mu::GetRenderer();
    renderer.Begin2DPass();
    renderer.SetDepthTest(false);
    renderer.SetDepthMask(false);
    renderer.SetTexture2D(false);
    renderer.SetBlendMode(mu::BlendMode::Alpha);
    const auto color = mu::PackABGR(r, g, b, alpha);
    const float w = static_cast<float>(WindowWidth), h = static_cast<float>(WindowHeight);
    const mu::Vertex2D quad[] = {{0, 0, 0, 0, color}, {w, 0, 1, 0, color}, {w, h, 1, 1, color}, {0, h, 0, 1, color}};
    renderer.RenderQuad2D(quad, 0);
    renderer.SetTexture2D(true);
    renderer.SetDepthMask(true);
    renderer.End2DPass();
}
} // namespace
const State& Current()
{
    return current;
}
void SuspendWorld()
{
    StopAudio();
    current = {};
    currentMap = -1;
    lastTick = 0;
    SnowSurface::Reset(-1);
}
void UpdateWorld(int map)
{
    const auto now = GetTickCount64();
    const float dt = lastTick ? std::clamp(static_cast<float>(now - lastTick) / 1000.f, 0.f, .1f) : 0.f;
    lastTick = now;
    if (map != currentMap)
    {
        currentMap = map;
        wetness = 0;
        SnowSurface::Reset(map);
        current = {};
    }
    const bool exposed = Hero && SnowSurface::IsExposed(Hero->Object.Position[0], Hero->Object.Position[1]);
    const auto target = Evaluate(Options(), map, ElapsedMs(), exposed);
    const float mix = std::min(1.f, dt / 2.f);
    current.kind = target.kind;
    current.outdoor = target.outdoor;
    current.exposed = target.exposed;
    const auto ease = [&](float& value, float to) { value += (to - value) * mix; };
    ease(current.rain, target.rain);
    ease(current.storm, target.storm);
    ease(current.snow, target.snow);
    ease(current.fog, target.fog);
    ease(current.wind, target.wind);
    current.night = target.night;
    current.lightning = target.lightning;
    SnowSurface::Update(dt, current.snow);
    wetness = std::clamp(wetness + (current.rain * .1f - (1 - current.rain) * .025f) * dt, 0.f, 1.f);
    if (!exposed)
    {
        current.rain = 0;
        current.snow = 0;
        current.lightning = 0;
    }
    UpdateAudio(current);
}
bool WantsFog()
{
    return current.outdoor && (Options().distanceFog || current.fog > .02f);
}
void AdjustFog(mu::FogParams& fog)
{
    if (!WantsFog())
        return;
    const float amount = std::max(current.fog, Options().distanceFog ? .15f + .7f * Options().fogDensity / 100.f : 0.f);
    fog.start = std::max(100.f, fog.end * (.35f - amount * .25f));
    fog.end *= 1.f - amount * .45f;
    const float tint[3] = {.35f, .4f, .48f};
    for (int i = 0; i < 3; ++i)
        fog.color[i] = fog.color[i] * (1 - amount) + tint[i] * amount;
}
void AdjustLighting(mu::SceneLighting& lighting)
{
    if (!current.outdoor)
        return;
    if (Options().ambientLight)
    {
        lighting.ambient += .08f;
        lighting.diffuseFloor += .04f;
    }
    lighting.specularScale *= 1.f + wetness * .3f;
}
void DrawSky()
{
    DrawPegasusSky(currentMap);
}
void DrawWorld()
{
    if (!Hero || !current.outdoor)
        return;
    constexpr float Invisible = .001f;
    if (current.rain < Invisible && current.snow < Invisible && current.night < Invisible &&
        current.storm < Invisible && current.lightning < Invisible && SnowSurface::Cover() < Invisible && wetness < Invisible &&
        Options().atmosphere == 0)
        return;
    LoadTextures();
    auto& renderer = mu::GetRenderer();
    const mu::ScopedBloomExclusion noBloom;
    renderer.SetDepthTest(true);
    renderer.SetDepthMask(false);
    renderer.SetCullFace(false);
    renderer.SetAlphaTest(false);
    renderer.SetTexture2D(true);
    renderer.SetBlendMode(mu::BlendMode::Alpha);
    Puddles::Draw(currentMap,wetness,current.night,current.lightning);
    Surface();
    if (current.exposed)
    {
        RainParticles();
        SnowParticles();
    }
    const float atmosphere = Options().atmosphere / 100.f;
    Overlay(.10f, .14f, .24f, current.night * .36f + current.storm * .08f + atmosphere * .04f);
    Overlay(.84f, .91f, 1.f, current.lightning * .3f);
    renderer.SetDepthMask(true);
    renderer.SetDepthTest(true);
    renderer.SetAlphaTest(true);
    renderer.SetCullFace(true);
    renderer.SetTexture2D(true);
}
} // namespace Render::Weather
