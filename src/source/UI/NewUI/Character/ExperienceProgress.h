#pragma once
#include <algorithm>

namespace UI::Character
{
    inline double ExperienceProgress(long long current,long long next,long long level,bool master)
    {
        long long lower=0;
        if(master)
        {
            const long long total=level+400,over=total-255;
            lower=((9+total)*total*total*10+(9+over)*over*over*1000-3892250000LL)/2;
        }
        else if(level>1)
        {
            const long long prior=level-1,over=prior-255;
            lower=(9+prior)*prior*prior*10;
            if(over>0) lower+=(9+over)*over*over*1000;
        }
        if(next<=lower) return 0;
        return std::clamp((static_cast<double>(current)-lower)/(static_cast<double>(next)-lower),0.0,1.0);
    }
}
