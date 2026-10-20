#pragma once
#include "Core/Platform/WinCompat.h"
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <span>
#include <cstring>
#include <algorithm>
#include <cstdint>

namespace Network::Paris::BattlePass
{
    inline constexpr int HeaderSize=80,RewardSize=112,MaximumLevels=100,MaximumRewards=200,MaximumPacketSize=24000,MaximumSockets=5;
    struct Reward
    {
        int Level=0; bool Premium=false,Claimed=false;
        BYTE Type=0; std::int64_t Amount=0;
        std::vector<BYTE> Data; std::wstring Name;
    };
    struct Snapshot
    {
        bool Active=false,Premium=false,PremiumFree=false;
        int Level=0,MaxLevel=0,Points=0,RequiredPoints=0,PremiumItem=0,PremiumCount=0,PremiumItemLevel=0;
        std::int64_t PremiumExpiresUtc=0;
        bool PremiumTermsKnown=false, PremiumRequirementReady=false;
        std::uint64_t ConfigurationToken=0;
        std::array<BYTE,16> Season{};
        std::wstring Name,Message;
        std::vector<Reward> Rewards;
    };
    inline int U16(const BYTE* p) { return p[0]|(p[1]<<8); }
    template<class T> inline T Number(const BYTE* p) { T value;std::memcpy(&value,p,sizeof(value));return value; }
    inline bool Utf8(const BYTE* text,int length,std::wstring& target)
    {
        if(length==0){target.clear();return true;}
        int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(text),length,nullptr,0);
        if(count<1||count>1024)return false;
        target.resize(count);
        return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(text),length,target.data(),count)==count;
    }
    inline bool NativeItemValid(std::span<const BYTE> item)
    {
        if(item.size()<5||item.size()>32)return false;
        const int index=((item[0]&15)<<8)|item[1];
        if(index>=512||item[2]>15)return false;
        std::size_t needed=5;
        for(BYTE flag:{BYTE(1),BYTE(8),BYTE(16),BYTE(32)}) if(item[4]&flag)++needed;
        if(item[4]&128)
        {
            if(needed>=item.size())return false;
            const int sockets=item[needed]&15;
            if(sockets>MaximumSockets)return false;
            needed+=1+sockets;
        }
        return needed<=item.size();
    }
    inline std::optional<Reward> DecodeReward(std::span<const BYTE> row,int maxLevel)
    {
        Reward reward;
        reward.Level=U16(row.data());reward.Premium=row[2]!=0;reward.Claimed=row[3]!=0;reward.Type=row[4];
        reward.Amount=Number<std::int64_t>(row.data()+8);
        if(reward.Level<1||reward.Level>maxLevel||row[2]>1||row[3]>1||reward.Type<2||reward.Type>6
            ||reward.Amount<1||reward.Amount>INT32_MAX||row[5]>32)return std::nullopt;
        const BYTE* end=std::find(row.data()+48,row.data()+RewardSize,0);
        if(end==row.data()+RewardSize||!Utf8(row.data()+48,static_cast<int>(end-row.data()-48),reward.Name))return std::nullopt;
        if(row[5])
        {
            auto item=row.subspan(16,row[5]);
            if(reward.Type!=6||!NativeItemValid(item))return std::nullopt;
            reward.Data.assign(item.begin(),item.end());
        }
        return reward;
    }
    inline std::optional<Snapshot> Decode(std::span<const BYTE> packet)
    {
        const int size=static_cast<int>(packet.size());
        if(size<HeaderSize||size>MaximumPacketSize)return std::nullopt;
        const BYTE* p=packet.data();
        if(p[0]!=0xC2||((p[1]<<8)|p[2])!=size||p[3]!=0xFA||p[4]!=0xB1||p[5]!=1||p[6]>1||p[7]>1||p[46]>1)return std::nullopt;
        Snapshot result;
        result.Level=U16(p+8);result.MaxLevel=U16(p+10);result.Points=Number<int>(p+12);result.RequiredPoints=Number<int>(p+16);
        result.PremiumCount=Number<int>(p+48);result.PremiumItem=Number<int>(p+52);result.ConfigurationToken=Number<std::uint64_t>(p+72);
        result.PremiumItemLevel=p[56];result.PremiumExpiresUtc=Number<std::int64_t>(p+36);
        if(p[47]==1){const auto days=Number<std::int16_t>(p+57);if(days < -1 || days > 3650 || p[59]>1)return std::nullopt;
            result.PremiumTermsKnown=true;result.PremiumRequirementReady=p[59]==1;}
        const int count=U16(p+44),messageLength=U16(p+68),nameLength=U16(p+70);
        if(result.MaxLevel<1||result.MaxLevel>MaximumLevels||result.Level>result.MaxLevel||count>MaximumRewards||result.Points<0||result.RequiredPoints<0
            ||result.PremiumCount<1||result.PremiumCount>1000||result.PremiumItem<0||result.PremiumItem>8191||p[56]>15||result.ConfigurationToken==0
            ||messageLength>1024||nameLength>256||size!=HeaderSize+nameLength+messageLength+RewardSize*count)return std::nullopt;
        if(!Utf8(p+HeaderSize,nameLength,result.Name)||!Utf8(p+HeaderSize+nameLength,messageLength,result.Message))return std::nullopt;
        std::copy(p+20,p+36,result.Season.begin());
        if(std::all_of(result.Season.begin(),result.Season.end(),[](BYTE b){return b==0;}))return std::nullopt;
        std::array<bool,MaximumRewards> seen{};
        int offset=HeaderSize+nameLength+messageLength;
        for(int i=0;i<count;++i,offset+=RewardSize)
        {
            auto reward=DecodeReward(packet.subspan(offset,RewardSize),result.MaxLevel);
            if(!reward)return std::nullopt;
            const int key=(reward->Level-1)*2+reward->Premium;
            if(seen[key])return std::nullopt;
            seen[key]=true;result.Rewards.push_back(std::move(*reward));
        }
        result.Active=p[6]!=0;result.Premium=p[7]!=0;result.PremiumFree=p[46]!=0;
        return result;
    }
}
