#pragma once
#include <string>
#include <string_view>
namespace Network::Paris::BloodPvpNotice {
inline std::wstring text;
inline unsigned long long started = 0;
inline std::string Utf8() { int size = WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr); std::string result(size,0); WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),result.data(),size,nullptr,nullptr); return result; }
inline bool Receive(const wchar_t* message) {
    constexpr wchar_t prefix[] = L"[BLOOD-PVP]";
    if (!message || std::wstring_view(message).substr(0,11) != prefix) return false;
    text = message + 11; started = GetTickCount64(); return true;
}
}
