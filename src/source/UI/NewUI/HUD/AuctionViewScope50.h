#pragma once
#include <cstddef>
namespace UI::Auctions
{
    // Keep the previous reply cached for settings, but never display its rows
    // under a different tab/page. This also guards late replies after a click.
    inline bool MatchesAuctionView(int tab,int page,int subTab,int replyTab,int replyPage,int replySubTab)
    { return tab==replyTab && page==replyPage && subTab==replySubTab; }
}
