#include "doctest.h"
#include "Network/Server/ServerListScript.h"
#include "Network/Paris/WarpRequirements.h"
#include <algorithm>
#include <vector>
#include <memory>
#include <cstring>

TEST_CASE("Server-list descriptions reject oversized and truncated data without overflowing")
{
    using namespace Network::ServerListScript;
    std::unique_ptr<FILE,decltype(&std::fclose)> file(std::tmpfile(),&std::fclose);
    REQUIRE(file!=nullptr);
    const char input[]="test";
    REQUIRE(std::fwrite(input,1,4,file.get())==4);
    std::rewind(file.get());
    char buffer[4]={};
    CHECK_FALSE(ReadDescription(file.get(),-1,buffer));
    CHECK_FALSE(ReadDescription(file.get(),32767,buffer));
    CHECK(std::ftell(file.get())==0);
    CHECK(ReadDescription(file.get(),4,buffer));
    CHECK(std::memcmp(input,buffer,4)==0);
    CHECK_FALSE(ReadDescription(file.get(),1,buffer));
    CHECK(ReadDescription(file.get(),0,buffer));
    CHECK_FALSE(ReadDescription(nullptr,0,buffer));
}

TEST_CASE("Server warp limits decode atomically and accept matching aliases and reject conflicting destinations")
{
    using namespace Network::Paris::Warp;
    std::vector<std::uint8_t> packet(32);
    const std::uint8_t header[]{0xC2,0,32,0xFA,0xC1,1,1,0};
    std::copy(std::begin(header),std::end(header),packet.begin());
    const auto write=[&packet](int offset,int value)
    {
        for(int i=0;i<4;++i)packet[offset+i]=static_cast<std::uint8_t>(static_cast<std::uint32_t>(value)>>(i*8));
    };
    write(8,1);write(12,200);write(16,400);write(20,10);write(24,20);write(28,5000);
    Requirements.clear();
    REQUIRE(Receive(packet));
    REQUIRE(Find(1));
    CHECK(Find(1)->level==200);
    CHECK(Find(1)->minimumReset==10);
    CHECK(Find(1)->maximumReset==20);
    CHECK(Find(1)->zen==5000);
    CHECK_FALSE(Receive(std::span(packet).first(20)));
    CHECK(Find(1)->minimumReset==10);
    const auto entry=std::vector<std::uint8_t>(packet.begin()+8,packet.end());
    packet.insert(packet.end(),entry.begin(),entry.end());packet[2]=56;packet[6]=2;
    CHECK(Receive(packet));
    CHECK(Requirements.size()==1);
    write(44, 18); // The second alias contradicts the first minimum-reset requirement.
    CHECK_FALSE(Receive(packet));
    CHECK(Find(1)->minimumReset==10);
    Requirements.clear();
}
