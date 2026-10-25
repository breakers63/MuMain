#include <doctest.h>
#include <fstream>
#include <filesystem>
#include "Render/Textures/OztFile.h"
#include <vector>
TEST_CASE("Gesture art loads through the actual OZT decoder with transparent corners and opaque gold frame")
{
    using namespace Render::Textures::Ozt;
    std::ifstream file(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "src/bin/Data/Interface/Paris_GestureWheel.OZT",std::ios::binary);
    REQUIRE(file);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),{});
    auto header=ReadHeader(bytes,1024,1024);
    REQUIRE(header.status==Status::Valid);
    CHECK(header.width==512);CHECK(header.height==512);
    std::vector<unsigned char> rgba(512*512*4);
    CopyPixels(bytes,header,rgba,512);
    CHECK(rgba[3]==0);
    CHECK(rgba[(256*512+256)*4+3]>200);
    int gold=0;
    for(size_t p=0;p<rgba.size();p+=4)
        if(rgba[p]>150 && rgba[p+1]>100 && rgba[p+2]<150 && rgba[p+3]>200)++gold;
    CHECK(gold>1000);
}
