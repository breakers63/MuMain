#include "ActiveInvasionState.h"
#include <algorithm>
#include <limits>
namespace GameLogic::Events
{
    namespace
    {
        int Number(std::wstring_view text)
        {
            if (text.empty()) return -1;
            int value=0;
            for (wchar_t c:text)
            {
                if(c<L'0'||c>L'9'||value>(std::numeric_limits<int>::max()-(c-L'0'))/10) return -1;
                value=value*10+c-L'0';
            }
            return value;
        }
        template<size_t N> void Copy(std::array<wchar_t,N>& target,std::wstring_view source)
        {
            target.fill(0);
            std::copy_n(source.begin(),std::min(source.size(),N-1),target.begin());
        }
    }
    void ActiveInvasionState::Clear() { for(auto& event:events) event=Event{}; }
    void ActiveInvasionState::Update(const std::array<std::wstring_view,8>& f,std::uint64_t now)
    {
        const int id=Number(f[1]), monsterId=Number(f[2]), alive=Number(f[3]), total=Number(f[4]), seconds=Number(f[5]);
        constexpr int MaxId=65535, MaxCount=1000000, MaxSeconds=604800;
        if(id<0||id>MaxId||monsterId<0||monsterId>MaxId||alive<0||total<alive||total>MaxCount||seconds<0||seconds>MaxSeconds) return;
        Event* event=nullptr;
        for(auto& candidate:events) if(candidate.used&&candidate.id==id) {event=&candidate;break;}
        if(!event) for(auto& candidate:events) if(!candidate.used) {event=&candidate;break;}
        if(!event) return;
        Monster* monster=nullptr;
        for(auto& candidate:event->monsters) if(candidate.used&&candidate.id==monsterId) {monster=&candidate;break;}
        if(!monster) for(auto& candidate:event->monsters) if(!candidate.used) {monster=&candidate;break;}
        if(!monster) return;
        event->used=true; event->id=id; event->deadline=now+static_cast<std::uint64_t>(seconds)*1000;
        Copy(event->name,f[7]);
        monster->used=true; monster->id=monsterId; monster->alive=alive; monster->total=total; Copy(monster->name,f[6]);
    }
    bool ActiveInvasionState::Apply(std::wstring_view text,std::uint64_t now)
    {
        constexpr std::wstring_view Prefix=L"[#AINV:";
        if(!text.starts_with(Prefix)) return false;
        if(text.size()>512||!text.ends_with(L"#]")) return true;
        text=text.substr(Prefix.size(),text.size()-Prefix.size()-2);
        std::array<std::wstring_view,8> fields{};size_t count=0;
        while(count<fields.size())
        {
            auto end=text.find(L'|');fields[count++]=text.substr(0,end);
            if(end==text.npos) break;
            if(count==fields.size()) return true;
            text.remove_prefix(end+1);
        }
        if(fields[0]==L"C"&&count==1) Clear();
        else if(fields[0]==L"E"&&count==2) {int id=Number(fields[1]);for(auto& event:events)if(event.used&&event.id==id)event=Event{};}
        else if(fields[0]==L"U"&&count==8) Update(fields,now);
        return true;
    }
}
