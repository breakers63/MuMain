#include "stdafx.h"
#include "ItemRarityTooltip.h"
#include "GameLogic/Items/ItemRarityConfig.h"
#include "Network/Paris/ItemRarityProtocol.h"
#include "UI/RmlBridge/RmlTooltip.h"
#include "Data/Translation/MultiLanguage.h"
#include "Core/Utilities/StringUtils.h"
#include "Core/Globals/_struct.h"
#include <algorithm>
#include <cstdio>

namespace UI::Items::RarityTooltip
{
namespace
{
using namespace GameLogic::Items::Rarity;
using UI::RmlBridge::Tooltip::Line;

std::string Color(const std::array<int, 3>& value)
{
    char text[16];
    std::snprintf(text, sizeof(text), "#%02x%02x%02x", value[0], value[1], value[2]);
    return text;
}

void ApplyStyle(Line& line, const TooltipStyle& style)
{
    line.customColor = Color(style.Text);
    char background[20];
    std::snprintf(background, sizeof(background), "#%02x%02x%02x%02x", style.Background[0], style.Background[1], style.Background[2], style.Background[3]);
    line.customBackground = background;
}

bool Matches(const Legend& lore, const tagITEM& item)
{
    const int excellent = ExcellentCount(item.ExcellentFlags);
    return item.Type >= lore.Start && item.Type <= lore.End &&
        (lore.MinLevel < 0 || item.Level >= lore.MinLevel) && (lore.MaxLevel < 0 || item.Level <= lore.MaxLevel) &&
        (lore.Skill < 0 || item.HasSkill == bool(lore.Skill)) && (lore.Luck < 0 || item.HasLuck == bool(lore.Luck)) &&
        (lore.MinOption < 0 || item.OptionLevel >= lore.MinOption) && (lore.MaxOption < 0 || item.OptionLevel <= lore.MaxOption) &&
        (lore.MinExcellent < 0 || excellent >= lore.MinExcellent) && (lore.MaxExcellent < 0 || excellent <= lore.MaxExcellent) &&
        (lore.Ancient < 0 || (lore.Ancient == 3 ? (item.AncientDiscriminator & 3) != 0 : (item.AncientDiscriminator & 3) == lore.Ancient));
}

void AppendLore(std::vector<Line>& lines, const Configuration& config, const tagITEM& item, int position)
{
    for (const auto& lore : config.Lore)
    {
        if (lore.Position != position || !Matches(lore, item)) continue;
        for (int blank = 1; blank < lore.BreakLine; ++blank)
            lines.push_back({.kind = Line::Kind::HalfSpacer});
        for (const auto& text : lore.Lines)
        {
            Line line{.text = StringUtils::WideToNarrow(text.c_str()), .bold = lore.Bold};
            ApplyStyle(line, config.Styles[lore.Color]);
            lines.push_back(std::move(line));
        }
    }
}

void AppendBonuses(std::vector<Line>& lines, const Category& category, const tagITEM& item, int tier, int language)
{
    if (category.Bonuses.empty()) return;
    using UI::RmlBridge::Tooltip::LineColor;
    lines.push_back({.kind = Line::Kind::HalfSpacer});
    constexpr const wchar_t* Headings[] = {L"SPECIAL OPTIONS", L"OPÇÕES ESPECIAIS", L"OPCIONES ESPECIALES"};
    lines.push_back({.text = StringUtils::WideToNarrow(Headings[language]), .color = LineColor::Yellow, .bold = true});
    lines.back().sourceColor = TEXT_COLOR_YELLOW;
    constexpr const wchar_t* Locked[] = {L"requires", L"requer", L"requiere"};
    for (const auto& bonus : category.Bonuses)
    {
        const auto& label = Localized(bonus.Label, bonus.LabelTranslations, language);
        wchar_t text[160];
        if (item.Level >= bonus.Level)
            swprintf_s(text, L"%ls +%g%ls", label.c_str(), bonus.Values[tier], bonus.Percent ? L"%" : L"");
        else
            swprintf_s(text, L"%ls +%g%ls (%ls +%d)", label.c_str(), bonus.Values[tier], bonus.Percent ? L"%" : L"", Locked[language], bonus.Level);
        lines.push_back({.text = StringUtils::WideToNarrow(text), .color = item.Level >= bonus.Level ? LineColor::Green : LineColor::Gray});
        lines.back().sourceColor = item.Level >= bonus.Level ? TEXT_COLOR_GREEN : TEXT_COLOR_GRAY;
    }
}
}

void Decorate(UI::RmlBridge::Tooltip::Config& tooltip, const tagITEM& item)
{
    Network::Paris::ItemRarity::Refresh();
    const auto& config = Current();
    const int language = std::clamp(CMultiLanguage::GetSingletonPtr() ? CMultiLanguage::GetSingletonPtr()->GetLanguage() : 0, 0, 2);
    const int tier = std::min(3, ExcellentCount(item.ExcellentFlags));
    const bool categorized = item.Type >= 0 && item.Type < MaximumItemTypes && config.CategoryByItem[item.Type] >= 0;
    std::vector<Line> output;
    if (categorized && !config.Tiers[tier].Name.empty())
    {
        const auto& style = config.Tiers[tier];
        Line label{.text = StringUtils::WideToNarrow(Localized(style.Name, style.NameTranslations, language).c_str()), .bold = true};
        label.customColor = Color(style.Color);
        label.customBackground = Color(style.Background);
        output.push_back(std::move(label));
        tooltip.borderColor = Color(style.Border);
        tooltip.borderWidth = style.BorderWidth;
    }
    AppendLore(output, config, item, 0);
    if (!tooltip.lines.empty())
    {
        output.push_back(std::move(tooltip.lines.front()));
        AppendLore(output, config, item, 1);
        output.insert(output.end(), std::make_move_iterator(tooltip.lines.begin() + 1), std::make_move_iterator(tooltip.lines.end()));
    }
    AppendLore(output, config, item, 2);
    if (config.Enabled && categorized) AppendBonuses(output, config.Categories[config.CategoryByItem[item.Type]], item, tier, language);
    AppendLore(output, config, item, 3);
    AppendLore(output, config, item, 4);
    for (auto& line : output)
    {
        const int color = line.sourceColor;
        if (color >= 0 && color < static_cast<int>(config.Styles.size()))
            ApplyStyle(line, config.Styles[color]);
    }
    tooltip.lines = std::move(output);
}
}
