#include "Network/Paris/RegistrationSession.h"
#include "stdafx.h"
#include "RegistrationDialog.h"
#include "UI/Windows/LoginSceneUpdates.h"
#include "UI/Windows/LoginWin.h"
#include "Network/Server/WSclient.h"
#include "Network/Paris/RegistrationClient.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Input.h>
#include <algorithm>

extern BOOL g_bGameServerConnected;
namespace UI::Login::Registration {
namespace {
struct Model {
    Rml::String user,password,confirmation,email,message;
    Rml::String submitText="CREAR CUENTA";
    bool busy=false,done=false;
};
Network::Paris::Registration::Request request;
std::string createdSecret;
bool visible=false;
bool alive=true;
void Submit();
void Close(bool restoreLogin=true);
void RefreshControls();
void Bind(Rml::DataModelConstructor& constructor,Model& model){
    constructor.Bind("username",&model.user);constructor.Bind("password",&model.password);
    constructor.Bind("confirmation",&model.confirmation);constructor.Bind("email",&model.email);
    constructor.Bind("message",&model.message);constructor.Bind("submit_text",&model.submitText);
    constructor.Bind("busy",&model.busy);constructor.Bind("done",&model.done);
    constructor.BindEventCallback("registration_submit",[](Rml::DataModelHandle,Rml::Event&,const Rml::VariantList&){Submit();});
    constructor.BindEventCallback("registration_close",[](Rml::DataModelHandle,Rml::Event&,const Rml::VariantList&){Close();});
    constructor.BindEventCallback("registration_key",[](Rml::DataModelHandle,Rml::Event& event,const Rml::VariantList&){
        const int key=event.GetParameter<int>("key_identifier",0);
        if(key==Rml::Input::KI_RETURN){Submit();event.StopPropagation();}
        else if(key==Rml::Input::KI_ESCAPE){Close();event.StopPropagation();}
    });
}
UI::RmlBridge::ThemedView<Model> view{"paris_registration",Bind,{{"Data/Interface/RmlUi/registration.rml"}},
    {.modal=Rml::ModalFlag::Modal,.focus=Rml::FocusFlag::Document,.afterBuild=[] { RefreshControls(); }}};
struct LifetimeGuard {~LifetimeGuard(){alive=false;}} lifetimeGuard;
void RefreshControls(){
    if(!view.Document())return;
    const bool busy=view.GetModel().busy;
    for(const char* id:{"registration_user","registration_password","registration_confirmation","registration_email","registration_submit","registration_close"}){
        if(auto* element=view.Document()->GetElementById(id)){if(busy)element->SetAttribute("disabled",true);else element->RemoveAttribute("disabled");}
    }
}
void Dirty(){RefreshControls();for(const char* name:{"username","password","confirmation","email","message","submit_text","busy","done"})view.MarkDirty(name);}
void ClearSecrets(){
    auto& model=view.GetModel();
    std::fill(model.password.begin(),model.password.end(),'\0');model.password.clear();
    std::fill(model.confirmation.begin(),model.confirmation.end(),'\0');model.confirmation.clear();
    view.MarkDirty("password");view.MarkDirty("confirmation");
}
void Close(bool restoreLogin){
    if(request.Busy())return;
    ClearSecrets();std::fill(createdSecret.begin(),createdSecret.end(),'\0');createdSecret.clear();visible=false;view.Hide();
    if(restoreLogin&&SceneFlag==LOG_IN_SCENE){
        if(g_bGameServerConnected&&SocketClient&&SocketClient->IsConnected()&&CurrentProtocolState==RECEIVE_JOIN_SERVER_SUCCESS){
            UI::LoginScene::ShowLoginWindow();
        }else{
            g_bGameServerConnected=FALSE;delete SocketClient;SocketClient=nullptr;
            g_LoginWin.ConnectConnectionServer();
            if(SocketClient&&SocketClient->IsConnected())SocketClient->ToConnectServer()->SendServerListRequest();
            UI::LoginScene::ServerListReceived();
        }
    }
}
void Submit(){
    auto& model=view.GetModel();if(request.Busy())return;
    if(model.done){Network::Paris::RegistrationSession::Begin(model.user,createdSecret);std::fill(createdSecret.begin(),createdSecret.end(),'\0');createdSecret.clear();Close(false);return;}
    createdSecret=model.password;
    model.message=request.Start(model.user,model.password,model.confirmation,model.email);
    model.busy=request.Busy();model.submitText=model.busy?"CREANDO...":"CREAR CUENTA";
    if(model.busy)model.message="Creando cuenta";Dirty();
}
}
void Open(){
    if(SceneFlag!=LOG_IN_SCENE||!view.Ensure())return;
    auto& model=view.GetModel();ClearSecrets();model.user.clear();model.email.clear();model.message.clear();model.done=false;model.busy=request.Busy();model.submitText="CREAR CUENTA";
    Dirty();visible=true;view.Document()->Show(Rml::ModalFlag::Modal,Rml::FocusFlag::Document);
    if(auto* input=view.Document()->GetElementById("registration_user"))input->Focus();
}
void Tick(){
    if(!visible)return;
    if(SceneFlag!=LOG_IN_SCENE){Release();return;}
    if(auto result=request.Poll()){
        auto& model=view.GetModel();ClearSecrets();model.message=result->Message;model.busy=false;model.done=result->Status==201;if(!model.done){std::fill(createdSecret.begin(),createdSecret.end(),'\0');createdSecret.clear();}model.submitText=model.done?"CONTINUAR":"CREAR CUENTA";Dirty();
    }
}
void Release(){if(!alive)return;ClearSecrets();visible=false;view.Hide();}
bool Visible(){return visible;}
}
