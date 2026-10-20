#include "stdafx.h"
#include "ItemRarityConfig.h"
#include "../../../ThirdParty/json.hpp"
#include <cmath>
#include <fstream>
namespace GameLogic::Items::Rarity {
namespace {
Configuration Config;
bool Loaded = false;
std::wstring Wide(const std::string &text) {
  const int length =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                          static_cast<int>(text.size()), nullptr, 0);
  if (length <= 0)
    throw std::invalid_argument("Invalid UTF-8 rarity text");
  std::wstring result(length, 0);
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                      static_cast<int>(text.size()), result.data(), length);
  return result;
}
std::vector<std::wstring> Wrap(const std::wstring &text) {
  constexpr int LineLength = 85;
  std::vector<std::wstring> result;
  size_t offset = 0;
  while (offset < text.size()) {
    size_t length = std::min<size_t>(LineLength, text.size() - offset);
    if (offset + length < text.size()) {
      const auto space = text.rfind(L' ', offset + length);
      if (space != std::wstring::npos && space > offset)
        length = space - offset;
    }
    result.push_back(text.substr(offset, length));
    offset += length;
    while (offset < text.size() && text[offset] == L' ')
      ++offset;
  }
  return result;
}
std::array<std::wstring, 3> Translations(const nlohmann::json &source,
                                         const char *key,
                                         const std::wstring &fallback,
                                         size_t limit) {
  std::array<std::wstring, 3> result;
  const char *codes[] = {"ENG", "POR", "SPN"};
  if (source.contains(key)) {
    const auto &values = source.at(key);
    if (!values.is_object() || values.size() > 3)
      throw std::invalid_argument("Invalid translations");
    for (int i = 0; i < 3; ++i)
      if (values.contains(codes[i])) {
        const auto value = values.at(codes[i]).get<std::string>();
        if (!value.empty())
          result[i] = Wide(value);
        if (result[i].size() > limit)
          throw std::invalid_argument("Translation too long");
      }
  }
  // Reference translations also support v26 servers without language fields.
  struct Reference {
    const wchar_t *Spanish, *English, *Portuguese;
  };
  static const Reference reference[] = {
      {L"Aumento de daño físico y mágico",L"Physical and magic damage increase",L"Aumento de dano físico e mágico"},
      {L"Aumento de defensa",L"Defense increase",L"Aumento de defesa"},
      {L"Reducción del daño recibido",L"Damage reduction",L"Redução de dano recebido"},
      {L"Velocidad de ataque físico y mágico",L"Physical and magic attack speed",L"Velocidade de ataque físico e mágico"},
      {L"Índice de ataque PvM/PvP",L"PvM/PvP attack rate",L"Taxa de ataque PvM/PvP"},
      {L"Índice de defensa PvM/PvP",L"PvM/PvP defense rate",L"Taxa de defesa PvM/PvP"},
      {L"Mana máximo",L"Maximum mana",L"Mana máximo"},
      {L"Probabilidad de daño doble",L"Double damage chance",L"Chance de dano duplo"},
      {L"COMÚN", L"COMMON", L"COMUM"},
      {L"RARO", L"RARE", L"RARO"},
      {L"ÉPICO", L"EPIC", L"ÉPICO"},
      {L"LEGENDARIO", L"LEGENDARY", L"LENDÁRIO"},
      {L"Probabilidad de daño Excellent", L"Excellent damage chance",
       L"Chance de dano Excellent"},
      {L"Probabilidad de ignorar defensa", L"Ignore defense chance",
       L"Chance de ignorar defesa"},
      {L"Aumento de daño", L"Damage increase", L"Aumento de dano"},
      {L"Reducción de daño", L"Damage reduction", L"Redução de dano"},
      {L"Probabilidad de daño crítico", L"Critical damage chance",
       L"Chance de dano crítico"},
      {L"Velocidad de ataque", L"Attack speed", L"Velocidade de ataque"},
      {L"Resistencia a daño crítico", L"Critical damage resistance",
       L"Resistência a dano crítico"},
      {L"Resistencia a daño Excellent", L"Excellent damage resistance",
       L"Resistência a dano Excellent"},
      {L"Devolución de ataque", L"Damage reflection", L"Reflexão de dano"},
      {L"Vida máxima", L"Maximum health", L"Vida máxima"},
      {L"Resistencia a daño doble", L"Double damage resistance",
       L"Resistência a dano duplo"},
      {L"Resistencia a aturdimiento", L"Stun resistance",
       L"Resistência a atordoamento"},
  };
  for (const auto &entry : reference)
    if (fallback == entry.Spanish) {
      if (result[0].empty())
        result[0] = entry.English;
      if (result[1].empty())
        result[1] = entry.Portuguese;
      break;
    }
  return result;
}
void ReadCategories(const nlohmann::json &root, Configuration &next) {
  const auto &categories = root.at("Categories");
  if (categories.size() > 32)
    throw std::invalid_argument("Too many rarity categories");
  for (const auto &source : categories) {
    Category category;
    if (source.at("Bonuses").size() > 8)
      throw std::invalid_argument("Too many rarity bonuses");
    for (const auto &option : source.at("Bonuses")) {
      Bonus bonus;
      bonus.Level = option.at("Level").get<int>();
      bonus.Percent = option.at("Percent").get<bool>();
      bonus.Label = Wide(option.at("Label").get<std::string>());
      bonus.LabelTranslations =
          Translations(option, "LabelTranslations", bonus.Label, 70);
      bonus.Values = option.at("Values").get<std::array<float, 4>>();
      if (bonus.Level < 0 || bonus.Level > 15 || bonus.Label.size() > 70)
        throw std::invalid_argument("Invalid rarity bonus");
      for (float value : bonus.Values)
        if (!std::isfinite(value) || value < 0 || value > 100000)
          throw std::invalid_argument("Invalid rarity value");
      category.Bonuses.push_back(std::move(bonus));
    }
    for (const auto &sourceRange : source.at("Ranges")) {
      const auto range = sourceRange.get<std::array<int, 2>>();
      if (range[0] < 0 || range[1] >= MaximumItemTypes || range[0] > range[1])
        throw std::invalid_argument("Invalid item range");
      for (int item = range[0]; item <= range[1]; ++item) {
        if (next.CategoryByItem[item] >= 0)
          throw std::invalid_argument("Overlapping categories");
        next.CategoryByItem[item] = static_cast<int>(next.Categories.size());
      }
    }
    next.Categories.push_back(std::move(category));
  }
}
void ReadLore(const nlohmann::json &root, Configuration &next) {
  if (root.at("Lore").size() > 200)
    throw std::invalid_argument("Too many item legends");
  for (const auto &source : root.at("Lore")) {
    Legend line;
    line.Start = source.at("Start");
    line.End = source.at("End");
    line.Position = source.at("Position");
    line.MinLevel = source.at("MinLevel");
    line.MaxLevel = source.at("MaxLevel");
    line.Skill = source.at("Skill");
    line.Luck = source.at("Luck");
    line.MinOption = source.at("MinOption");
    line.MaxOption = source.at("MaxOption");
    line.MinExcellent = source.at("MinExcellent");
    line.MaxExcellent = source.at("MaxExcellent");
    line.Ancient = source.at("Ancient");
    line.BreakLine = source.at("BreakLine");
    line.Color = source.at("Color");
    line.Bold = source.at("Bold");
    auto text = Wide(source.at("Text").get<std::string>());
    if (line.Start < 0 || line.End >= MaximumItemTypes ||
        line.Start > line.End || line.Position < 0 || line.Position > 4 ||
        line.Color < 0 || line.Color > 20 || text.size() > 255 ||
        line.BreakLine < 0 || line.BreakLine > 10)
      throw std::invalid_argument("Invalid legend");
    line.Lines = Wrap(text);
    next.Lore.push_back(std::move(line));
  }
}
} // namespace
bool Replace(const char *bytes, int size) {
  if (!bytes || size <= 0 || size > MaximumConfigurationBytes)
    return false;
  try {
    const auto root = nlohmann::json::parse(bytes, bytes + size);
    Configuration next;
    next.Enabled = root.at("Enabled").get<bool>();
    if (root.at("Rarities").size() != next.Tiers.size())
      return false;
    for (int i = 0; i < 4; ++i) {
      const auto &source = root.at("Rarities")[i];
      auto &tier = next.Tiers[i];
      tier.Name = Wide(source.at("Name").get<std::string>());
      tier.NameTranslations =
          Translations(source, "NameTranslations", tier.Name, 32);
      tier.Color = source.at("Color").get<std::array<int, 3>>();
      tier.Background = source.at("Background").get<std::array<int, 3>>();
      tier.Border = source.at("Border").get<std::array<int, 3>>();
      tier.BorderWidth = source.at("BorderWidth").get<float>();
      if (!std::isfinite(tier.BorderWidth) || tier.BorderWidth < 0 ||
          tier.BorderWidth > 4)
        return false;
      for (int value : tier.Border)
        if (value < 0 || value > 255)
          return false;
      if (source.at("MinimumExcellent").get<int>() != i ||
          tier.Name.size() > 32)
        return false;
      for (int value : tier.Color)
        if (value < 0 || value > 255)
          return false;
      for (int value : tier.Background)
        if (value < 0 || value > 255)
          return false;
    }
    if (root.at("TooltipStyles").size() != next.Styles.size())
      return false;
    for (size_t i = 0; i < next.Styles.size(); ++i) {
      next.Styles[i].Text =
          root.at("TooltipStyles")[i].at("Text").get<std::array<int, 3>>();
      next.Styles[i].Background = root.at("TooltipStyles")[i]
                                      .at("Background")
                                      .get<std::array<int, 4>>();
      for (int value : next.Styles[i].Text)
        if (value < 0 || value > 255)
          return false;
      for (int value : next.Styles[i].Background)
        if (value < 0 || value > 255)
          return false;
    }
    ReadCategories(root, next);
    ReadLore(root, next);
    Config = std::move(next);
    Loaded = true;
    return true;
  } catch (const std::exception &) {
    return false;
  }
}
const Configuration &Current() {
  if (Loaded)
    return Config;
  Loaded = true;
  std::ifstream file("Data/Custom/ParisItemRarity.json", std::ios::binary);
  const std::string data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  Replace(data.data(), static_cast<int>(data.size()));
  return Config;
}
const std::wstring &Localized(const std::wstring &fallback,
                              const std::array<std::wstring, 3> &translations,
                              int language) {
  return language >= 0 && language < 3 && !translations[language].empty()
             ? translations[language]
             : fallback;
}
int ExcellentCount(unsigned char mask) {
  constexpr unsigned char ExcellentMask = 0x3F;
  int count = 0;
  for (unsigned char flags = mask & ExcellentMask; flags; flags >>= 1)
    count += flags & 1;
  return count;
}
} // namespace GameLogic::Items::Rarity
