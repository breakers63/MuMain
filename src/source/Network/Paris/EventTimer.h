#pragma once
#include <string>
#include <string_view>
#include <chrono>
#include <algorithm>
namespace Network::Paris::EventTimer {
inline int event = -1, state = -1, seconds = 0;
inline std::chrono::steady_clock::time_point received{};
inline bool Receive(const wchar_t* text) {
 constexpr std::wstring_view prefix=L"[PARIS-EVENT-TIMER]";
 const std::wstring_view value(text ? text : L""); if(!value.starts_with(prefix))return false;
 int kind,phase,remaining; wchar_t extra;
 if(swscanf_s(text+prefix.size(),L"%d:%d:%d%c",&kind,&phase,&remaining,&extra,1)==3
   && kind>=1 && kind<=3 && phase>=0 && phase<=4 && remaining>=0 && remaining<=86400) {
  event=kind;state=phase;seconds=remaining;received=std::chrono::steady_clock::now();
 }
 return true;
}
inline bool Visible(bool blood,bool devil,bool chaos) {
 return event>=1 && ((event==1 && devil)||(event==3 && chaos))
  && std::chrono::steady_clock::now()-received<std::chrono::seconds(8) && seconds>0;
}
inline std::wstring Text() {
 const int elapsed=static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-received).count());
 const int left=std::max(0,seconds-elapsed);wchar_t time[32]{};swprintf_s(time,L"%02d:%02d",left/60,left%60);
 return std::wstring(event==1?L"DEVIL SQUARE":event==2?L"BLOOD CASTLE":L"CHAOS CASTLE")+L"\n"+time;
}
}
