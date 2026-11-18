#pragma once
#include <string>
namespace Network::Paris::RegistrationSession {
void Begin(const std::string& user,const std::string& password);
bool ServerList();
void Joined();
}
