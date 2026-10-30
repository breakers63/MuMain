#pragma once
#include <string>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <unordered_map>
namespace ParisTvtHud {
inline std::unordered_map<unsigned,int> teams;
inline int seconds=0,blue=0,red=0,team=0;
inline std::chrono::steady_clock::time_point received{};
inline bool Receive(const char* text){
 std::string value=text?text:"";
 const std::string rosterPrefix="[PARIS-TVT-TEAM]";
 if(value.rfind(rosterPrefix,0)==0){unsigned id=0;int assigned=0;char extra=0;auto body=value.substr(rosterPrefix.size());if(std::sscanf(body.c_str(),"%d:%u%c",&assigned,&id,&extra)==2&&assigned>=1&&assigned<=2&&id<32768)teams[id]=assigned;return true;}
 if(value.rfind("[PARIS-TVT]",0)!=0)return false;
 std::istringstream stream(value.substr(11));int values[4]{};std::string field;
 for(auto& n:values){if(!std::getline(stream,field,':')||field.empty()||field.size()>6)return true;for(char c:field)if(c<'0'||c>'9')return true;n=std::stoi(field);}
 if(values[0]>86400||values[1]>999999||values[2]>999999||values[3]<1||values[3]>2)return true;
 seconds=values[0];blue=values[1];red=values[2];team=values[3];received=std::chrono::steady_clock::now();return true;
}
inline bool Receive(const wchar_t* text){std::string s;for(auto p=text;p&&*p;++p)s.push_back((char)*p);return Receive(s.c_str());}
inline bool Visible(){return team>0&&seconds>0&&std::chrono::steady_clock::now()-received<std::chrono::seconds(4);}
inline bool IsEnemy(unsigned id){if(!Visible())return true;auto it=teams.find(id);return it!=teams.end()&&it->second!=team;}
inline std::string Clock(){int elapsed=(int)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-received).count();int left=seconds>elapsed?seconds-elapsed:0;char t[24];std::snprintf(t,sizeof(t),"%02d:%02d",left/60,left%60);return t;}
}
