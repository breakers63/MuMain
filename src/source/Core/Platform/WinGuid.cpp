// Non-Windows implementation of the GUID shim (WinGuid.h).
#ifndef _WIN32

#include "Core/Platform/WinGuid.h"

#include <cwchar>   // wcslen
#include <exception>
#include <random>

namespace
{
    // "{" + 8-4-4-4-12 hex digits with dashes + "}".
    constexpr std::size_t GuidTextLength = 38;
    constexpr std::size_t DashPositions[] = {9, 14, 19, 24};

    // RFC 4122 version 4 / variant 1 marker bits.
    constexpr std::uint16_t VersionMask = 0x0FFF;
    constexpr std::uint16_t Version4 = 0x4000;
    constexpr std::uint8_t VariantMask = 0x3F;
    constexpr std::uint8_t Variant1 = 0x80;

    int HexValue(wchar_t c)
    {
        if (c >= L'0' && c <= L'9') return c - L'0';
        if (c >= L'a' && c <= L'f') return c - L'a' + 10;
        if (c >= L'A' && c <= L'F') return c - L'A' + 10;
        return -1;
    }

    bool IsDashPosition(std::size_t position)
    {
        for (const std::size_t dash : DashPositions)
            if (position == dash) return true;
        return false;
    }

    // Reads the 32 hex digits of a braced GUID into 16 bytes, in text order.
    bool ReadHexBytes(const wchar_t* text, std::uint8_t (&bytes)[16])
    {
        std::size_t byteIndex = 0;
        bool highNibble = true;
        for (std::size_t i = 1; i + 1 < GuidTextLength; ++i)
        {
            if (IsDashPosition(i))
            {
                if (text[i] != L'-') return false;
                continue;
            }
            const int value = HexValue(text[i]);
            if (value < 0) return false;
            if (highNibble) bytes[byteIndex] = static_cast<std::uint8_t>(value << 4);
            else bytes[byteIndex++] |= static_cast<std::uint8_t>(value);
            highNibble = !highNibble;
        }
        return true;
    }

    std::uint32_t BigEndian32(const std::uint8_t* b)
    {
        return (std::uint32_t{b[0]} << 24) | (std::uint32_t{b[1]} << 16) | (std::uint32_t{b[2]} << 8) | b[3];
    }

    std::uint16_t BigEndian16(const std::uint8_t* b)
    {
        return static_cast<std::uint16_t>((b[0] << 8) | b[1]);
    }
}

HRESULT CoCreateGuid(GUID* guid)
{
    if (!guid) return E_INVALIDARG;
    try
    {
        std::random_device device;
        std::uniform_int_distribution<unsigned int> byteDistribution(0, 0xFF);
        std::uint8_t bytes[16];
        for (std::uint8_t& byte : bytes)
            byte = static_cast<std::uint8_t>(byteDistribution(device));

        guid->Data1 = BigEndian32(bytes);
        guid->Data2 = BigEndian16(bytes + 4);
        guid->Data3 = static_cast<std::uint16_t>((BigEndian16(bytes + 6) & VersionMask) | Version4);
        for (int i = 0; i < 8; ++i)
            guid->Data4[i] = bytes[8 + i];
        guid->Data4[0] = static_cast<std::uint8_t>((guid->Data4[0] & VariantMask) | Variant1);
        return S_OK;
    }
    catch (const std::exception&)
    {
        return E_FAIL;  // std::random_device has no entropy source
    }
}

HRESULT CLSIDFromString(const wchar_t* text, GUID* guid)
{
    if (!text || !guid) return E_INVALIDARG;
    if (std::wcslen(text) != GuidTextLength) return E_INVALIDARG;
    if (text[0] != L'{' || text[GuidTextLength - 1] != L'}') return E_INVALIDARG;

    std::uint8_t bytes[16];
    if (!ReadHexBytes(text, bytes)) return E_INVALIDARG;

    guid->Data1 = BigEndian32(bytes);
    guid->Data2 = BigEndian16(bytes + 4);
    guid->Data3 = BigEndian16(bytes + 6);
    for (int i = 0; i < 8; ++i)
        guid->Data4[i] = bytes[8 + i];
    return S_OK;
}

#endif // !_WIN32
