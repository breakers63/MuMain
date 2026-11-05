#pragma once

class CHARACTER;
namespace Render::Items::SetTrails
{
    constexpr short Marker = -2;
    void Load();
    float Speed();
    bool Render(CHARACTER* character, int minimumEquipmentLevel);
}
