#pragma once
#include "Render/Text/CUIRenderText.h"
#include <string>
#include <string_view>
#include <cstdio>
#include <unordered_map>
#include <cmath>
namespace Render::NameRgb {
struct Style { int Mode=0, First=0x33FFFF, Last=0xCC55FF, Speed=4; bool operator==(const Style&) const = default; };
struct NameHash {
    using is_transparent=void;
    size_t operator()(std::wstring_view value) const noexcept { return std::hash<std::wstring_view>{}(value); }
};
using Map=std::unordered_map<std::wstring,Style,NameHash,std::equal_to<>>;
inline Map Styles;
inline Style Own;
inline int Color(const Style& style, float position, unsigned now) {
    constexpr float AnimationPeriod=10000.f;
    float t=style.Mode==1?0.f:position;
    if(style.Mode==3)t=.5f-.5f*std::cos((position+now*style.Speed/AnimationPeriod)*6.2831853f);
    int rgb=0;
    for(int shift: {16,8,0}) {
        const int a=(style.First>>shift)&255,b=(style.Last>>shift)&255;
        rgb|=static_cast<int>(a+(b-a)*t+.5f)<<shift;
    }
    return rgb;
}
inline const std::string& Markup(const wchar_t* name)
{
    struct Cache { Style Profile; unsigned Tick=~0u;std::string Html; };
    static std::unordered_map<std::wstring,Cache,NameHash,std::equal_to<>> cache;
    const auto found=Styles.find(name);
    const Style profile=found==Styles.end()?Style{}:found->second;
    auto cached=cache.find(name);
    if(cached==cache.end())cached=cache.emplace(name,Cache{}).first;
    auto& value=cached->second;
    const unsigned now=timeGetTime(),tick=profile.Mode==3?now/100:0;
    if(value.Tick==tick&&value.Profile==profile)return value.Html;
    value.Profile=profile;value.Tick=tick;value.Html.clear();
    const size_t length=std::wcslen(name);
    for(size_t index=0;index<length;++index) {
        if(profile.Mode!=0) {
            char tag[40];std::snprintf(tag,sizeof(tag),"<span style=\"color:#%06X;\">",Color(profile,length>1?float(index)/float(length-1):0.f,now));
            value.Html+=tag;
        }
        char utf8[8]{};const wchar_t letter[2]={name[index],0};
        const int bytes=WideCharToMultiByte(CP_UTF8,0,letter,1,utf8,sizeof(utf8),nullptr,nullptr);
        if(name[index]==L'<')value.Html+="&lt;";else if(name[index]==L'>')value.Html+="&gt;";
        else if(name[index]==L'&')value.Html+="&amp;";else value.Html.append(utf8,bytes);
        if(profile.Mode!=0)value.Html+="</span>";
    }
    return value.Html;
}
inline void DrawStyled(const wchar_t* text,int x,int y,int width,int height,const Style& style) {
    const auto saved=g_pRenderText->GetTextColor();
    const size_t length=std::wcslen(text);
    for(size_t i=0;i<length;++i) {
        const int rgb=Color(style,length>1?float(i)/float(length-1):0.f,timeGetTime());
        g_pRenderText->SetTextColor((rgb>>16)&255,(rgb>>8)&255,rgb&255,255);
        const wchar_t letter[2]={text[i],0};SIZE drawn{};
        g_pRenderText->RenderText(x,y,letter,0,height,RT3_SORT_LEFT,&drawn);
        x+=drawn.cx;
    }
    g_pRenderText->SetTextColor(saved);
}
inline bool Draw(const wchar_t* text,int x,int y,int width,int height) {
    const auto found=Styles.find(text);
    if(found==Styles.end()||found->second.Mode==0)return false;
    DrawStyled(text,x,y,width,height,found->second);return true;
}
}
