#include "stdafx.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/HUD/ParisOverlayInput.h"
#include "MentorPanel.h"
#include "I18N/All.h"
#include "UI/NewUI/HUD/NewUIBuffWindow.h"
#include "Network/Paris/MentorProtocol.h"
#include "UI/NewUI/HUD/GmEventsPanel.h"
#include "UI/NewUI/Character/PanelAspectScale.h"
#include "UI/NewUI/Character/ClassPortraits.h"
#include "UI/NewUI/Widgets/ParisTintOverlay.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Legacy/UIControls.h"
#include "Render/Textures/ZzzTexture.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include <memory>
namespace UI::Mentor
{
namespace {
using namespace Network::Paris::Mentor;
constexpr int Width=1536, Height=1024;
GLuint texture=BITMAP_UNKNOWN, findTexture=BITMAP_UNKNOWN, homeTexture=BITMAP_UNKNOWN, rewardsTexture=BITMAP_UNKNOWN, historyTexture=BITMAP_UNKNOWN, portraitTexture=BITMAP_UNKNOWN;
int resetFilter=0,historyFilter=0;
const char* historyFilters[]={"all","completed","ended","rewards"};
std::wstring lastSearch;
std::unique_ptr<CUITextInputBox> nameInput;
HFONT normal=nullptr,bold=nullptr,small=nullptr;
bool visible=false,requested=false,inviteInput=false;
int tab=0,page=0,selected=0,fontSize=0,ox=0;
float scale=1;
DWORD lastRequest=0;
constexpr DWORD OpeningTimeout=8000;
DWORD openedAt=0; bool timeoutReported=false;
std::wstring message;
std::string confirmation, selectedIdentity;
int confirmCost=0, offerSelected=0;
unsigned long long confirmToken=0;
ITEM* tooltip=nullptr;
int tooltipRow=-1; DWORD tooltipRevision=0;
void ClearTooltip(){if(tooltip&&g_pNewItemMng)g_pNewItemMng->DeleteItem(tooltip);tooltip=nullptr;tooltipRow=-1;tooltipRevision=0;}
int W(int value){return UI::Character::PanelAspect::Axis(value,scale,g_fScreenRate_x);}
int H(int value){return UI::Character::PanelAspect::Axis(value,scale,g_fScreenRate_y);}
int X(int value){return ox+W(value);} int Y(int value){return H(value);}
bool Hit(int x,int y,int w,int h){return SEASON3B::CheckMouseIn(X(x),Y(y),W(w),H(h));}
std::wstring Label(const char* key){auto found=Data.Labels.find(key);return found==Data.Labels.end()?L"":found->second;}
std::wstring PlainText(std::wstring value){
 const std::wstring accents=L"áàâäãåÁÀÂÄÃÅéèêëÉÈÊËíìîïÍÌÎÏóòôöõÓÒÔÖÕúùûüÚÙÛÜñÑçÇ";
 const std::wstring ascii=L"aaaaaaAAAAAAeeeeEEEEiiiiIIIIoooooOOOOOuuuuUUUUnNcC";
 for(auto& letter:value){auto index=accents.find(letter);if(index!=std::wstring::npos)letter=ascii[index];}
 return value;
}
void Text(int x,int y,int width,const std::wstring& value,bool gold=false,bool strong=false,int alignment=RT3_SORT_LEFT){
 EnableAlphaTest();UI::Paris::Color(1,1,1,1);g_pRenderText->SetFont(strong?g_hFontBold:g_hFont);
 g_pRenderText->SetBgColor(0,0,0,0);g_pRenderText->SetTextColor(gold?240:255,gold?211:255,gold?138:255,255);
 auto clean=PlainText(value);g_pRenderText->RenderText(X(x),Y(y),clean.c_str(),W(width),0,alignment);
}
void Glow(int x,int y,int w,int h){UI::Theme::TintOverlay(X(x),Y(y),W(w),H(h),1,.8f,.25f,.16f);}
void Cover(int x,int y,int w,int h){UI::Theme::TintOverlay(X(x),Y(y),W(w),H(h),.025f,.025f,.02f,.98f);}
void Layout(){scale=UI::Character::PanelAspect::Fit(WindowWidth,WindowHeight*.88f,Width,Height);ox=static_cast<int>((WindowWidth-Width*scale)/(2*g_fScreenRate_x));nameInput->SetFont(g_hFont);nameInput->SetPosition(X(tab==1?196:160),Y(tab==1?291:750));nameInput->SetSize(W(tab==1?403:584),H(28));}
void Send(BYTE action=0,const std::string& id={},const wchar_t* name=L""){std::wstring filter;
 if(action==0&&tab==1){wchar_t value[11]{};nameInput->GetText(value,11);filter=value;filter+=L"|"+std::to_wstring(Data.Minimum+resetFilter);name=filter.c_str();}
 if(action==0&&tab==3){filter=std::wstring(historyFilters[historyFilter],historyFilters[historyFilter]+strlen(historyFilters[historyFilter]));name=filter.c_str();}
 Request(action,static_cast<BYTE>(tab),static_cast<BYTE>(page),id,name);lastRequest=timeGetTime();}
int PageCount(){int total=tab==0?Data.RelationCount:tab==1?Data.MentorCount:tab==2?Data.OfferCount:Data.HistoryCount;const int rows=tab==1||tab==2||tab==3?6:tab==0&&Data.IsMentor?7:3;return std::max(1,(total+rows-1)/rows);}
void Button(int x,int y,int w,int h,const std::wstring& text,bool enabled=true){if(enabled&&Hit(x,y,w,h))Glow(x,y,w,h);if(!enabled)UI::Theme::TintOverlay(X(x),Y(y),W(w),H(h),0,0,0,.6f);Text(x+8,y+(h-25)/2,w-16,text,false,false,RT3_SORT_CENTER);}
void Portrait(int characterClass,int x,int y,int w,int h){
 if(characterClass<0||portraitTexture==BITMAP_UNKNOWN)return;
 int tile=5;const int base=characterClass>>2;
 if(base>=3&&base<=5){UI::Character::Portraits::RenderPortrait(base,X(x),Y(y),W(w),H(h));return;}
 if(base==0)tile=characterClass>=1?2:4;else if(base==1)tile=characterClass>=6?0:1;else if(base==2)tile=3;
 EnableAlphaTest();UI::Paris::Color(1,1,1,1);RenderBitmap(portraitTexture,X(x),Y(y),W(w),H(h),(tile%3)*.25f,(tile/3)*.5f,.25f,.5f);
}
void FindMentors(){
 Text(218,185,350,Label("MentorRank")+L": "+Label(Data.IsMentor?"MentorMentorRole":"MentorNoviceRole"));
 Text(696,185,340,Label("MentorMentorRole")+L": "+(Data.AssignedMentor.empty()?Label("MentorUnassigned"):Data.AssignedMentor));
 Text(1196,185,212,Label("MentorRequests")+L": "+std::to_wstring(Data.Requests));
 Text(150,241,710,Label("MentorAvailable"),true,true);Text(918,241,485,Label("MentorTop"),true,true);
 Button(679,280,147,43,std::to_wstring(Data.Minimum+resetFilter)+L"+ RESETS");
 for(size_t i=0;i<Data.Mentors.size();++i){const auto& row=Data.Mentors[i];int y=335+static_cast<int>(i)*84;
  if(row.Class>=0)Portrait(row.Class,145,y+7,76,63);
  Text(242,y+14,280,row.Name,false,true);Text(247,y+45,277,row.ClassName+L" - "+Label("MentorLevel")+L" "+std::to_wstring(row.Level));Text(424,y+14,100,row.Online?Label("MentorOnline"):Label("MentorOfflineStatus"),row.Online);
  Text(544,y+14,78,std::to_wstring(row.Resets),true,true,RT3_SORT_CENTER);Text(544,y+45,78,L"RESETS",true,false,RT3_SORT_CENTER);
  Button(637,y+15,213,48,!row.Id.empty()?row.Accepted?row.Status:Label(Data.LinkIncoming?"MentorAccept":"MentorCancel"):Label("MentorRequest"),Data.Enabled&&(!row.Id.empty()&&!row.Accepted||Data.CanRequest&&row.Novices<row.Maximum));
 }
 const int topY[]={291,401,512,627,717,811};
 for(size_t i=0;i<Data.Top.size();++i){const auto& row=Data.Top[i];int y=topY[i],height=i<3?85:70;
  if(row.Class>=0)Portrait(row.Class,1005,y+10,77,height-8);
  Text(1113,y+14,150,row.Name,false,true);Text(1145,y+43,120,row.ClassName);Text(1145,y+(i<3?69:59),120,row.GuildName,true);
  Text(1278,y+20,126,Label("MentorGraduated"),false,false,RT3_SORT_CENTER);Text(1278,y+49,126,std::to_wstring(row.GraduatedCount),true,true,RT3_SORT_CENTER);
 }
 Button(195,853,202,45,Label("MentorPrevious"),page>0);Button(603,853,202,45,Label("MentorNext"),page+1<PageCount());
 Text(441,865,134,std::to_wstring(page+1)+L" / "+std::to_wstring(PageCount()),true,false,RT3_SORT_CENTER);
 Text(195,918,Data.LinkId.empty()?655:427,message.empty()?Data.LinkId.empty()?Label("MentorRequestHelp"):Data.LinkName+L": "+Data.LinkState:message);if(!Data.LinkId.empty()&&!Data.LinkAccepted){RenderBitmap(texture,X(684),Y(911),W(170),H(38),1291.f/2048.f,886.f/1024.f,125.f/2048.f,59.f/1024.f);Button(684,911,170,38,Label(Data.LinkIncoming?"MentorReject":"MentorCancel"));}
 nameInput->SetState(UISTATE_NORMAL);nameInput->Render();wchar_t value[11]{};nameInput->GetText(value,11);
 if(!*value&&!nameInput->HaveFocus())Text(196,291,403,Label("MentorSearch"));
 if(lastSearch!=value){lastSearch=value;page=0;if(timeGetTime()-lastRequest>=1000)Send();}
}
std::wstring RewardName(const Reward& reward){
 if(reward.Type==6 && reward.Group>=0 && reward.Group<16 && reward.Number>=0 && reward.Number<512){wchar_t name[256]{};GetItemName(reward.Group*512+reward.Number,reward.Level,name);return std::to_wstring(reward.Amount)+L" x "+name;}
 return std::to_wstring(reward.Amount)+L" "+(reward.Type==3?L"WC":reward.Type==4?L"WP":reward.Type==5?L"GP":L"Zen");
}
void OfferDetails(){Cover(137,278,646,434);Text(155,302,600,Label("MentorRewards"),true,true);if(Data.Offers.empty()){Text(155,350,600,Label("MentorEmpty"));return;}const auto& offer=Data.Offers[std::min(offerSelected,static_cast<int>(Data.Offers.size())-1)];Text(155,345,606,offer.Name+L"  -  "+std::to_wstring(offer.Cost)+L" "+Label("MentorPointUnit"),true,true);for(size_t i=0;i<offer.Rewards.size();++i)Text(155,386+static_cast<int>(i)*23,606,RewardName(offer.Rewards[i]));}
void PersonRows(){
 const auto& rows=Data.Relations;
 for(size_t i=0;i<rows.size();++i){const auto& row=rows[i];int y=272+static_cast<int>(i)*83;
  if(static_cast<int>(i)==selected||Hit(130,y,660,74))Glow(130,y,660,74);
  Portrait(row.Class,145,y+7,79,62);Text(244,y+13,375,row.Name,false,true);
  Text(247,y+42,133,std::to_wstring(row.Resets)+L" / "+std::to_wstring(row.Goal)+L" resets",true);
  Cover(385,y+37,224,14);float progress=std::min(1.f,static_cast<float>(row.Resets)/std::max(1,row.Goal));UI::Theme::TintOverlay(X(385),Y(y+37),W(static_cast<int>(222*progress)),H(14),row.Graduated?1.f:.08f,.9f,.15f,.9f);
  Text(674,y+28,109,row.Status,row.Graduated);
 }
 if(rows.empty()){Text(247,315,510,Label("MentorEmpty"));return;}
 const auto& row=rows[std::min(selected,static_cast<int>(rows.size())-1)];Portrait(row.Class,145,544,116,98);
 Text(285,544,478,row.Name+L": "+Label("MentorProgress"),true,true);Text(285,574,470,std::to_wstring(row.Resets)+L" / "+std::to_wstring(row.Goal)+L" resets");
 float progress=std::min(1.f,static_cast<float>(row.Resets)/std::max(1,row.Goal));UI::Theme::TintOverlay(X(289),Y(600),W(static_cast<int>(468*progress)),H(14),.08f,.9f,.15f,.9f);Text(285,626,470,row.Status);
 const char* keys[]={"MentorSameParty","MentorSameMap","MentorNear"};bool flags[]={row.Party,row.Map,row.Near};const int positions[]={208,415,622};
 for(int i=0;i<3;++i)Text(positions[i],682,128,Label(keys[i])+(flags[i]?L"  OK":L"  -"),flags[i]);
 Text(360,749,399,L"+"+std::to_wstring(row.Party&&row.Map&&row.Near?Data.NoviceBonus:0)+L"% EXP",true,true);
 if(row.Incoming){Button(143,660,195,58,Label("MentorAccept"));Button(348,660,195,58,Label("MentorReject"));}
}
void Offers(){
 Text(837,239,535,Label("MentorRewards"),true,true);Text(837,273,535,Label("MentorRewardHelp"));
 for(size_t index=0;index<Data.Offers.size();++index){const auto& offer=Data.Offers[index];int y=304+static_cast<int>(index)*143;
  Text(984,y+14,265,offer.Name,false,true);std::wstring summary;int amounts[4]{};
  for(const auto& reward:offer.Rewards){if(!summary.empty())summary+=L" + ";summary+=std::to_wstring(reward.Amount)+L" "+(reward.Type==3?L"WC":reward.Type==4?L"WP":reward.Type==5?L"GP":reward.Type==6?L"item":L"Zen");if(reward.Type>=3&&reward.Type<=5)amounts[reward.Type-3]+=reward.Amount;else if(reward.Type==6)amounts[3]+=reward.Amount;}
  Text(984,y+46,266,summary);const int positions[]={1020,1092,1145,1220};for(int i=0;i<4;++i)Text(positions[i],y+99,35,std::to_wstring(amounts[i]));
  Text(1264,y+20,143,std::to_wstring(offer.Cost)+L" "+Label("MentorPointUnit"),true,false,RT3_SORT_CENTER);Button(1273,y+59,130,53,Label("MentorRedeem"),Data.Enabled&&Data.IsMentor&&!Data.Pending&&Data.Points>=offer.Cost);
 }
 if(Data.Offers.empty())Text(864,341,510,Label("MentorEmpty"));
 Text(994,759,392,Data.Pending?Label("MentorPendingDelivery"):Label("MentorBalance")+L": "+std::to_wstring(std::max(0,Data.Points-(Data.Offers.empty()?0:Data.Offers[std::min(offerSelected,static_cast<int>(Data.Offers.size())-1)].Cost)))+L" "+Label("MentorPoints"));
 Text(850,823,541,Label("MentorRecent"),true,true);for(size_t i=0;i<Data.Recent.size();++i)Text(867,862+static_cast<int>(i)*34,419,Data.Recent[i].Kind+L"  "+Data.Recent[i].Name);
}
std::wstring RelativeAge(long long stamp){
 if(stamp<=0)return {};auto minutes=std::max(0LL,(static_cast<long long>(time(nullptr))-stamp)/60);
 if(minutes==0)return Label("MentorAgeNow");auto text=Label(minutes>=60?"MentorAgeHours":"MentorAgeMinutes");auto token=text.find(L"{0}");if(token!=std::wstring::npos)text.replace(token,3,std::to_wstring(minutes>=60?minutes/60:minutes));return text;
}
std::vector<int> HomeOrder(){std::vector<int> rows;if(Data.Relations.empty())return rows;for(int i=0;i<static_cast<int>(Data.Relations.size());++i)if(Data.Relations[i].Id==selectedIdentity)selected=i;selected=std::clamp(selected,0,static_cast<int>(Data.Relations.size())-1);selectedIdentity=Data.Relations[selected].Id;rows.push_back(selected);for(int i=0;i<static_cast<int>(Data.Relations.size());++i)if(i!=selected)rows.push_back(i);return rows;}
void MentorHome(){
 Text(125,251,630,Label("MentorIncomingRequests")+L"  "+std::to_wstring(Data.Requests),true,true);
 Text(885,251,560,Label("MentorSummary"),true,true);
 auto order=HomeOrder();
 for(size_t i=0;i<order.size();++i){const auto& row=Data.Relations[order[i]];int y=i==0?307:472+(static_cast<int>(i)-1)*84;
  Portrait(row.Class,78,y+5,i==0?82:68,i==0?72:61);
  Text(i==0?195:181,y+12,330,row.Name,false,true);
  Text(i==0?195:181,y+43,330,row.ClassName,true);
  Text(535,y+12,195,L"Lv "+std::to_wstring(row.Level)+L" / "+std::to_wstring(row.Resets)+L" resets");
  Text(535,y+43,195,row.Accepted?row.Status:RelativeAge(row.Created),true);
  if(i==0){bool consent=Data.Enabled&&row.Incoming&&!row.Accepted&&!row.Graduated;Button(130,400,270,50,Label("MentorAccept"),consent);Button(422,400,273,50,Label("MentorReject"),consent);}
 }
 if(order.empty())Text(185,333,535,Label("MentorEmpty"));
 Text(868,332,582,Label("MentorActiveNovices")+L"   "+std::to_wstring(Data.ActiveNovices)+L" / "+std::to_wstring(Data.MaximumNovices),true,true,RT3_SORT_CENTER);
 float fill=std::min(1.f,static_cast<float>(Data.ActiveNovices)/std::max(1,Data.MaximumNovices));UI::Theme::TintOverlay(X(850),Y(386),W(static_cast<int>(600*fill)),H(27),.12f,.8f,.12f,.7f);
 Text(928,477,197,Label("MentorPoints"),true);Text(950,522,175,std::to_wstring(Data.Points),true,true);
 Text(1250,477,205,Label("MentorGraduated"),true);Text(1290,522,165,std::to_wstring(Data.Graduated),true,true);
 Text(928,593,197,Label("MentorPartyHours"),true);Text(950,644,175,std::to_wstring(static_cast<int>(Data.HelpSeconds/3600))+L" h",true,true);
 Text(1250,593,205,Label("MentorProgress"),true);Text(1290,644,165,std::to_wstring(static_cast<int>(Data.NoviceProgress))+L"%",true,true);
 Text(892,727,535,Label("MentorRecent"),true,true);for(size_t i=0;i<Data.Recent.size();++i){int y=790+static_cast<int>(i)*46;Text(845,y,400,Data.Recent[i].Kind+L" - "+Data.Recent[i].Name);Text(1260,y,190,RelativeAge(Data.Recent[i].Time));}
 Button(752,296,32,35,Label("MentorPrevious"),page>0);Button(752,950,32,35,Label("MentorNext"),page+1<PageCount());
 Text(95,985,650,message,true);
}
void RewardsGrid(){
 for(size_t i=0;i<Data.Offers.size();++i){const auto& offer=Data.Offers[i];int x=65+static_cast<int>(i%3)*471,y=222+static_cast<int>(i/3)*344;
  Text(x+20,y+9,415,offer.Name,true,true,RT3_SORT_CENTER);
  std::wstring summary;for(const auto& reward:offer.Rewards){if(!summary.empty())summary+=L" + ";summary+=RewardName(reward);}
  Text(x+24,y+207,403,summary,false,false,RT3_SORT_CENTER);Text(x+24,y+238,403,std::to_wstring(offer.Cost)+L" "+Label("MentorPointUnit"),true,true,RT3_SORT_CENTER);
  bool redeem=Data.Enabled&&Data.IsMentor&&!Data.Pending&&Data.Points>=offer.Cost;
  RenderBitmap(rewardsTexture,X(x+53),Y(y+265),W(359),H(54),118.f/2048.f,(redeem?489.f:830.f)/1024.f,359.f/2048.f,54.f/1024.f);
  auto caption=Label("MentorMissingPoints");auto placeholder=caption.find(L"{0}");if(placeholder!=std::wstring::npos)caption.replace(placeholder,3,std::to_wstring(std::max(0,offer.Cost-Data.Points)));
  Button(x+53,y+265,359,54,redeem?Label("MentorRedeem"):Data.Pending?Label("MentorPendingDelivery"):caption,redeem);
 }
 for(int i=static_cast<int>(Data.Offers.size());i<6;++i){int x=65+(i%3)*471,y=222+(i/3)*344;Text(x+20,y+9,415,Label("MentorEmpty"),true,false,RT3_SORT_CENTER);Button(x+53,y+265,359,54,Label("MentorEmpty"),false);}
 Button(180,927,230,45,Label("MentorPrevious"),page>0);Button(1100,927,230,45,Label("MentorNext"),page+1<PageCount());
 Text(460,942,585,message.empty()?Label("MentorDeductOnConfirm"):message,true,false,RT3_SORT_CENTER);
}
void Confirmation(){if(confirmation.empty())return;Cover(353,385,810,267);Text(397,415,700,Label("MentorConfirm"),true,true);Text(397,461,700,Label("MentorCost")+L" "+std::to_wstring(confirmCost)+L" / "+Label("MentorPoints")+L" "+std::to_wstring(Data.Points),true);Button(413,556,280,57,Label("MentorConfirm"));Button(791,556,280,57,Label("MentorCancel"));}
void RenderPending()
{
 Layout(); EnableAlphaTest(); UI::Paris::Color(1,1,1,1);
 RenderBitmap(texture,X(0),Y(0),W(Width),H(Height),0,0,.75f,1);
 Cover(120,165,1294,785);
 const bool timeout=timeGetTime()-openedAt>=OpeningTimeout;
 if(timeout&&!timeoutReported){Trace("open-timeout");timeoutReported=true;}
 Text(210,450,1110,timeout?I18N::Game::MentorUnavailable:I18N::Game::MentorConnecting,true,true,RT3_SORT_CENTER);
 g_pRenderText->SetFont(g_hFont);
}
std::wstring Date(long long stamp,bool full=false){if(stamp<=0)return L"-";time_t time=static_cast<time_t>(stamp);tm date{};localtime_s(&date,&time);wchar_t out[40]{};wcsftime(out,40,full?L"%d/%m/%Y %H:%M":L"%d/%m %H:%M",&date);return out;}
void History(){
 Text(105,250,660,Label("MentorHistory"),true,true);Text(860,250,585,Label("MentorHistoryDetail"),true,true);
 const char* keys[]={"MentorHistoryAll","MentorHistoryCompleted","MentorHistoryEnded","MentorHistoryRewards"};
 for(int i=0;i<4;++i){int x=80+i*180;Button(x,292,160,44,Label(keys[i]));if(i==historyFilter)Glow(x,292,160,44);}
 Text(95,362,135,Label("MentorHistoryDate"),true);Text(250,362,165,Label("MentorHistoryStudent"),true);Text(440,362,200,Label("MentorHistoryActivity"),true);Text(670,362,100,Label("MentorPoints"),true);
 for(size_t i=0;i<Data.History.size();++i){const auto& e=Data.History[i];int y=398+static_cast<int>(i)*69;if(static_cast<int>(i)==selected)Glow(80,y,710,70);Text(95,y+24,140,Date(e.Time));Text(250,y+24,160,e.Name);Text(440,y+24,215,e.Kind);Text(674,y+24,100,(e.Points>0?L"+":L"")+std::to_wstring(e.Points),true,true);}
 Text(95,870,260,std::to_wstring(Data.HistoryCount)+L" "+Label("MentorHistoryRecords"));Button(400,853,65,45,L"<",page>0);Button(604,853,65,45,L">",page+1<PageCount());Text(485,865,105,std::to_wstring(page+1)+L" / "+std::to_wstring(PageCount()),true,false,RT3_SORT_CENTER);
 Button(1260,902,210,65,Label("MentorClose"));if(Data.History.empty()){Text(870,350,540,Label("MentorEmpty"));return;}
 selected=std::clamp(selected,0,static_cast<int>(Data.History.size())-1);const auto& e=Data.History[selected];
 Text(930,311,325,e.HasDetail?e.StudentName:e.Name,true,true);Text(930,354,340,e.HasDetail?Label("MentorMentorRole")+L" "+e.MentorName:e.Kind);Text(1290,330,165,e.HasDetail?e.State:e.Kind,true);
 if(e.HasDetail){Text(860,418,275,Label("MentorHistoryStart"));Text(1170,418,285,Date(e.Created,true));Text(860,461,275,Label("MentorHistoryEnd"));Text(1170,461,285,e.Ended?Date(e.Ended,true):e.State==Label("MentorAccepted")?Label("MentorHistoryInProgress"):e.State);Text(860,503,275,Label("MentorValidTime"));Text(1170,503,285,std::to_wstring(e.Seconds/3600)+L" h "+std::to_wstring(e.Seconds%3600/60)+L" min");Text(860,546,275,Label("MentorPartyHours"));Text(1170,546,285,std::to_wstring(e.Seconds/3600));Text(860,586,275,Label("MentorHistoryEarned"));Text(1170,586,285,std::to_wstring(e.Awarded),true,true);}
 Text(860,647,580,Label("MentorRecent"),true,true);Text(930,705,480,e.Kind,true,true);Text(930,735,480,Date(e.Time,true));Text(930,765,480,(e.Points>0?L"+":L"")+std::to_wstring(e.Points)+L" "+Label("MentorPointUnit"),true);
 Text(860,812,275,Label("MentorProgress"),true,true);if(e.HasDetail){Text(1200,812,255,std::to_wstring(e.Resets)+L" / "+std::to_wstring(e.Goal)+L" resets",true);float f=std::clamp(static_cast<float>(e.Resets)/std::max(1,e.Goal),0.f,1.f);UI::Theme::TintOverlay(X(850),Y(853),W(static_cast<int>(590*f)),H(18),1,.65f,.05f,.8f);}
}

} // namespace
void Load(){Trace("panel-loaded");historyTexture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_MentorHistoryV40.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);homeTexture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_MentorHomeV40.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);rewardsTexture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_MentorRewardsV40.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);texture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_MentorV34.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);findTexture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_MentorFindV34.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);portraitTexture=Bitmaps.LoadImage(L"Data\\Interface\\Paris_MentorPortraitsV34.tga",GL_LINEAR,GL_CLAMP_TO_EDGE);nameInput=std::make_unique<CUITextInputBox>();nameInput->Init(g_hWnd,220,18,10);nameInput->SetTextColor(255,255,255,255);nameInput->SetBackColor(0,0,0,0);nameInput->SetState(UISTATE_HIDE);UI::Character::Portraits::Load();Layout();}
void Close(){ClearTooltip();visible=requested=inviteInput=false;confirmation.clear();if(nameInput)nameInput->SetState(UISTATE_HIDE);}
void Unload(){Close();nameInput.reset();g_pRenderText->SetFont(g_hFont);if(normal)DeleteObject(normal);if(bold)DeleteObject(bold);if(small)DeleteObject(small);normal=bold=small=nullptr;fontSize=0;if(texture!=BITMAP_UNKNOWN)DeleteBitmap(texture);texture=BITMAP_UNKNOWN;if(findTexture!=BITMAP_UNKNOWN)DeleteBitmap(findTexture);findTexture=BITMAP_UNKNOWN;if(portraitTexture!=BITMAP_UNKNOWN)DeleteBitmap(portraitTexture);portraitTexture=BITMAP_UNKNOWN;if(homeTexture!=BITMAP_UNKNOWN)DeleteBitmap(homeTexture);if(rewardsTexture!=BITMAP_UNKNOWN)DeleteBitmap(rewardsTexture);if(historyTexture!=BITMAP_UNKNOWN)DeleteBitmap(historyTexture);historyTexture=BITMAP_UNKNOWN;homeTexture=rewardsTexture=BITMAP_UNKNOWN;Received=BuffReceived=0;BuffBonus=0;BuffActive=false;ActiveBuffs.clear();BuffCaption.clear();Data=Snapshot();}
void Toggle(){if(IsOpen()){Close();return;}if(!nameInput){Trace("panel-not-initialized");return;}Trace("open-requested");UI::GmEvents::Close();g_pNewUISystem->HideAll();visible=false;requested=true;tab=page=selected=offerSelected=resetFilter=0;lastSearch.clear();historyFilter=0;nameInput->SetText(L"");message.clear();confirmation.clear();selectedIdentity.clear();Received=0;openedAt=timeGetTime();timeoutReported=false;Send();}
bool IsOpen(){return visible||requested;}bool HasInputFocus(){return visible&&tab==1&&!Data.IsMentor&&nameInput->HaveFocus();}
void Render(){if(requested&&Received){tab=Data.Tab;page=Data.Page;visible=true;requested=false;nameInput->SetState(UISTATE_NORMAL);}if(requested){RenderPending();return;}if(!visible)return;if((Data.IsMentor&&tab==1)||(!Data.IsMentor&&(tab==0||tab==2))){tab=Data.IsMentor?0:1;page=selected=0;selectedIdentity.clear();nameInput->SetText(L"");}Layout();if(timeGetTime()-lastRequest>=2000)Send();if(!Data.Message.empty()){message=Data.Message;Data.Message.clear();}if(!confirmation.empty()&&confirmToken!=Data.Token){confirmation.clear();message=Label("MentorRefresh");}EnableAlphaTest();UI::Paris::Color(1,1,1,1);bool modern=tab==3||Data.IsMentor&&(tab==0||tab==2);RenderBitmap(tab==3?historyTexture:modern?(tab==0?homeTexture:rewardsTexture):tab==1?findTexture:texture,X(0),Y(0),W(Width),H(Height),0,0,.75f,1);
 const char* tabs[]={"MentorNovices","MentorFind","MentorRewards","MentorHistory"};
 if(Data.IsMentor){if(!modern)Cover(310,112,920,50);int values[]={0,2,3};for(int i=0;i<3;++i){int x=330+i*300;if(!modern)RenderBitmap(homeTexture,X(x),Y(115),W(285),H(44),622.f/2048.f,115.f/1024.f,290.f/2048.f,44.f/1024.f);Button(x,115,285,44,Label(tabs[values[i]]));if(values[i]==tab)Glow(x,115,285,44);}}
 else for(int i=0;i<4;++i){if(i==0||i==2){Cover(317+i*227,112,222,50);continue;}Button(319+i*227,115,217,44,Label(tabs[i]));if(i==tab)Glow(319+i*227,115,217,44);}
 if(tab==1){FindMentors();g_pRenderText->SetFont(g_hFont);return;}
 Text(modern?80:150,184,modern?240:224,Data.Name,true,true);Text(modern?375:399,184,modern?235:275,Label("MentorPoints")+L": "+std::to_wstring(Data.Points),true,true);Text(modern?665:701,184,modern?220:173,Label("MentorGraduated")+L": "+std::to_wstring(Data.Graduated),true);Text(modern?945:908,184,modern?215:239,Label("MentorToday")+L": "+std::to_wstring(Data.Daily)+L" / "+std::to_wstring(Data.DailyLimit),true);Text(modern?1220:1180,184,205,L"EXP: +"+std::to_wstring(Data.Bonus)+L"%",true);
 if(modern){nameInput->SetState(UISTATE_HIDE);if(tab==0)MentorHome();else if(tab==2)RewardsGrid();else History();Confirmation();g_pRenderText->SetFont(g_hFont);return;}
 Text(156,240,600,Label(tab==1?"MentorFind":tab==3?"MentorHistory":"MentorNovices"),true,true);if(tab<2)PersonRows();else if(tab==2)OfferDetails();else History();
 if(tab!=3)Offers();nameInput->SetState(tab==0&&inviteInput?UISTATE_NORMAL:UISTATE_HIDE);if(tab<2&&inviteInput){Cover(150,745,607,38);nameInput->Render();wchar_t value[11]{};nameInput->GetText(value,11);if(!*value&&!nameInput->HaveFocus())Text(161,750,580,Label("MentorName"));}
 Button(143,809,330,53,Label(Data.IsMentor?"MentorInvite":"MentorRequest"),tab<2&&Data.Enabled);Button(495,809,278,53,Label("MentorHistory"));Button(1291,886,125,59,Label("MentorClose"));Text(151,902,627,message,true);if(timeGetTime()-Received>6000)Text(151,931,627,Label("MentorNoResponse"));Text(842,932,125,Label("MentorPrevious"),true);Text(1067,932,127,Label("MentorNext"),true);Text(974,932,85,std::to_wstring(page+1)+L" / "+std::to_wstring(PageCount()),true);
 if(!confirmation.empty()){Cover(353,385,810,267);Text(397,415,700,Label("MentorConfirm"),true,true);Text(397,461,700,Label("MentorCost")+L": "+std::to_wstring(confirmCost)+L" / "+Label("MentorPoints")+L": "+std::to_wstring(Data.Points),true);Button(413,556,280,57,Label("MentorConfirm"));Button(791,556,280,57,Label("MentorCancel"));}
 g_pRenderText->SetFont(g_hFont);}
void RenderActiveBuff(int x,int y){
 constexpr DWORD BuffExpiryMilliseconds=20000;
 if(!BuffActive||!BuffReceived||timeGetTime()-BuffReceived>BuffExpiryMilliseconds||texture==BITMAP_UNKNOWN)return;
 EnableAlphaTest();UI::Paris::Color(1,1,1,1);
 int column=0;
 if(BuffBonus>0){
  RenderBitmap(texture,x,y,16,16,302.f/2048.f,738.f/1024.f,40.f/2048.f,40.f/1024.f);
  g_pRenderText->SetFont(g_hFont);g_pRenderText->SetBgColor(0,0,0,0);g_pRenderText->SetTextColor(120,255,120,255);
  wchar_t exp[24]{};swprintf_s(exp,L"+%d%% EXP",BuffBonus);g_pRenderText->RenderText(x,y+17,exp,64);column=1;
 }
 for(const auto& buff:ActiveBuffs){
  const int bx=x+(column%8)*44,by=y+(column/8)*34;
  auto number=static_cast<eBuffState>(buff.Number);g_pBuffWindow->RenderBuffIcon(number,bx,by,16,22);
  g_pRenderText->SetFont(g_hFont);g_pRenderText->SetBgColor(0,0,0,0);g_pRenderText->SetTextColor(120,255,120,255);
  wchar_t percentage[24]{};swprintf_s(percentage,L"+%.1f%%",static_cast<double>(buff.Percent));g_pRenderText->RenderText(bx,by+22,percentage,44);
  if(SEASON3B::CheckMouseIn(bx,by,40,32)){
   g_pRenderText->SetBgColor(0,0,0,220);g_pRenderText->RenderText(bx,by+36,buff.Caption.c_str(),280);g_pRenderText->SetBgColor(0,0,0,0);
  }
  ++column;
 }
}

void Render3D(){
 if(!visible||tab!=2||!confirmation.empty())return;
 bool hovered=false;
 for(size_t index=0;index<Data.Offers.size();++index){
  const auto& offer=Data.Offers[index];int px=415+static_cast<int>(index%3)*471,py=355+static_cast<int>(index/3)*344;
  for(const auto& reward:offer.Rewards){
   if(reward.Type!=6||reward.Group<0||reward.Group>=16||reward.Number<0||reward.Number>=512)continue;
   RenderMarketPreviewItem3D(X(px),Y(py),W(70),H(62),reward.Group*512+reward.Number,reward.Level,reward.Excellent,reward.Ancient,.58f);
   if(Hit(px,py,70,62)&&g_pNewItemMng){
    hovered=true;
    if(tooltipRow!=static_cast<int>(index)||tooltipRevision!=Received){
     ClearTooltip();ItemCreationParams parameters{};parameters.Group=reward.Group;parameters.Number=reward.Number;parameters.Level=static_cast<BYTE>(reward.Level);parameters.Durability=static_cast<BYTE>(reward.Durability?reward.Durability:255);parameters.WithLuck=reward.Luck;parameters.WithSkill=reward.Skill;parameters.WithOption=reward.Option>0;parameters.OptionLevel=static_cast<BYTE>(reward.Option);parameters.HasExcellentOption=reward.Excellent!=0;parameters.ExcellentFlags=static_cast<BYTE>(reward.Excellent);parameters.IsAncient=reward.Ancient!=0;parameters.AncientDiscriminator=static_cast<BYTE>(reward.Ancient);parameters.AncientBonusOption=static_cast<BYTE>(reward.AncientBonus);std::fill(std::begin(parameters.SocketOptions),std::end(parameters.SocketOptions),SOCKET_EMPTY);tooltip=g_pNewItemMng->CreateItemByParameters(&parameters);tooltipRow=static_cast<int>(index);tooltipRevision=Received;
    }
    if(tooltip)RenderMarketItemInfo(std::clamp(MouseX+12,12,400),std::clamp(MouseY+12,30,240),tooltip);
   }
   break;
  }
 }
 if(!hovered)ClearTooltip();
}

bool Mouse(){if(!IsOpen())return false;UI::OverlayInput::Scope mouseOwner;if(requested){if(SEASON3B::IsPress(VK_LBUTTON)&&Hit(1370,49,50,47))Close();MouseLButton=MouseLButtonPush=MouseLButtonPop=false;return true;}if(!visible)return false;Layout();nameInput->DoAction();if(SEASON3B::IsPress(VK_LBUTTON)){if(!confirmation.empty()){if(Hit(413,556,280,57)){Send(6,confirmation);confirmation.clear();}else if(Hit(791,556,280,57))confirmation.clear();return true;}bool modern=tab==3||Data.IsMentor&&(tab==0||tab==2);if((modern?Hit(1430,40,68,65):Hit(1370,49,50,47))||(!modern&&tab!=1&&Hit(1291,886,125,59))){Close();return true;}for(int i=0;i<4;++i)if((Data.IsMentor?i!=1:i==1||i==3)&&Hit(Data.IsMentor?330+(i==0?0:i-1)*300:319+i*227,115,Data.IsMentor?285:217,44)){tab=i;page=selected=offerSelected=0;inviteInput=false;nameInput->SetText(L"");lastSearch.clear();Send();return true;}if(!modern&&tab!=1&&Hit(842,919,125,45)&&page>0){--page;selected=0;Send();return true;}if(!modern&&tab!=1&&Hit(1067,919,127,45)&&page+1<PageCount()){++page;selected=0;Send();return true;}
 if(Data.Tab!=tab||Data.Page!=page||timeGetTime()-Received>6000)return true;
 if(tab==3){if(Hit(1260,902,210,65)){Close();return true;}for(int i=0;i<4;++i)if(Hit(80+i*180,292,160,44)){historyFilter=i;page=selected=0;Send();return true;}if(Hit(400,853,65,45)&&page>0){--page;selected=0;Send();return true;}if(Hit(604,853,65,45)&&page+1<PageCount()){++page;selected=0;Send();return true;}for(size_t i=0;i<Data.History.size();++i)if(Hit(80,398+static_cast<int>(i)*69,710,70))selected=static_cast<int>(i);return true;}

 if(Data.IsMentor&&tab==0){
  if(Hit(752,296,32,35)&&page>0){--page;selected=0;Send();return true;}if(Hit(752,950,32,35)&&page+1<PageCount()){++page;selected=0;Send();return true;}
  auto order=HomeOrder();if(!order.empty()){const auto& row=Data.Relations[order[0]];if(Data.Enabled&&row.Incoming&&!row.Accepted&&!row.Graduated){if(Hit(130,400,270,50)){Send(3,row.Id);return true;}if(Hit(422,400,273,50)){Send(4,row.Id);return true;}}}
  for(size_t i=1;i<order.size();++i)if(Hit(65,472+(static_cast<int>(i)-1)*84,675,72)){selected=order[i];selectedIdentity=Data.Relations[selected].Id;return true;}
  return true;
 }
 if(Data.IsMentor&&tab==2){
  if(Hit(180,927,230,45)&&page>0){--page;Send();return true;}if(Hit(1100,927,230,45)&&page+1<PageCount()){++page;Send();return true;}
  for(size_t i=0;i<Data.Offers.size();++i){const auto& offer=Data.Offers[i];if(Data.Enabled&&!Data.Pending&&Data.Points>=offer.Cost&&Hit(118+static_cast<int>(i%3)*471,487+static_cast<int>(i/3)*344,359,54)){confirmation=offer.Id;confirmCost=offer.Cost;confirmToken=Data.Token;return true;}}
  return true;
 }
 if(tab==1){
  if(Hit(195,853,202,45)&&page>0){--page;Send();return true;}
  if(Hit(603,853,202,45)&&page+1<PageCount()){++page;Send();return true;}
  if(Hit(634,280,229,43)){resetFilter=(resetFilter+25)%125;page=0;Send();return true;}
  if(Hit(140,280,474,43))nameInput->GiveFocus();
  if(!Data.LinkId.empty()&&!Data.LinkAccepted&&Hit(684,911,170,38)){Send(Data.LinkIncoming?4:5,Data.LinkId);return true;}
  for(size_t i=0;i<Data.Mentors.size();++i)if(Data.Enabled&&Hit(637,350+static_cast<int>(i)*84,213,48)){const auto& row=Data.Mentors[i];if(!row.Id.empty()&&!row.Accepted)Send(Data.LinkIncoming?3:5,row.Id);else if(Data.CanRequest&&row.Novices<row.Maximum)Send(1,{},row.Name.c_str());return true;}
  return true;
 }

 if(tab<2){const auto& rows=tab==1?Data.Mentors:Data.Relations;for(size_t i=0;i<rows.size();++i)if(Hit(130,275+static_cast<int>(i)*83,660,74))selected=static_cast<int>(i);if(!rows.empty()){const auto& row=rows[std::min(selected,static_cast<int>(rows.size())-1)];if(tab==0){if(row.Incoming&&Hit(143,660,195,58))Send(3,row.Id);if(row.Incoming&&Hit(348,660,195,58))Send(4,row.Id);}else if(Data.IsNovice&&Hit(143,660,195,58))Send(1,{},row.Name.c_str());}if(Hit(143,729,630,59))nameInput->GiveFocus();if(Hit(143,809,330,53)){wchar_t name[11]{};nameInput->GetText(name,11);if(!inviteInput){inviteInput=true;nameInput->GiveFocus();}else if(*name&&Data.Enabled){Send(Data.IsMentor?2:1,{},name);inviteInput=false;}}}
 if(Hit(495,809,278,53)){tab=3;page=selected=offerSelected=0;Send();}
 if(tab==2)for(size_t i=0;i<Data.Offers.size();++i)if(Hit(820,304+static_cast<int>(i)*143,440,130))offerSelected=static_cast<int>(i);
 if(tab!=3&&Data.Enabled&&Data.IsMentor&&!Data.Pending)for(size_t i=0;i<Data.Offers.size();++i){const auto& offer=Data.Offers[i];if(Data.Points>=offer.Cost&&Hit(1273,363+static_cast<int>(i)*143,130,53)){confirmation=offer.Id;confirmCost=offer.Cost;confirmToken=Data.Token;}}
 }return true;}
} // namespace UI::Mentor
