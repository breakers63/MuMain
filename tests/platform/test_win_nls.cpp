#include <doctest.h>

#include <string>

#include "Core/Platform/WinNls.h"

#ifndef _WIN32

namespace
{
    // "a" + a lone continuation byte + "b": not valid UTF-8.
    constexpr char InvalidUtf8[] = "a\x80" "b";
    // A literal U+FFFD (EF BF BD) is valid input and must not be treated as an error.
    constexpr char ReplacementCharUtf8[] = "\xEF\xBF\xBD";
}

TEST_CASE("MultiByteToWideChar substitutes U+FFFD for invalid UTF-8 without flags [platform][nls]")
{
    wchar_t buffer[8]{};

    const int count = MultiByteToWideChar(CP_UTF8, 0, InvalidUtf8, 3, buffer, 8);

    CHECK(count == 3);
    CHECK(std::wstring(buffer, 3) == std::wstring(L"a�b"));
}

TEST_CASE("MultiByteToWideChar fails on invalid UTF-8 with MB_ERR_INVALID_CHARS [platform][nls]")
{
    wchar_t buffer[8]{};

    CHECK(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, InvalidUtf8, 3, nullptr, 0) == 0);
    CHECK(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, InvalidUtf8, 3, buffer, 8) == 0);
}

TEST_CASE("MultiByteToWideChar accepts a literal U+FFFD with MB_ERR_INVALID_CHARS [platform][nls]")
{
    wchar_t buffer[4]{};

    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ReplacementCharUtf8, 3, buffer, 4);

    CHECK(count == 1);
    CHECK(buffer[0] == L'�');
}

TEST_CASE("MultiByteToWideChar converts valid UTF-8 with MB_ERR_INVALID_CHARS [platform][nls]")
{
    const char text[] = "Espa\xC3\xB1" "a";  // "España"
    wchar_t buffer[8]{};

    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, buffer, 8);

    CHECK(count == 7);  // six characters plus the terminator
    CHECK(std::wstring(buffer) == L"España");
}

TEST_CASE("WideCharToMultiByte fails on a lone surrogate with WC_ERR_INVALID_CHARS [platform][nls]")
{
    const wchar_t text[] = {L'a', static_cast<wchar_t>(0xD800), L'b', L'\0'};
    char buffer[16]{};

    CHECK(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, buffer, 16, nullptr, nullptr) == 0);
    CHECK(WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer, 16, nullptr, nullptr) > 0);
}

TEST_CASE("WideCharToMultiByte converts valid text with WC_ERR_INVALID_CHARS [platform][nls]")
{
    char buffer[16]{};

    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, L"España", -1, buffer, 16, nullptr, nullptr);

    CHECK(count == 8);  // "Espa" + 2-byte ñ + "a" + terminator
    CHECK(std::string(buffer) == "Espa\xC3\xB1" "a");
}

#endif // !_WIN32
