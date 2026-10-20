#pragma once
#include <span>
namespace UI::Skills
{
    template <typename Slot> int ResolveIconSkill(int index, std::span<const Slot> slots, int petBegin, int petEnd)
    {
        if (index >= petBegin && index < petEnd) return index;
        if (index < 0 || static_cast<size_t>(index) >= slots.size()) return 0;
        return static_cast<int>(slots[index]);
    }
}
