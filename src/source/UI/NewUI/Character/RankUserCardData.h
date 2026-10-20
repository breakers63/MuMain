#pragma once
#include <array>
class CHARACTER;
namespace UI::Character {
    struct RankUserCardData {
        const wchar_t* Name=L"";
        const wchar_t* Class=L"";
        const wchar_t* Rank=L"";
        const wchar_t* Guild=L"";
        std::array<int,8> Values{};
        unsigned short Fields=0;
        int BaseClass=-1;
        int GlobalRank=0;
        GLuint Emblem=BITMAP_UNKNOWN;
        bool GuildMember=false;
        const CHARACTER* Character = nullptr;
    };
}
