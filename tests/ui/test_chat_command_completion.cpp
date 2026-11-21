#include <doctest.h>
#include "UI/Chat/CommandCompletion.h"
TEST_CASE("Slash completion filters prefixes, operator commands and argument hints")
{
    UI::Chat::CommandCompletion state;
    CHECK(state.Sync(L"/",false,true));CHECK(state.Count()>10);
    for(int i=0;i<state.Count();++i) CHECK_FALSE(state.Row(i).gm);
    const int regular=state.Count();state.Sync(L"/",true,true);CHECK(state.Count()>regular);
    state.Sync(L"/POST",false,true);REQUIRE(state.Count()==1);
    CHECK(std::wstring(state.Row(0).name)==L"/post");CHECK(state.Complete(0)==L"/post ");
    state.Sync(L"/post hello",false,true);REQUIRE(state.Count()==1);CHECK(state.Complete(0)==L"/post hello");
    state.Sync(L"/setmoney",false,true);CHECK(state.Count()==0);
    state.Sync(L"/setmoney",true,true);CHECK(state.Count()==1);
    state.Sync(L"normal text",true,true);CHECK(state.Count()==0);
    state.Sync(L"/",true,false);CHECK(state.Count()==0);
    state.Sync(L"/notacommand",true,true);CHECK(state.Count()==0);
}
TEST_CASE("Slash navigation scrolls, wraps and keeps the draft until acceptance")
{
    UI::Chat::CommandCompletion state;state.Sync(L"/",true,true);
    state.Move(-1);CHECK(state.Selected()==state.Count()-1);CHECK(state.First()+state.VisibleRows==state.Count());
    state.Move(1);CHECK(state.Selected()==0);CHECK(state.First()==0);
    CHECK_FALSE(state.Sync(L"/",true,true));CHECK(state.Selected()==0);
    for(int i=0;i<9;++i)state.Move(1);
    CHECK(state.Selected()==9);CHECK(state.First()==2);
    CHECK(state.Complete(-1)==L"/");
    state.Sync(L"/party player",false,true);CHECK(state.Complete(0)==L"/party player");
}
