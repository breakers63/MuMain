#include <doctest.h>
#include <initializer_list>
#include "GameLogic/Social/GestureWheelGeometry.h"
TEST_CASE("All gesture, emoji and instrument sectors select their stable action ids")
{
    using namespace UI::Gestures;
    CHECK(Pick(0,0)==-1);CHECK(Pick(1.1,0)==-1);
    for(int id=0;id<37;++id)
    {
        const int count=id<9?9:id<25?16:id<32?7:5;
        const int sector=id<9?id:id<25?id-9:id<32?id-25:id-32;
        const double radius=id<9?.285:id<25?.52:id<32?.735:.915;
        for(double offset:{-.49,0.,.49})
        {
            double a=(sector+offset)*Tau/count;
            CHECK(Pick(std::sin(a)*radius,-std::cos(a)*radius)==id);
        }
    }
}
