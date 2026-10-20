#pragma once
#include <array>
#include <string>
#include <vector>
namespace UI::Character
{
class GuildRankingView
{
  public:
    void Load();
    void Release();
    void Clear();
    void Request(const std::wstring &guild);
    void Retry();
    bool Receive(const BYTE *packet, int size);
    void SetLayout(int x, int y, float physicalScale);
    bool HandleClick();
    void RenderBackground();
    void Render(const std::wstring &guild, const std::wstring &master, const std::array<BYTE, 32> &mark, int rank,
                int score, int count);

  private:
    struct Member
    {
        wchar_t Name[11]{};
        BYTE Class{}, Position{};
        int Level{}, Resets{}, Stats{}, Rank{};
        std::array<int, 5> Attributes{};
        bool DetailsAvailable = false, OnlineKnown = false, Online = false;
    };
    void SendRequest();
    void UpdateRequestState();
    static Member DecodeMember(const BYTE *record, int version);
    void RenderStats(int rank, int score, int count, const std::wstring &master);
    void DrawText(int x, int y, int width, const wchar_t *text, bool bold = false, int alignment = 0);
    void DrawTitle(int x, int y, int width, const wchar_t *text);
    void DrawPortrait(const Member &member, int x, int y, int width, int height);
    void RenderCards();
    void RenderPager();
    void DrawPagerButton(int x, bool next, bool enabled);
    void RenderSelectedMember();
    void DrawMemberPanel(const Member &member);
    void DrawMemberDetails(const Member &member, const wchar_t *className);
    void RenderEmblem(const std::array<BYTE, 32> &mark);
    int X(int value) const;
    int Y(int value) const;
    int W(int value) const;
    int H(int value) const;
    bool Hit(int x, int y, int width, int height) const;
    GLuint m_Background = BITMAP_UNKNOWN, m_Buttons = BITMAP_UNKNOWN;
    HFONT m_Font = nullptr, m_Bold = nullptr, m_Title = nullptr;
    int m_X = 0, m_Y = 0, m_Page = 0, m_SelectedMember = 0, m_FontSize = 0;
    float m_Scale = 1.f;
    DWORD m_RequestedAt = 0;
    bool m_Pending = false, m_Received = false;
    bool m_RequestFailed = false;
    BYTE m_ProtocolVersion = 2;
    std::wstring m_Guild;
    std::vector<Member> m_Members;
};
GuildRankingView &GuildRanking();
} // namespace UI::Character
