#include "stdafx.h"
#include "ItemProgressionConfig.h"
#include "../../../ThirdParty/json.hpp"
#include <array>
#include <fstream>

namespace GameLogic::Items::Progression {
namespace {
constexpr int MaximumItemTypes = 8192;
constexpr size_t MaximumConfigurationBytes = 100000;
constexpr const char* ConfigurationPath = "Data/Custom/ParisItemTiers.json";
using TierMap = std::array<unsigned char, MaximumItemTypes>;

TierMap Load() {
  TierMap result{};
  std::ifstream file(ConfigurationPath, std::ios::binary | std::ios::ate);
  if (!file || file.tellg() <= 0 || file.tellg() > MaximumConfigurationBytes)
    return result;
  file.seekg(0);
  try {
    const auto root = nlohmann::json::parse(file);
    if (root.at("Version").get<int>() != 1 || !root.at("Items").is_array() ||
        root.at("Items").size() > MaximumItemTypes)
      return {};
    for (const auto& item : root.at("Items")) {
      const int code = item.at("ItemIndex").get<int>();
      const int tier = item.at("Tier").get<int>();
      if (code < 0 || code >= MaximumItemTypes || tier < 1 ||
          tier > MaximumTier || result[code] != 0)
        return {};
      result[code] = static_cast<unsigned char>(tier);
    }
    return result;
  } catch (const std::exception&) {
    return {};
  }
}
} // namespace

int TierFor(int itemType, bool excellent) {
  static const TierMap tiers = Load();
  if (itemType < 0 || itemType >= MaximumItemTypes || tiers[itemType] == 0)
    return 0;
  return excellent ? tiers[itemType] : 1;
}

const wchar_t* Suffix(int tier) {
  static constexpr const wchar_t* Labels[] = {
      L"", L" - TIER 1", L" - TIER 2", L" - TIER 3", L" - TIER 4",
      L" - TIER 5", L" - TIER 6", L" - TIER 7", L" - TIER 8"};
  return tier >= 0 && tier <= MaximumTier ? Labels[tier] : Labels[0];
}
} // namespace GameLogic::Items::Progression
