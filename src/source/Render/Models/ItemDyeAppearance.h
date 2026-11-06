#pragma once
#include <unordered_map>
namespace Render::ItemDye
{
struct Equipped { CHARACTER* Character=nullptr; short Key=-1; std::unordered_map<int,int> Colors; };
inline std::unordered_map<DWORD,int> Inventory;
inline std::unordered_map<const OBJECT*,Equipped> World;
inline int Preview=-1;
inline bool IsPreview=false;
struct PreviewScope
{
    int Previous;
    bool PreviousIsPreview;
    explicit PreviewScope(int color, bool preview=true):Previous(Preview),PreviousIsPreview(IsPreview)
    { Preview=color; IsPreview=preview; }
    ~PreviewScope(){Preview=Previous; IsPreview=PreviousIsPreview;}
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
    // Preserve light intensity without multiplying the dye by the old hue.
    const float intensity = source[0] > source[1] ? source[0] : source[1];
    const float neutralLight = intensity > source[2] ? intensity : source[2];
    for (int channel = 0; channel < 3; ++channel)
        target[channel] = (neutralLight + (1.0f - neutralLight) * 0.25f)
            * ((color >> ((2 - channel) * 8)) & 255) / 255.f;
    return true;
}
inline void Clear(){Inventory.clear();World.clear();Preview=-1;}
}
