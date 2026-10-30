#pragma once
#include "UI/NPCs/BuffDetails.h"

#include <string>

#include <string_view>

#include <vector>

#include <cstdint>

#include <sstream>

#include <array>

namespace UI::NPCs::ServiceDialog

{

    struct Item

    {

        int type = 0;

        int level = 0;

        int count = 0;

        int owned = 0;

        std::wstring name;

    };

    struct Snapshot

    {

        bool active = false;

        bool ready = false;

        std::uint16_t npc = 0;

        long long cost = 0;

        long long money = 0;

        bool eligible = false;

        std::array<std::wstring,5> presentation{L"REQUISITOS",L"BUFF DISPONIBLE",L"DURACIÓN",L"PORCENTAJE",L""};

        std::wstring title;

        std::vector<std::wstring> lines;

        std::vector<Item> requirements;

        std::vector<Item> rewards;

    };

    inline bool CanExchange(const Snapshot& quote)

    {

        if (!quote.eligible || quote.money < quote.cost) return false;

        for (const auto& item : quote.requirements) if (item.owned < item.count) return false;

        return true;

    }

    inline Snapshot state;

    // These bounded notices come from the NPC service and are consumed before chat display.

    inline bool Receive(std::wstring_view text)

    {
        if(UI::BuffDetails::Receive(text))return true;

        constexpr std::wstring_view begin = L"~npc.begin~";

        constexpr std::wstring_view line = L"~npc.line~";

        constexpr std::wstring_view end = L"~npc.end~";

        if (text.starts_with(begin))

        {

            state = {};

            const auto value = text.substr(begin.size());

            const auto separator = value.find(L'|');

            if (separator == std::wstring_view::npos || separator > 5) return true;

            unsigned id = 0;

            for (auto c : value.substr(0, separator))

            {

                if (c < L'0' || c > L'9') return true;

                id = id * 10 + static_cast<unsigned>(c - L'0');

            }

            if (id > 65535) return true;

            state.npc = static_cast<std::uint16_t>(id);

            state.title = value.substr(separator + 1, 64);

            state.active = true;

            return true;

        }

        if (text.starts_with(line))

        {

            if (state.active && state.lines.size() < 16) state.lines.emplace_back(text.substr(line.size(), 180));

            return true;

        }

        constexpr std::wstring_view uiPrefix = L"~npc.ui~";

        if(text.starts_with(uiPrefix)) {

            const auto value=text.substr(uiPrefix.size());

            if(state.active && value.size()>2 && value[1]==L'|' && value[0]>=L'0' && value[0]<=L'4')

                state.presentation[value[0]-L'0']=value.substr(2,180);

            return true;

        }

        constexpr std::wstring_view metadata = L"~npc.meta~";

        if (text.starts_with(metadata))

        {

            if (!state.active) return true;

            std::wistringstream input(std::wstring{text.substr(metadata.size())});

            long long cost = 0, money = 0, eligible = 0;

            wchar_t separator1 = 0, separator2 = 0;

            if (input >> cost >> separator1 >> money >> separator2 >> eligible

                && (input >> std::ws).eof() && separator1 == L'|' && separator2 == L'|'

                && cost >= 0 && money >= 0 && cost <= 2147483647 && money <= 2147483647 && (eligible == 0 || eligible == 1))

            {

                state.cost = cost; state.money = money; state.eligible = eligible == 1;

            }

            return true;

        }

        constexpr std::wstring_view itemPrefix = L"~npc.item~";

        if (text.starts_with(itemPrefix))

        {

            if (!state.active) return true;

            std::wistringstream input(std::wstring{text.substr(itemPrefix.size())});

            std::array<std::wstring, 7> fields;

            for (int i = 0; i < 6; ++i) if (!std::getline(input, fields[i], L'|')) return true;

            std::getline(input, fields[6]);

            int values[5]{};

            for (int i = 0; i < 5; ++i)

            {

                std::wistringstream value(fields[i + 1]);

                if (!(value >> values[i]) || !(value >> std::ws).eof()) return true;

            }

            if (values[0] < 0 || values[0] > 15 || values[1] < 0 || values[1] > 511

                || values[2] < -1 || values[2] > 15 || values[3] < 1 || values[3] > 999 || values[4] < 0) return true;

            if (fields[0] != L"R" && fields[0] != L"P") return true;

            auto& items = fields[0] == L"R" ? state.requirements : state.rewards;

            if (items.size() < 6) items.push_back({values[0] * 512 + values[1], values[2], values[3], values[4], fields[6].substr(0, 100)});

            return true;

        }

        if (text == end)

        {

            state.ready = state.active;

            return true;

        }

        return false;

    }

}

