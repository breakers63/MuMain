#pragma once
#include <array>
#include <string>
class CHARACTER;
namespace World::Monsters::Dressed
{
    struct Definition
    {
        int Id = 0;
        int Kind = 0;
        int Inventory = 0;
        int Class = 0;
        float Scale = 1;
        std::wstring Name;
        std::array<int, 21> Equipment{};
    };
    const Definition* Find(int id);
    CHARACTER* Create(const Definition& definition, int x, int y, int key);
    void Configure(const Definition& definition, CHARACTER* character);
}
