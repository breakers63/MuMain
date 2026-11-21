#include <doctest.h>

#include <cstring>

#include "Core/Platform/WinGuid.h"

#ifndef _WIN32

TEST_CASE("CLSIDFromString parses a braced GUID into the Win32 field layout [platform][guid]")
{
    GUID guid{};

    const HRESULT result = CLSIDFromString(L"{01234567-89AB-cdef-0123-456789ABCDEF}", &guid);

    REQUIRE(SUCCEEDED(result));
    CHECK(guid.Data1 == 0x01234567u);
    CHECK(guid.Data2 == 0x89ABu);
    CHECK(guid.Data3 == 0xCDEFu);
    const unsigned char expectedData4[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    CHECK(std::memcmp(guid.Data4, expectedData4, sizeof(expectedData4)) == 0);
}

TEST_CASE("CLSIDFromString rejects malformed text [platform][guid]")
{
    GUID guid{};

    CHECK(FAILED(CLSIDFromString(L"01234567-89AB-CDEF-0123-456789ABCDEF", &guid)));    // no braces
    CHECK(FAILED(CLSIDFromString(L"{01234567-89AB-CDEF-0123-456789ABCDEG}", &guid)));  // not hex
    CHECK(FAILED(CLSIDFromString(L"{01234567+89AB-CDEF-0123-456789ABCDEF}", &guid)));  // bad dash
    CHECK(FAILED(CLSIDFromString(L"{01234567-89AB-CDEF-0123-456789ABCDE}", &guid)));   // too short
    CHECK(FAILED(CLSIDFromString(nullptr, &guid)));
}

TEST_CASE("CoCreateGuid produces distinct version 4 GUIDs [platform][guid]")
{
    GUID first{}, second{};

    REQUIRE(SUCCEEDED(CoCreateGuid(&first)));
    REQUIRE(SUCCEEDED(CoCreateGuid(&second)));

    CHECK(std::memcmp(&first, &second, sizeof(GUID)) != 0);
    CHECK((first.Data3 & 0xF000u) == 0x4000u);       // version 4
    CHECK((first.Data4[0] & 0xC0u) == 0x80u);        // RFC 4122 variant
    CHECK(FAILED(CoCreateGuid(nullptr)));
}

#endif // !_WIN32
