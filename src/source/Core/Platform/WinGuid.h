// Portable GUID shim.
//
// Request identities sent to the server are Win32 GUIDs: a 16-byte struct with
// the <guiddef.h> layout, created with CoCreateGuid and parsed with
// CLSIDFromString. On Windows these come from <objbase.h>; elsewhere this
// provides the same layout and portable implementations (WinGuid.cpp), so the
// bytes put on the wire are identical on every platform.
#pragma once

#ifdef _WIN32

#include <objbase.h>

#else  // ---- non-Windows ----------------------------------------------------

#include "Core/Platform/WinCompat.h"  // HRESULT

#include <cstdint>

#ifndef GUID_DEFINED
#define GUID_DEFINED
// Fixed-width members: Data1 is a 32-bit `unsigned long` on Windows, but
// `unsigned long` is 64-bit on Linux, which would change sizeof(GUID).
struct GUID
{
    std::uint32_t Data1;
    std::uint16_t Data2;
    std::uint16_t Data3;
    std::uint8_t Data4[8];
};
#endif
static_assert(sizeof(GUID) == 16, "GUID must keep the 16-byte Win32 layout");

// A random (version 4) GUID. Returns S_OK, or E_FAIL if no randomness is available.
HRESULT CoCreateGuid(GUID* guid);

// Parses "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" (hex digits, any case).
// Returns S_OK, or E_INVALIDARG for any other text.
HRESULT CLSIDFromString(const wchar_t* text, GUID* guid);

#endif // _WIN32
