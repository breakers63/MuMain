#include "doctest.h"
#include "Network/Server/ServerListScript.h"
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
