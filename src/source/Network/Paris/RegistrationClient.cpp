#include "stdafx.h"
#include "RegistrationClient.h"
#include "../../../ThirdParty/json.hpp"
#include <algorithm>
#include <chrono>
#include <memory>
#if defined(_WIN32)
#include <winhttp.h>
#pragma comment(lib,"winhttp.lib")
#endif

namespace Network::Paris::Registration {
namespace {
constexpr std::size_t MinimumLength=4,MaximumUserLength=10,MaximumPasswordLength=20,MaximumEmailLength=254;
bool Printable(const std::string& value){return std::all_of(value.begin(),value.end(),[](unsigned char c){return c>=32&&c<=126;});}
#if defined(_WIN32)
struct CloseInternet {void operator()(void* handle) const {if(handle)WinHttpCloseHandle(handle);}};
using InternetHandle=std::unique_ptr<void,CloseInternet>;
struct PrivatePayload {
    std::string Text;
    ~PrivatePayload(){if(!Text.empty())SecureZeroMemory(Text.data(),Text.size());}
};
int Send(const std::string& user,const std::string& password,const std::string& email){
    InternetHandle session(WinHttpOpen(L"MUParis-Registration/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!session)return 0;
    WinHttpSetTimeouts(session.get(),10000,10000,20000,20000);
    InternetHandle connection(WinHttpConnect(session.get(),L"ws.hispanogaming.com",INTERNET_DEFAULT_HTTPS_PORT,0));
    if(!connection)return 0;
    InternetHandle request(WinHttpOpenRequest(connection.get(),L"POST",L"/launcher-api/register",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request)return 0;
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if(!WinHttpSetOption(request.get(),WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects)))return 0;
    PrivatePayload payload{nlohmann::json{{"LoginName",user},{"Password",password},{"Email",email},{"Remember",false}}.dump()};
    const auto size=static_cast<DWORD>(payload.Text.size());
    if(!WinHttpSendRequest(request.get(),L"Content-Type: application/json; charset=utf-8\r\n",static_cast<DWORD>(-1),payload.Text.data(),size,size,0)||!WinHttpReceiveResponse(request.get(),nullptr))return 0;
    DWORD status=0,bytes=sizeof(status);
    return WinHttpQueryHeaders(request.get(),WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&bytes,WINHTTP_NO_HEADER_INDEX)?static_cast<int>(status):0;
}
#else
int Send(const std::string&,const std::string&,const std::string&){return 0;}
#endif
}
std::string Validate(const std::string& user,const std::string& password,const std::string& confirmation,const std::string& email){
    if(user.size()<MinimumLength||user.size()>MaximumUserLength||!std::all_of(user.begin(),user.end(),[](unsigned char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9');}))return "Usuario 4 a 10 letras o numeros";
    if(password.size()<MinimumLength||password.size()>MaximumPasswordLength||!Printable(password))return "Contrasena 4 a 20 caracteres sin acentos";
    if(password!=confirmation)return "Las contrasenas no coinciden";
    const auto at=email.find('@'),dot=at==std::string::npos?std::string::npos:email.find('.',at+1);
    if(email.empty()||email.size()>MaximumEmailLength||!Printable(email)||email.find(' ')!=std::string::npos||at==0||at==std::string::npos||email.find('@',at+1)!=std::string::npos||dot==std::string::npos||dot==at+1||dot+1>=email.size())return "Escribe un correo electronico valido";
    return {};
}
std::string StatusMessage(int status){
    switch(status){
    case 201:return "Cuenta creada Pulsa CONTINUAR para entrar";
    case 409:return "Ese usuario ya esta registrado";
    case 429:return "Demasiados intentos para esta conexion Intenta mas tarde";
    case 400:return "Revisa los datos del registro";
    default:return "No se pudo confirmar el registro Prueba iniciar sesion antes de repetirlo";
    }
}
std::string Request::Start(std::string user,std::string password,const std::string& confirmation,std::string email){
    if(Busy())return "Solicitud en curso";
    auto error=Validate(user,password,confirmation,email);if(!error.empty())return error;
    future=std::async(std::launch::async,[user=std::move(user),password=std::move(password),email=std::move(email)]()mutable{
        int status=0;try{status=Send(user,password,email);}catch(...){status=0;}
        std::fill(password.begin(),password.end(),'\0');password.clear();return Result{status,StatusMessage(status)};
    });
    return {};
}
std::optional<Result> Request::Poll(){
    if(!future.valid()||future.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return std::nullopt;
    try{return future.get();}catch(...){return Result{0,StatusMessage(0)};}
}
}
