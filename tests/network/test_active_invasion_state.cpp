#include "doctest.h"
#include "GameLogic/Events/ActiveInvasionState.h"
#include <memory>
using GameLogic::Events::ActiveInvasionState;
TEST_CASE("Invasion notices update counts and timer without entering chat")
{
    auto state=std::make_unique<ActiveInvasionState>();
    CHECK_FALSE(state->Apply(L"Normal chat",1000));
    REQUIRE(state->Apply(L"[#AINV:U|2|79|16|16|90|Golden Dragon|Golden Invasion#]",1000));
    REQUIRE(state->events[0].used);
    CHECK(state->events[0].id==2);
    CHECK(state->events[0].deadline==91000);
    CHECK(state->events[0].monsters[0].id==79);
    CHECK(state->events[0].monsters[0].alive==16);
    state->Apply(L"[#AINV:U|2|79|3|16|80|Golden Dragon|Golden Invasion#]",2000);
    CHECK(state->events[0].monsters[0].alive==3);
    CHECK_FALSE(state->events[0].monsters[1].used);
    state->Apply(L"[#AINV:E|2#]",3000);
    CHECK_FALSE(state->events[0].used);
}
TEST_CASE("Invalid invasion counters and overflow do not create events")
{
    auto state=std::make_unique<ActiveInvasionState>();
    for(const auto* message:{L"[#AINV:U|2|79|17|16|90|Dragon|Invasion#]",
        L"[#AINV:U|2|79|-1|16|90|Dragon|Invasion#]",L"[#AINV:U|999999999999999999999|79|1|16|90|Dragon|Invasion#]",
        L"[#AINV:U|2|79|1|16|90|Dragon|Invasion|extra#]",L"[#AINV:U|2|79|1|16|90|Dragon|Invasion"})
    {
        CHECK(state->Apply(message,0));
        CHECK_FALSE(state->events[0].used);
    }
}
TEST_CASE("Invasion state clears all events and supports bounded names")
{
    auto state=std::make_unique<ActiveInvasionState>();
    state->Apply(L"[#AINV:U|2|79|1|16|90|Dragon|Gold#]",0);
    state->Apply(L"[#AINV:U|3|275|2|3|100|Kundun|Kundun Invasion#]",0);
    CHECK(state->events[1].used);
    state->Apply(L"[#AINV:C#]",0);
    CHECK_FALSE(state->events[0].used);
    CHECK_FALSE(state->events[1].used);
}
