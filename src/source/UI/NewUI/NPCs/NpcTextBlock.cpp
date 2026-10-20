#include "stdafx.h"
#include "NpcTextBlock.h"
#include "UI/Legacy/UIControls.h"
#include "Core/Text/TextLineWrap.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include <algorithm>
namespace UI::NPCs
{
    void NpcTextBlock::Rebuild(std::span<const wchar_t* const> paragraphs, int pixelWidth)
    {
        m_sources.assign(paragraphs.begin(), paragraphs.end());
        m_font = reinterpret_cast<std::uintptr_t>(g_hFont);
        m_pixelWidth = pixelWidth;
        m_lines.clear();
        for (const auto* paragraph : paragraphs)
        {
            auto lines = WrapTextToWidth(paragraph, pixelWidth, [](const wchar_t* text, size_t length)
                { return static_cast<int>(g_pRenderText->MeasureText(text, static_cast<int>(length)).cx); });
            m_lines.insert(m_lines.end(), std::make_move_iterator(lines.begin()), std::make_move_iterator(lines.end()));
        }
    }
    void NpcTextBlock::Render(std::span<const wchar_t* const> paragraphs, int x, int y, int width, int height)
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(225, 219, 205, 255);
        const int pixelWidth = static_cast<int>(width * g_fScreenRate_x);
        if (pixelWidth != m_pixelWidth || m_font != reinterpret_cast<std::uintptr_t>(g_hFont)
            || !std::equal(paragraphs.begin(), paragraphs.end(), m_sources.begin(), m_sources.end()))
            Rebuild(paragraphs, pixelWidth);
        constexpr int LineGap = 1;
        const int lineHeight = std::max(1, static_cast<int>(g_pRenderText->MeasureText(L"Ag", 2).cy / g_fScreenRate_y)) + LineGap;
        UI::Paris::Clip(x, y, width, height);
        for (const auto& line : m_lines)
        {
            if (lineHeight > height) break;
            g_pRenderText->RenderText(x, y, line.c_str(), width, 0, RT3_SORT_CENTER);
            y += lineHeight;
            height -= lineHeight;
        }
        UI::Paris::ResetClip();
    }
}
