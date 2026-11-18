#pragma once
#include <future>
#include <optional>
#include <string>

namespace Network::Paris::Registration {
struct Result {int Status=0;std::string Message;};
std::string Validate(const std::string& user,const std::string& password,const std::string& confirmation,const std::string& email);
std::string StatusMessage(int status);

// One request at a time. Poll only from the UI thread; the worker owns the HTTPS call.
class Request {
    std::future<Result> future;
public:
    bool Busy() const {return future.valid();}
    std::string Start(std::string user,std::string password,const std::string& confirmation,std::string email);
    std::optional<Result> Poll();
};
}
