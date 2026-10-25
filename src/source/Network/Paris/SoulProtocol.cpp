#include "stdafx.h"
#include "SoulProtocol.h"
#include "Network/Server/WSclient.h"
#include "../../../ThirdParty/json.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <objbase.h>

namespace Network::Paris::Soul
{
    Snapshot Data;
    DWORD Received = 0;
    namespace
    {
        constexpr int MaximumJsonBytes = 60000, RequestLength = 128;
        std::wstring Wide(const std::string& text)
        {
            if (text.size() > 1024) throw std::runtime_error("Soul text limit");
            int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
            if (count == 0 && !text.empty()) throw std::runtime_error("Soul UTF8");
            std::wstring result(count, L'\0');
            if (count) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), count);
            return result;
        }

        Ability ReadAbility(const nlohmann::json& row, const Snapshot& snapshot)
        {
            Ability ability;
            ability.Enabled = row.at("enabled");
            ability.MaximumRank = row.at("maximumRank");
            ability.Cost = row.at("costPerRank");
            ability.MaximumBenefit = row.at("maximumBenefit");
            ability.Milestones = row.at("milestones").get<std::array<int, 3>>();
            ability.Bonuses = row.at("milestoneBenefits").get<std::array<double, 3>>();
            if (ability.MaximumRank < 1 || ability.MaximumRank > 10000 || ability.Cost < 1 || ability.Cost > 1000000
                || !std::isfinite(ability.MaximumBenefit) || ability.MaximumBenefit < 0 || ability.MaximumBenefit > 1000000)
                throw std::runtime_error("Soul ability range");
            for (int tier = 0; tier < 3; ++tier)
                if (ability.Milestones[tier] < 1 || ability.Milestones[tier] > ability.MaximumRank
                    || tier > 0 && ability.Milestones[tier] <= ability.Milestones[tier - 1]
                    || !std::isfinite(ability.Bonuses[tier]) || ability.Bonuses[tier] < 0 || ability.Bonuses[tier] > 1000000)
                    throw std::runtime_error("Soul milestone range");
            const auto custom = row.at("customName").get<std::string>();
            const auto key = row.at("nameKey").get<std::string>();
            auto found = snapshot.Labels.find(key);
            ability.Name = custom.empty() ? found == snapshot.Labels.end() ? Wide(key) : found->second : Wide(custom);
            return ability;
        }
    }

    bool Receive(const BYTE* packet, int size)
    {
        if (!packet || size < 6 || size > MaximumJsonBytes + 6 || packet[0] != 0xC2 || packet[3] != 0xFA
            || packet[4] != 0xD8 || packet[5] != 1 || ((packet[1] << 8) | packet[2]) != size) return false;
        try
        {
            auto json = nlohmann::json::parse(packet + 6, packet + size);
            const auto& state = json.at("state");
            const auto& settings = state.at("settings");
            Snapshot next;
            if (!json.at("labels").is_object() || json.at("labels").size() > 64 || settings.at("skills").size() != AbilityCount
                || !state.at("ranks").is_array() || state.at("ranks").size() != AbilityCount
                || !state.at("paid").is_array() || state.at("paid").size() != AbilityCount) return false;
            for (auto it = json.at("labels").begin(); it != json.at("labels").end(); ++it)
                if (it.key().size() <= 64) next.Labels[it.key()] = Wide(it.value()); else return false;
            next.Total = state.at("total"); next.Spent = state.at("spent"); next.Revision = state.at("revision");
            next.Token = state.at("configurationToken"); next.Decision = state.at("decision");
            next.Ranks = state.at("ranks").get<std::array<int, AbilityCount>>();
            next.Paid = state.at("paid").get<std::array<long long, AbilityCount>>();
            next.Enabled = settings.at("enabled"); next.AllowReallocation = settings.at("allowReallocation"); next.AllowReset = settings.at("allowReset");
            next.ResetFee = settings.at("resetFee"); next.ResetCurrency = settings.at("resetFeeCurrency");
            if (next.Total < 0 || next.Total > INT_MAX || next.Spent < 0 || next.Spent > next.Total || next.Revision < 0
                || next.Decision < 0 || next.Decision > 5 || next.ResetFee < 0 || next.ResetCurrency < 0 || next.ResetCurrency > 7) return false;
            for (int index = 0; index < AbilityCount; ++index)
            {
                next.Abilities[index] = ReadAbility(settings.at("skills")[index], next);
                if (next.Ranks[index] < 0 || next.Ranks[index] > 10000 || next.Paid[index] < 0 || next.Paid[index] > INT_MAX) return false;
            }
            long long paidTotal = 0;
            for (auto value : next.Paid) paidTotal += value;
            if (paidTotal != next.Spent) return false;
            Data = std::move(next);
            Received = timeGetTime();
            return true;
        }
        catch (const std::exception&) { return false; }
    }

    void Clear() { Data = Snapshot{}; Received = 0; }

    const std::wstring& Label(const char* key)
    {
        static const std::wstring empty;
        auto found = Data.Labels.find(key);
        return found == Data.Labels.end() ? empty : found->second;
    }

    bool Request(BYTE action, BYTE category, const std::array<int, AbilityCount>& ranks)
    {
        if (!SocketClient || action > 2 || category > 3) return false;
        BYTE packet[RequestLength] = { 0xC1, RequestLength, 0xFA, 0xD7, 1, action, category };
        std::memcpy(packet + 8, &Data.Revision, sizeof(Data.Revision));
        std::memcpy(packet + 16, &Data.Token, sizeof(Data.Token));
        std::memcpy(packet + 24, ranks.data(), sizeof(int) * AbilityCount);
        if (action != 0)
        {
            GUID identity{};
            if (FAILED(CoCreateGuid(&identity))) return false;
            std::snprintf(reinterpret_cast<char*>(packet + 88), 37, "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                identity.Data1, identity.Data2, identity.Data3, identity.Data4[0], identity.Data4[1], identity.Data4[2],
                identity.Data4[3], identity.Data4[4], identity.Data4[5], identity.Data4[6], identity.Data4[7]);
        }
        SocketClient->Send(packet, sizeof(packet));
        return true;
    }
}
