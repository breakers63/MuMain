#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace GameLogic::Events
{
    class ActiveInvasionState
    {
    public:
        struct Monster { bool used=false; int id=0, alive=0, total=0; std::array<wchar_t,96> name{}; };
        struct Event { bool used=false; int id=0; std::uint64_t deadline=0; std::array<wchar_t,96> name{}; std::array<Monster,32> monsters{}; };
        std::array<Event,8> events{};
        bool Apply(std::wstring_view text, std::uint64_t now);
        void Clear();
    private:
        void Update(const std::array<std::wstring_view,8>& fields, std::uint64_t now);
    };
}
