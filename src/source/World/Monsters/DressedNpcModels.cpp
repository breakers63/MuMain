#include "stdafx.h"
#include "DressedNpcModels.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include "Core/Globals/_enum.h"
#include "Core/Utilities/Log/ErrorReport.h"
#include "Engine/Object/ZzzCharacter.h"

namespace World::Monsters::Dressed
{
    namespace
    {
        constexpr int ItemGroupSize = 512;
        constexpr int FirstArmorGroup = 7;
        constexpr int MaximumEquipmentLevel = 15;
        constexpr int MaximumNpcId = 32767;
        constexpr const char* ConfigPath = "Data/Custom/Dressed.txt";

        bool ReadEquipment(std::istringstream& row, std::array<int, 21>& equipment)
        {
            for (int i = 0; i < 20; ++i) if (!(row >> equipment[i])) return false;
            if (!(row >> std::ws).eof() && !(row >> equipment[20])) return false;
            if (equipment[20] < 0 || equipment[20] > 63) return false;
            if (!(row >> std::ws).eof()) return false;
            if (equipment[1] != -1 && equipment[1] != 0) return false;
            for (int part = 2; part <= 7; ++part)
                if (equipment[part] < -1 || equipment[part] >= ItemGroupSize) return false;
            for (int pair = 8; pair <= 14; pair += 2)
                if (equipment[pair] < -1 || equipment[pair] > 15 || equipment[pair + 1] < -1 || equipment[pair + 1] >= ItemGroupSize) return false;
            return equipment[17] >= -1 && equipment[17] <= MaximumEquipmentLevel
                && equipment[18] >= -1 && equipment[18] <= MaximumEquipmentLevel;
        }

        std::unordered_map<int, Definition> Load()
        {
            std::ifstream file(ConfigPath);
            std::unordered_map<int, Definition> definitions;
            std::unordered_map<int, std::array<int, 21>> equipment;
            std::string line;
            int section = -1, lineNumber = 0;
            while (std::getline(file, line))
            {
                ++lineNumber;
                if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) line.erase(0, 3);
                line = line.substr(0, line.find("//"));
                std::istringstream marker(line);
                std::string word;
                if (!(marker >> word)) continue;
                if (word == "end") { section = -1; continue; }
                if (section == -1 && (word == "0" || word == "1") && (marker >> std::ws).eof())
                { section = word == "0" ? 0 : 1; continue; }
                std::istringstream row(line);
                bool valid = false;
                if (section == 0)
                {
                    Definition d;
                    std::string name;
                    valid = static_cast<bool>(row >> d.Id >> d.Kind >> d.Inventory >> d.Class >> d.Scale >> std::quoted(name));
                    valid = valid && (row >> std::ws).eof() && d.Id > 0 && d.Id <= MaximumNpcId && d.Kind >= 0 && d.Kind <= 1
                        && d.Class >= 0 && d.Class <= CLASS_END && std::isfinite(d.Scale) && d.Scale > 0 && d.Scale <= 10
                        && !definitions.contains(d.Id) && name.size() <= MAX_MONSTER_NAME;
                    if (valid) { d.Name.assign(name.begin(), name.end()); definitions.emplace(d.Id, d); }
                }
                else if (section == 1)
                {
                    std::array<int, 21> values{};
                    valid = ReadEquipment(row, values) && !equipment.contains(values[0]);
                    if (valid) equipment.emplace(values[0], values);
                }
                if (!valid) g_ErrorReport.Write(L"Invalid Dressed row %d.\r\n", lineNumber);
            }
            for (auto it = definitions.begin(); it != definitions.end();)
            {
                const auto found = equipment.find(it->second.Inventory);
                if (found == equipment.end()) it = definitions.erase(it);
                else { it->second.Equipment = found->second; ++it; }
            }
            return definitions;
        }

        int Model(int group, int index)
        {
            return group < 0 || index < 0 ? -1 : MODEL_ITEM + group * ItemGroupSize + index;
        }
    }

    const Definition* Find(int id)
    {
        static const auto definitions = Load();
        const auto it = definitions.find(id);
        return it == definitions.end() ? nullptr : &it->second;
    }

    CHARACTER* Create(const Definition&, int x, int y, int key)
    {
        return CreateCharacter(key, MODEL_PLAYER, static_cast<unsigned char>(x), static_cast<unsigned char>(y));
    }

    void Configure(const Definition& definition, CHARACTER* character)
    {
        if (character == nullptr) return;
        const auto& e = definition.Equipment;
        character->Class = static_cast<CLASS_TYPE>(definition.Class);
        character->Object.Kind = definition.Kind == 0 ? KIND_NPC : KIND_MONSTER;
        character->Object.Scale = definition.Scale;
        for (int part = 0; part < 5; ++part)
        {
            character->BodyPart[part].Type = Model(FirstArmorGroup + part, e[1] == -1 ? e[2] : e[3 + part]);
            character->BodyPart[part].ExcellentFlags = static_cast<unsigned char>(e[20]);
            character->BodyPart[part].Level = static_cast<unsigned char>(std::max(0, e[17]));
        }
        for (int weapon = 0; weapon < 2; ++weapon)
        {
            character->Weapon[weapon].Type = Model(e[8 + weapon * 2], e[9 + weapon * 2]);
            character->Weapon[weapon].Level = static_cast<unsigned char>(std::max(0, e[18]));
        }
        character->Wing.Type = Model(e[12], e[13]);
        character->Wing.Level = static_cast<unsigned char>(std::max(0, e[17]));
        character->Wing.ExcellentFlags = static_cast<unsigned char>(e[20]);
        character->Helper.Type = Model(e[14], e[15]);
        character->Helper.Level = static_cast<unsigned char>(std::max(0, e[16]));
        if (e[19] >= 0) character->PK = static_cast<unsigned char>(e[19]);
        wcsncpy_s(character->ID, MAX_MONSTER_NAME + 1, definition.Name.c_str(), _TRUNCATE);
    }
}
