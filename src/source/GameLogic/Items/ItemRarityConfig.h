#pragma once
#include <array>
#include <string>
#include <vector>
namespace GameLogic::Items::Rarity {
constexpr int MaximumConfigurationBytes = 60000;
constexpr int MaximumItemTypes = 8192;
struct Tier {
  std::wstring Name;
  std::array<std::wstring, 3> NameTranslations;
  std::array<int, 3> Color{}, Background{}, Border{};
  float BorderWidth = 1;
};
struct Bonus {
  int Level = 0;
  bool Percent = true;
  std::wstring Label;
  std::array<std::wstring, 3> LabelTranslations;
  std::array<float, 4> Values{};
};
struct Category {
  std::vector<Bonus> Bonuses;
};
struct Legend {
  int Start = 0, End = 0, Position = 1, MinLevel = -1, MaxLevel = -1,
      Skill = -1, Luck = -1;
  int MinOption = -1, MaxOption = -1, MinExcellent = -1, MaxExcellent = -1,
      Ancient = -1, BreakLine = 1, Color = 0;
  bool Bold = false;
  std::vector<std::wstring> Lines;
};
struct TooltipStyle {
  std::array<int, 3> Text{};
  std::array<int, 4> Background{};
};
struct Configuration {
  bool Enabled = false;
  std::array<Tier, 4> Tiers;
  std::array<TooltipStyle, 21> Styles;
  std::array<int, MaximumItemTypes> CategoryByItem;
  std::vector<Category> Categories;
  std::vector<Legend> Lore;
  Configuration() { CategoryByItem.fill(-1); }
};
const Configuration &Current();
bool Replace(const char *bytes, int size);
int ExcellentCount(unsigned char mask);
const std::wstring &Localized(const std::wstring &fallback,
                              const std::array<std::wstring, 3> &translations,
                              int language);
} // namespace GameLogic::Items::Rarity
