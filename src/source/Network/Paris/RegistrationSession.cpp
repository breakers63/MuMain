#include "stdafx.h"
#include "RegistrationSession.h"
#include "Network/Server/WSclient.h"
#include "Network/Server/ServerListManager.h"
#include "Data/GameConfig/GameConfig.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Windows/LoginWin.h"

extern int LogIn;
extern BOOL g_bGameServerConnected;
namespace Network::Paris::RegistrationSession {
namespace {std::wstring username,secret;int pending=0;
void Clear(){std::fill(secret.begin(),secret.end(),L'\0');secret.clear();username.clear();pending=0;}}
void Begin(const std::string& user,const std::string& password){
 Clear();username=std::wstring(user.begin(),user.end());secret=std::wstring(password.begin(),password.end());pending=1;
 auto& config=GameConfig::GetInstance();config.SetRememberMe(true);config.SetSavePassword(true);config.EncryptAndSaveCredentials(username.c_str(),secret.c_str());
 for(std::size_t slot=0;slot<SavedLoginAccountLimit;++slot){auto account=config.ReadSavedLoginAccount(slot);if(account.username.empty()||account.username==username){config.SaveLoginAccount(slot,{username,secret},true);break;}}
 LogIn=0;CurrentProtocolState=REQUEST_JOIN_SERVER;g_bGameServerConnected=FALSE;
 delete SocketClient;SocketClient=nullptr;
 if(!CreateSocket(szServerIpAddress,g_ServerPort)){Clear();return;}
 SocketClient->ToConnectServer()->SendServerListRequest();
}
bool ServerList(){
 if(!pending)return false;
 if(pending==2)return true;
 CServerInfo* selected=nullptr;CServerGroup* selectedGroup=nullptr;
 for(auto& entry:g_ServerListManager->m_mapServerGroup){
  for(auto* server:entry.second->m_listServerInfo){
   if(server&&server->m_iPercent<100&&(!selected||server->m_iConnectIndex==g_ServerListManager->GetSelectServerIndex())){selected=server;selectedGroup=entry.second;}
  }
 }
 if(!selected){Clear();return false;}
 g_ServerListManager->SetSelectServerInfo(selectedGroup->m_szName,selected->m_iConnectIndex,selected->m_byNonPvP);
 pending=2;
 SocketClient->ToConnectServer()->SendConnectionInfoRequest(static_cast<uint16_t>(selected->m_iConnectIndex));return true;
}
void Joined(){
 if(!pending||CurrentProtocolState!=RECEIVE_JOIN_SERVER_SUCCESS)return;
 pending=0;
 g_LoginWin.SubmitCredentials(username.c_str(),secret.c_str());
 Clear();
}
}
