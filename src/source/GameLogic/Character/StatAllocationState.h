#pragma once

#include <array>
#include <algorithm>

namespace GameLogic::Character
{
struct StatAllocationState
{
    std::array<int, 5> amounts{};
    std::array<int, 5> percentages{};
    std::array<int, 5> remainders{};
    std::array<bool, 5> full{};
    bool automatic = false;
    bool percentMode = false;
    bool open = false;
    bool applying = false;
    bool waiting = false;
    bool stopRequested = false;
    int selectedField = 0;
    bool replaceField = true;
    int pendingAmount = 0;
    static constexpr int MaxAmount = 65535;
    int pendingStat = 0;
    unsigned int previousValue = 0;
    unsigned int previousPoints = 0;
    unsigned long sentAt = 0;
    const wchar_t* status = L"Escribe cantidades y pulsa Add.";

    int Total() const;

    void EnterDigit(int digit);

    int PercentTotal() const;

    void PlanPercent(int available, const std::array<int, 5>& room, int spendable = -1);

    void PlanFull(int available, const std::array<int, 5>& room);

    bool StartAutomatic();

    void PlanAutomatic(int gained, int spendable, const std::array<int, 5>& room);

    void Stop(const wchar_t* message);
};
} // namespace GameLogic::Character
