#include "doctest.h"
#include "Render/Weather/PuddleShape.h"
using namespace Render::Weather::Puddles;
TEST_CASE("Puddles remain anchored to their map cell and have a closed irregular edge")
{
    const auto seed=Seed(40,52,0);
    CHECK(seed==Seed(40,52,0));CHECK(seed!=Seed(40,52,3));
    CHECK(EdgeRadius(0,seed)==doctest::Approx(EdgeRadius(6.28318530718f,seed)));
    for(int i=0;i<100;++i){const auto radius=EdgeRadius(i*.1f,seed);CHECK(radius>=.65f);CHECK(radius<=1.01f);}
}
TEST_CASE("Puddles disappear on dry ground and feather to transparent at the edge")
{
    CHECK(Opacity(0,0)==0);CHECK(Opacity(.12f,.5f)==0);CHECK(Opacity(1,1)==0);
    CHECK(Opacity(1,0)>.3f);CHECK(Opacity(.5f,.5f)<Opacity(1,.5f));
}

TEST_CASE("Puddle rings share terrain samples without changing their topology")
{
    struct Point {float angle,radius;};
    int calls=0;
    const auto points=SamplePoints([&](float angle,float radius) {++calls;return Point{angle,radius};});
    CHECK(calls==1+(Rings.size()-1)*Segments);
    CHECK(calls==61);
    for(int i=0;i<Segments;++i) {
        CHECK(points[0][i].radius==0.f);
        CHECK(points.back()[i].radius==1.f);
        CHECK(points.back()[i].angle==doctest::Approx(i*Tau/Segments));
    }
    CHECK((points.size()-1)*Segments*6==360);
}
