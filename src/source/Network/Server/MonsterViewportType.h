#pragma once
namespace Network::Viewport
{
    template <typename IsConfigured> int ResolveMonsterType(unsigned char high, unsigned char low, IsConfigured configured)
    {
        const int raw = (static_cast<int>(high) << 8) | low;
        if (configured(raw)) return raw;
        constexpr int NativeTypeMask = 0x03;
        return ((high & NativeTypeMask) << 8) | low;
    }
}
