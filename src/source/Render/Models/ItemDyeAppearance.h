#pragma once
#include <unordered_map>
namespace Render::ItemDye
{
struct Equipped { CHARACTER* Character=nullptr; short Key=-1; std::unordered_map<int,int> Colors; };
inline std::unordered_map<DWORD,int> Inventory;
inline std::unordered_map<const OBJECT*,Equipped> World;
inline int Preview=-1;
struct PreviewScope
{
    int Previous;
    explicit PreviewScope(int color):Previous(Preview){Preview=color;}
    ~PreviewScope(){Preview=Previous;}
};
inline int Color(const ITEM* item)
{
    auto found=Inventory.find(item->Key);
    return found==Inventory.end()?-1:found->second;
}
inline int Resolve(const OBJECT* object,int modelType)
{
    if(Preview>=0)return Preview;
    auto found=World.find(object);
    if(found==World.end() || found->second.Character->Key!=found->second.Key)return -1;
    auto tint=found->second.Colors.find(modelType);
    return tint==found->second.Colors.end()?-1:tint->second;
}
inline bool Light(const OBJECT* object,int modelType,const float* source,float* target)
{
    const int color=Resolve(object,modelType);
    if(color<0)return false;
    for(int channel=0;channel<3;++channel)
        target[channel]=source[channel]*((color >> ((2-channel)*8)) & 255)/255.f;
    return true;
}
inline void Clear(){Inventory.clear();World.clear();Preview=-1;}
}
