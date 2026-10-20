#pragma once
#include <string>
#include <vector>
class CUITextInputBox;
namespace UI::ActivityLogs
{
class ActivityLogView
{
  public:
    void Load();
    void Unload();
    void SetOpen(bool open, bool kills = false);
    void Render();
    void Render3D();
    bool Mouse();
    bool CapturesMouse() const;

  private:
    void UpdateFilter();
    void UpdateLayout();
    void DrawScroll();
    void ClearTooltip();
    void RenderItemTooltip();
    ITEM *m_Tooltip = nullptr;
    int m_TooltipIndex = -1;
    unsigned long long m_TooltipRevision = ~0ull;
    void DrawAtlas(int x, int y, int w, int h, int sx, int sy, int sw, int sh);
    void DrawRow(int row, int index);
    void DrawText(int x, int y, int width, const wchar_t *text, bool gold = false, bool bold = false);
    CUITextInputBox *m_Search = nullptr;
    GLuint m_Background = BITMAP_UNKNOWN, m_PostBackground = BITMAP_UNKNOWN;
    GLuint m_Frame = BITMAP_UNKNOWN, m_PostAtlas = BITMAP_UNKNOWN, m_Cutouts = BITMAP_UNKNOWN,
           m_Winner = BITMAP_UNKNOWN;
    GLuint m_Widgets = BITMAP_UNKNOWN, m_Buttons = BITMAP_UNKNOWN;
    void DrawButton(int x, int width, const wchar_t *caption, bool green = false);
    HFONT m_Font = nullptr, m_Bold = nullptr;
    std::wstring m_Query;
    std::vector<int> m_Visible;
    unsigned long long m_Revision = ~0ull;
    int m_First = 0, m_FontSize = 0;
    bool m_All = true, m_Dragging = false, m_Kills = false;
};
ActivityLogView &ActivityLog();
} // namespace UI::ActivityLogs
