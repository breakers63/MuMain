#pragma once
#include <array>
#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>
#include <sstream>
namespace UI::BuffDetails {
struct Entry { std::wstring text; long long seconds=-1; std::chrono::steady_clock::time_point received{}; };
inline std::array<Entry,256> entries;
inline void Clear() {entries={};}
inline long long Remaining(int id) {
 if(id<0||id>=256||entries[id].seconds<0)return -1;
 return std::max(0LL,entries[id].seconds-std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-entries[id].received).count());
}
inline std::wstring Time(int id) {
 const auto seconds=Remaining(id); if(seconds<0)return L"";
 return std::to_wstring(seconds/86400)+L" d "+std::to_wstring((seconds/3600)%24)+L" h "+std::to_wstring((seconds/60)%60)+L" min "+std::to_wstring(seconds%60)+L" s";
}
inline bool Receive(std::wstring_view message) {
 constexpr std::wstring_view prefix=L"~buff.detail~",remove=L"~buff.remove~";
 if(message.starts_with(remove)) {
  const auto value=message.substr(remove.size()); int id=0;
  if(value.empty()||value.size()>3)return true;
  for(auto c:value) {if(c<L'0'||c>L'9')return true;id=id*10+c-L'0';}
  if(id<256)entries[id]={}; return true;
 }
 if(!message.starts_with(prefix))return false;
 auto value=message.substr(prefix.size());auto split=value.find(L'|');if(split==std::wstring_view::npos)return true;
 int id=0;std::wistringstream key(std::wstring{value.substr(0,split)});if(!(key>>id)||!(key>>std::ws).eof()||id<0||id>=256)return true;
 value=value.substr(split+1);split=value.find(L'|');if(split==std::wstring_view::npos)return true;
 long long seconds=0;std::wistringstream time(std::wstring{value.substr(0,split)});if(!(time>>seconds)||!(time>>std::ws).eof()||seconds< -1||seconds>3155760000LL)return true;
 entries[id]={std::wstring(value.substr(split+1,180)),seconds,std::chrono::steady_clock::now()};return true;
}
}
