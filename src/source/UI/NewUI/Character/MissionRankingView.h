#pragma once
#include "UI/NewUI/Quests/NewUICustomQuestCenter.h"
#include <array>
#include <string>
#include "ParisExactAssets.h"
namespace UI::Character
{
class MissionRankingView
{
  public:
    void Load();
    void Release();
    void Open();
    void Clear();
    void SetLayout(int x, int y, float scale);
    void Render(const std::wstring &search);
    void RenderBackground();
    void RenderFrame();
    bool Mouse();
    bool Receive(const BYTE *packet, int size);
    bool MonsterPreview(SEASON3B::CustomQuestMonsterPreviewInfo &preview, int row = -1) const;
    void ReceiveGlobal(const BYTE *packet, int size, int payloadOffset);
    void AdvanceScroll();
    int ListTop() const
    {
        return Y(157);
    }
    int ListBottomPosition() const
    {
        return Y(511);
    }
    UI::Theme::ParisExactAssets m_ScrollArt;
    float m_Scroll = 0, m_TargetScroll = 0, m_GrabOffset = 0;
    DWORD m_ScrollTick = 0;
    void SetGlobalRank(int rank)
    {
        m_GlobalRank = rank;
    }

  private:
    struct Entry
    {
        wchar_t Name[11]{};
        int Progress = 0, Elapsed = -1;
    };
    void SyncCatalog(const std::wstring &query);
    void Select(int index);
    void Request();
    void DrawList();
    void DrawDetails();
    void Atlas(int x, int y, int w, int h, int sx, int sy, int sw, int sh);
    void Text(int x, int y, int width, const wchar_t *text, bool gold = false, bool bold = false);
    int X(int v) const;
    int Y(int v) const;
    int W(int v) const;
    int H(int v) const;
    bool Hit(int x, int y, int w, int h) const;
    GLuint m_Background = BITMAP_UNKNOWN, m_Medals[3]{BITMAP_UNKNOWN, BITMAP_UNKNOWN, BITMAP_UNKNOWN};
    GLuint m_Coins = BITMAP_UNKNOWN;
    HFONT m_Font = nullptr, m_Bold = nullptr, m_Title = nullptr;
    GLuint m_Atlas = BITMAP_UNKNOWN, m_Frame = BITMAP_UNKNOWN;
    int m_X = 0, m_Y = 0, m_First = 0, m_Selected = -1, m_Category = -1, m_Count = 0;
    int m_GlobalRank = 0;
    float m_Scale = 1;
    DWORD m_RequestTick = 0;
    DWORD m_CatalogRequestTick = 0;
    bool m_Pending = false, m_Deferred = false, m_Dragging = false, m_Failed = false;
    std::wstring m_Query;
    std::uint64_t m_Revision = ~0ull;
    std::vector<SEASON3B::CustomQuestDefinitionView> m_Catalog;
    std::vector<int> m_Filtered;
    std::array<Entry, 50> m_Entries{};
};
MissionRankingView &MissionRanking();
} // namespace UI::Character
