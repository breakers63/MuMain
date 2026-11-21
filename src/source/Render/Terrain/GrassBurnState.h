#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace Render::Terrain
{
class GrassBurnState
{
public:
    static constexpr int Size = 256;
    static constexpr int MaximumFires = 6;
    static constexpr std::uint32_t RegrowMs = 300000;
    struct Fire { float x = 0, y = 0, radius = 0; std::uint32_t started = 0; bool active = false; };
    using FuelProbe = bool (*)(int, int);
    void Reset() { burned.fill(0); fires.fill({}); nextTick = 0; }
    void Ignite(float x, float y, std::uint32_t now, FuelProbe fuel)
    {
        if (!std::isfinite(x) || !std::isfinite(y) || x < 0 || y < 0 || x >= Size || y >= Size || !fuel(static_cast<int>(x), static_cast<int>(y))) return;
        for (const auto& fire : fires)
            if (fire.active && std::hypot(fire.x-x,fire.y-y)<2.f) return;
        for (auto& fire : fires)
            if (!fire.active) { fire = {x,y,1.2f,now,true}; Stamp(fire,now,fuel); return; }
    }
    void Step(std::uint32_t now, FuelProbe fuel)
    {
        if (nextTick && static_cast<std::int32_t>(now-nextTick)<0) return;
        nextTick = now + 100;
        for (auto& fire : fires)
        {
            if (!fire.active) continue;
            const float age = (now-fire.started)*.001f;
            fire.radius = std::min(7.f,1.2f + age*.55f);
            Stamp(fire,now,fuel);
            if (age >= 11.f) fire.active = false;
        }
    }
    float Amount(int x, int y, std::uint32_t now) const
    {
        if (x<0 || y<0 || x>=Size || y>=Size) return 0;
        const auto stamp = burned[y*Size+x];
        if (!stamp) return 0;
        const auto age = now-(stamp-1);
        if (age>=RegrowMs) return 0;
        return 1.f-static_cast<float>(age)/RegrowMs;
    }
    const auto& Fires() const { return fires; }
private:
    bool Stamp(const Fire& fire, std::uint32_t now, FuelProbe fuel)
    {
        bool found = false;
        const int reach=static_cast<int>(std::ceil(fire.radius*1.3f));
        for(int y=std::max(0,static_cast<int>(fire.y)-reach);y<=std::min(Size-1,static_cast<int>(fire.y)+reach);++y)
        for(int x=std::max(0,static_cast<int>(fire.x)-reach);x<=std::min(Size-1,static_cast<int>(fire.x)+reach);++x)
        {
            const float dx=x+.5f-fire.x,dy=y+.5f-fire.y;
            const float angle=std::atan2(dy,dx);
            const float edge=fire.radius*(1.f+.18f*std::sin(angle*3+fire.x)+.1f*std::sin(angle*5+fire.y));
            if(dx*dx+dy*dy>edge*edge || !fuel(x,y))continue;
            auto& stamp=burned[y*Size+x];
            if(!stamp || now-(stamp-1)>=RegrowMs) {stamp=now+1;found=true;}
        }
        return found;
    }
    std::array<std::uint32_t,Size*Size> burned{};
    std::array<Fire,MaximumFires> fires{};
    std::uint32_t nextTick=0;
};
}
