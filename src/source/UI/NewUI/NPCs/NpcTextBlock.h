#pragma once
#include <span>
#include <string>
#include <vector>
#include <cstdint>
namespace UI::NPCs
{
    class NpcTextBlock
    {
    public:
        void Render(std::span<const wchar_t* const> paragraphs, int x, int y, int width, int height);
    private:
        void Rebuild(std::span<const wchar_t* const> paragraphs, int pixelWidth);
        std::vector<std::wstring> m_sources;
        std::vector<std::wstring> m_lines;
        std::uintptr_t m_font = 0;
        int m_pixelWidth = 0;
    };
}
