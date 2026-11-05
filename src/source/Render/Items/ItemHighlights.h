#pragma once

struct OBJECT;
namespace Render::Items::Highlights
{
    void Load();
    void Render(OBJECT* object, int modelType, int itemLevel, float alpha, int renderFlags);
}
