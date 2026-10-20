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

        int Total() const
        {
            int total = 0;
            for (int amount : amounts) total += amount;
            return total;
        }

        void EnterDigit(int digit)
        {
            if (applying || digit < 0 || digit > 9) return;
            auto& field = percentMode ? percentages[selectedField] : amounts[selectedField];
            const int value = replaceField ? digit : field * 10 + digit;
            if (value <= (percentMode ? 100 : MaxAmount)) field = value;
            replaceField = false;
        }

        int PercentTotal() const
        {
            int total = 0;
            for (int percent : percentages) total += percent;
            return total;
        }

        void PlanPercent(int available, const std::array<int, 5>& room, int spendable = -1)
        {
            amounts.fill(0);
            if (PercentTotal() > 100 || available <= 0) return;
            // Fractional shares from earlier gains can mature together. They may
            // spend previously retained points, but never exceed the live balance.
            int budget = spendable < 0 ? available : spendable;
            for (int i = 0; i < 5; ++i)
            {
                const long long weighted = static_cast<long long>(available) * percentages[i] + remainders[i];
                const int wanted = static_cast<int>(weighted / 100);
                amounts[i] = std::min({wanted, std::max(0, room[i]), budget, MaxAmount});
                remainders[i] = room[i] > 0 ? static_cast<int>(weighted % 100) : 0;
                budget -= amounts[i];
            }
        }

        void PlanFull(int available, const std::array<int, 5>& room)
        {
            int budget = available;
            for (int i = 0; i < 5; ++i)
            {
                amounts[i] = std::min({full[i] ? std::max(0, room[i]) : amounts[i], std::max(0, room[i]), budget, MaxAmount});
                budget -= amounts[i];
            }
        }

        bool StartAutomatic()
        {
            if (percentMode && (PercentTotal() <= 0 || PercentTotal() > 100))
            {
                status = L"La suma debe ser de 1 a 100%.";
                return false;
            }
            if (!percentMode && std::none_of(full.begin(), full.end(), [](bool selected) { return selected; }))
            {
                status = L"Marca Lleno en el stat que deseas.";
                return false;
            }
            automatic = true;
            remainders.fill(0);
            status = L"Automatico activo. Esperando puntos.";
            return true;
        }

        void PlanAutomatic(int gained, int spendable, const std::array<int, 5>& room)
        {
            if (percentMode)
            {
                PlanPercent(std::min(gained, spendable), room, spendable);
                return;
            }
            amounts.fill(0);
            PlanFull(std::min(gained, spendable), room);
        }

        void Stop(const wchar_t* message)
        {
            amounts.fill(0);
            applying = false;
            waiting = false;
            stopRequested = false;
            status = message;
        }
    };
}
