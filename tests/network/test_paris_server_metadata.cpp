#include "doctest.h"
#include "Network/Paris/EventTimerState.h"
#include "Network/Paris/StatServerLimits.h"
TEST_CASE("Event notices synchronize countdown and clear across sessions")
{
    Network::Paris::EventTimerState timers;
    int state=0,seconds=0;
    CHECK_FALSE(timers.Get(10,1000,state,seconds));
    CHECK(timers.Receive(L"[#EVT:U|10|0|3427#]",1000));
    REQUIRE(timers.Get(10,4000,state,seconds));
    CHECK(state==0); CHECK(seconds==3424);
    CHECK(timers.Receive(L"[#EVT:U|10|1|2#]",5000));
    REQUIRE(timers.Get(10,9000,state,seconds)); CHECK(state==1); CHECK(seconds==0);
    CHECK(timers.Receive(L"[#EVT:C#]",10000));
    CHECK_FALSE(timers.Get(10,10000,state,seconds));
    CHECK_FALSE(timers.Receive(L"Normal notice",10000));
}
TEST_CASE("Malformed event notices cannot overwrite valid data")
{
    Network::Paris::EventTimerState timers;
    timers.Receive(L"[#EVT:U|1|0|123#]",0);
    for(const auto* text:{L"[#EVT:U|1|0|-1#]",L"[#EVT:U|1|2|1#]",L"[#EVT:U|1|0|999999999999999999999#]",L"[#EVT:U|1|0|1#]tail"})
    {
        CHECK(timers.Receive(text,0));
        int state=0,seconds=0; REQUIRE(timers.Get(1,0,state,seconds)); CHECK(seconds==123);
    }
}
TEST_CASE("Stat limits accept only a matching complete server reply")
{
    namespace Limits=Network::Paris::ServerLimits;
    Limits::Request=7; Limits::Pending=true; Limits::Ready=false;
    CHECK(Limits::Receive(L"[#PSL1:6|32767|32767|32767|32767|0]")); CHECK_FALSE(Limits::Ready);
    CHECK(Limits::Receive(L"[#PSL1:7|999999999999999999999|1|1|1|0]")); CHECK_FALSE(Limits::Ready);
    CHECK(Limits::Receive(L"[#PSL1:7|32767|32000|31000|30000|0]"));
    CHECK(Limits::Ready); CHECK_FALSE(Limits::Pending); CHECK(Limits::Values[1]==32000);
    Limits::Ready=false;
}
