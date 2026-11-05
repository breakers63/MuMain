#pragma once

#include <string>
#include <array>

class CHARACTER;

namespace World::Monsters::Custom
{
    struct Definition
    {
        int Id = 0;
        int Type = 0;
        int Folder = 0;
        int ModelSlot = 0;
        float Scale = 1.0f;
        std::wstring Name;
        std::wstring ModelName;
    };

    const Definition* Find(int monsterId);
    const std::array<float, 3>* FindColor(int monsterId);
    CHARACTER* Create(const Definition& definition, int x, int y, int key);
    void Configure(const Definition& definition, CHARACTER* character);
}
