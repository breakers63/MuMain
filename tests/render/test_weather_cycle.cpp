#include "doctest.h"
#include "Render/Weather/WeatherCycle.h"
using namespace Render::Weather;
TEST_CASE("weather defaults preserve the existing world")
{
 Settings s;
 for(int map : {0,2,3,1,4}) {auto state=Evaluate(s,map,0,true);CHECK(state.kind==Kind::Clear);CHECK(state.rain==0);CHECK(state.night==0);CHECK(state.lightning==0);}
}
TEST_CASE("weather cycles keep full durations and a clear interval")
{
 Settings s;s.rain=true;s.storm=true;s.rainMinutes=1;s.stormMinutes=5;
 CHECK(ScheduledKind(s,0,59999)==Kind::Rain);
 CHECK(ScheduledKind(s,0,60000)==Kind::Clear);
 CHECK(ScheduledKind(s,0,120000)==Kind::Storm);
 CHECK(ScheduledKind(s,0,419999)==Kind::Storm);
 CHECK(ScheduledKind(s,0,420000)==Kind::Clear);
 CHECK(ScheduledKind(s,0,480000)==Kind::Rain);
 CHECK(ScheduledKind(s,1,0)==Kind::Clear);
 s.snowMinutes=1;CHECK(ScheduledKind(s,2,0)==Kind::Snow);CHECK(ScheduledKind(s,2,120000)==Kind::Blizzard);
}
TEST_CASE("shelters suppress precipitation and lightning")
{
 Settings s;s.storm=true;s.dayNight=true;s.dayNightMinutes=1;
 auto outside=Evaluate(s,0,0,true);CHECK(outside.rain==1);CHECK(outside.lightning==1);
 auto inside=Evaluate(s,0,0,false);CHECK(inside.rain==0);CHECK(inside.lightning==0);CHECK_FALSE(inside.exposed);
 CHECK(Evaluate(s,0,30000,true).night==doctest::Approx(1));CHECK(Evaluate(s,0,60000,true).night==doctest::Approx(0));
 CHECK(Evaluate(s,1,30000,true).night==0);
}
TEST_CASE("duration controls cycle through supported values")
{
 CHECK(NextDuration(1,-1)==120);CHECK(NextDuration(120,1)==1);
 CHECK(NextDuration(15,1)==30);CHECK(NextDuration(15,-1)==5);
}
