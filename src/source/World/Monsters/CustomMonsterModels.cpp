#include "stdafx.h"
#include "CustomMonsterModels.h"
#include "CustomMonsterColors.h"

#include <cmath>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <SDL3/SDL_stdinc.h>
#include "Core/Globals/_enum.h"
#include "Core/Utilities/Log/ErrorReport.h"
#include "Data/DataHandler/LoadData.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Models/ZzzBMD.h"

namespace World::Monsters::Custom
{
    namespace
    {
        constexpr int FirstModelSlot = 256;
        constexpr const wchar_t* Folders[] = {L"Monster\\", L"NPC\\", L"Custom\\Monster\\", L"Custom\\NPC\\"};

        std::wstring ToWide(const std::string& value)
        {
            std::unique_ptr<char, decltype(&SDL_free)> converted(
                SDL_iconv_string("WCHAR_T", "UTF-8", value.c_str(), value.size() + 1), SDL_free);
            return converted ? std::wstring(reinterpret_cast<const wchar_t*>(converted.get())) : std::wstring();
        }

        bool ParseDefinition(const std::string& line, Definition& definition)
        {
            std::istringstream row(line);
            std::string name, model;
            if (!(row >> definition.Id >> definition.Type >> std::quoted(name) >> definition.Folder
                >> std::quoted(model) >> definition.Scale) || !(row >> std::ws).eof()) return false;
            constexpr int MaximumMonsterId = 32767;
            constexpr float MaximumScale = 1000.f;
            if (definition.Id < 0 || definition.Id > MaximumMonsterId || definition.Type < 0 || definition.Type > 1
                || definition.Folder < 0 || definition.Folder >= std::size(Folders)
                || !std::isfinite(definition.Scale) || definition.Scale <= 0 || definition.Scale > MaximumScale
                || model.empty() || model.size() > 48 || model.find_first_of("/\\:") != std::string::npos
                || model.find("..") != std::string::npos) return false;
            definition.Name = ToWide(name);
            definition.ModelName = ToWide(model);
            return true;
        }

        std::unordered_map<int, Definition> LoadDefinitions()
        {
            std::unordered_map<int, Definition> definitions;
            std::ifstream file("Data/Custom/Monsters.txt");
            std::string line;
            int lineNumber = 0;
            while (std::getline(file, line))
            {
                ++lineNumber;
                if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) line.erase(0, 3);
                line = line.substr(0, line.find("//"));
                std::istringstream row(line);
                std::string marker;
                if (!(row >> marker)) continue;
                if (marker == "end") break;
                if (marker == "0" && (row >> std::ws).eof()) continue;
                Definition definition;
                if (!ParseDefinition(line, definition))
                {
                    g_ErrorReport.Write(L"Custom monster configuration is invalid at line %d.\r\n", lineNumber);
                    continue;
                }
                if (definitions.contains(definition.Id)) continue;
                definition.ModelSlot = FirstModelSlot + static_cast<int>(definitions.size());
                if (definition.ModelSlot >= MAX_MODEL_MONSTER)
                {
                    g_ErrorReport.Write(L"Custom monster model capacity exceeded at line %d.\r\n", lineNumber);
                    break;
                }
                definitions.emplace(definition.Id, std::move(definition));
            }
            return definitions;
        }

        bool LoadModel(const Definition& definition)
        {
            const int index = MODEL_MONSTER01 + definition.ModelSlot;
            BMD& model = Models[index];
            if (model.NumMeshs > 0) return model.NumBones > 0 && model.NumBones <= MAX_BONES && model.NumActions > 0;
            static bool failed[MAX_MODEL_MONSTER] = {};
            if (failed[definition.ModelSlot]) return false;
            const std::wstring directory = std::wstring(L"Data\\") + Folders[definition.Folder];
            std::ifstream file(std::filesystem::path(directory + definition.ModelName + L".bmd"), std::ios::binary);
            char header[4] = {};
            if (!file.read(header, sizeof(header)) || header[0] != 'B' || header[1] != 'M' || header[2] != 'D'
                || (header[3] != 10 && header[3] != 12))
            {
                failed[definition.ModelSlot] = true;
                g_ErrorReport.Write(L"Custom BMD missing or unsupported: ID %d, %ls%ls.bmd.\r\n", definition.Id, directory.c_str(), definition.ModelName.c_str());
                return false;
            }
            gLoadData.AccessModel(index, directory.c_str(), definition.ModelName.c_str());
            if (model.NumMeshs <= 0 || model.NumBones <= 0 || model.NumBones > MAX_BONES || model.NumActions <= 0)
            {
                failed[definition.ModelSlot] = true;
                g_ErrorReport.Write(L"Custom model unavailable: ID %d, %ls%ls.bmd.\r\n", definition.Id, directory.c_str(), definition.ModelName.c_str());
                return false;
            }
            gLoadData.OpenTexture(index, Folders[definition.Folder]);
            for (int action = 0; action < model.NumActions; ++action) model.Actions[action].PlaySpeed = 0.25f;
            return true;
        }
    }

    const Definition* Find(int monsterId)
    {
        static const auto definitions = LoadDefinitions();
        const auto found = definitions.find(monsterId);
        return found == definitions.end() ? nullptr : &found->second;
    }

    const GoldenColor* FindColor(int monsterId)
    {
        static const auto colors = []
        {
            std::ifstream file("Data/Custom/Monsters.txt");
            try { return ReadGoldenColors(file); }
            catch (const std::invalid_argument&)
            {
                g_ErrorReport.Write(L"Invalid custom monster RGB configuration. Models remain available.\r\n");
                return std::unordered_map<int, GoldenColor>{};
            }
        }();
        const auto found = colors.find(monsterId);
        return found == colors.end() ? nullptr : &found->second;
    }

    CHARACTER* Create(const Definition& definition, int x, int y, int key)
    {
        if (!LoadModel(definition)) return nullptr;
        return CreateCharacter(key, MODEL_MONSTER01 + definition.ModelSlot, static_cast<unsigned char>(x), static_cast<unsigned char>(y));
    }

    void Configure(const Definition& definition, CHARACTER* character)
    {
        if (character == nullptr) return;
        character->Object.Scale = definition.Scale;
        character->Object.Kind = definition.Type == 0 ? KIND_NPC : KIND_MONSTER;
        wcsncpy_s(character->ID, MAX_MONSTER_NAME + 1, definition.Name.c_str(), _TRUNCATE);
    }
}
