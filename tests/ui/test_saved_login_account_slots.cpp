#include <doctest.h>
#include "Data/GameConfig/SavedLoginAccountSlots.h"
#include <array>

TEST_CASE("Automatic remembered login uses free space and never replaces another account")
{
    std::array<SavedLoginAccount, SavedLoginAccountLimit> slots{};
    slots[0].username = L"First";
    slots[1].username = L"Second";
    CHECK(Data::Login::ChooseAccountSlot(slots, L"Third", 0, false) == 2);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"Second", 0, false) == 1);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"Third", 4, false) == 4);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"Third", -1, false) == 2);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"Third", 99, false) == 2);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"", 0, false) == -1);
    for (std::size_t i = 2; i < slots.size(); ++i)
        slots[i].username = L"Filled" + std::to_wstring(i);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"New", 0, false) == -1);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"New", 0, true) == 0);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"New", -1, true) == -1);
    CHECK(Data::Login::ChooseAccountSlot(slots, L"Second", 0, true) == 1);
}
