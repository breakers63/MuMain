#include "doctest.h"
#include "Network/Paris/ActiveInvasionProtocol.h"
using Network::Paris::ActiveInvasion::Decode;
TEST_CASE("active invasion decodes the server updates without leaking chat")
{
 auto value=Decode(L"[#AINV:U|2|283|5|5|26928|Golden Wheel|Golden Invasion#]");
 REQUIRE(value);CHECK(value->event==2);CHECK(value->monster==283);CHECK(value->alive==5);CHECK(value->seconds==26928);CHECK(value->eventName==L"Golden Invasion");CHECK_FALSE(value->ended);
 auto cleared=Decode(L"[#AINV:C#]");REQUIRE(cleared);CHECK(cleared->cleared);
 auto ended=Decode(L"[#AINV:E|2#]");REQUIRE(ended);CHECK(ended->ended);
}
TEST_CASE("malformed invasion packets cannot change the HUD")
{
 for(auto text:{L"hello",L"[#AINV:U|2#]",L"[#AINV:E|0#]",L"[#AINV:E|-1#]",L"[#AINV:E|9999999999#]",L"[#AINV:U|2|283|6|5|1|Mob|Event#]",L"[#AINV:U|2|283|5|5|999999999|Mob|Event#]",L"[#AINV:U|2|283|5|5|1||Event#]",L"[#AINV:U|2|283|5|5|1|Mob|Event|extra#]"})CHECK_FALSE(Decode(text));
}
