#pragma once
#include <array>
#include <string>
#include <string_view>
namespace UI::Chat
{
struct CommandHint { const wchar_t* name; const wchar_t* usage; const wchar_t* description; bool gm; };
inline constexpr CommandHint CommandHints[] = {
    {L"/trade", L"", L"Intercambiar con el jugador bajo el cursor", false},
    {L"/party", L"", L"Invitar a tu grupo al jugador bajo el cursor", false},
    {L"/pt", L"", L"Igual que /party", false},
    {L"/guild", L"", L"Pedir entrar al m\u00e1ster de guild bajo el cursor", false},
    {L"/union", L"", L"Ofrecer una alianza al m\u00e1ster de guild bajo el cursor", false},
    {L"/rival", L"", L"Declarar rivalidad al m\u00e1ster de guild bajo el cursor", false},
    {L"/rivaloff", L"", L"Retirar la rivalidad al m\u00e1ster de guild bajo el cursor", false},
    {L"/purchase", L"", L"Abrir la tienda personal del jugador bajo el cursor", false},
    {L"/duelstart", L"", L"Retar a duelo al jugador bajo el cursor", false},
    {L"/duelend", L"", L"Terminar el duelo actual", false},
    {L"/whisper", L"", L"Susurrar al jugador bajo el cursor", false},
    {L"/addfriend", L"", L"A\u00f1adir a tus amigos al jugador bajo el cursor", false},
    {L"/follow", L"", L"Caminar tras el jugador bajo el cursor", false},
    {L"/post", L"<message>", L"Gritar a todo el servidor", false},
    {L"/help", L"[command]", L"Ayuda del servidor sobre un comando", false},
    {L"/list", L"", L"Listar los comandos del servidor", false},
    {L"/add", L"<str|agi|vit|ene|cmd> <amount>", L"Gastar puntos de nivel", false},
    {L"/addstr", L"<amount>", L"Gastar puntos de nivel en fuerza", false},
    {L"/addagi", L"<amount>", L"Gastar puntos de nivel en agilidad", false},
    {L"/addvit", L"<amount>", L"Gastar puntos de nivel en vitalidad", false},
    {L"/addene", L"<amount>", L"Gastar puntos de nivel en energ\u00eda", false},
    {L"/addcmd", L"<amount>", L"Gastar puntos de nivel en mando", false},
    {L"/move", L"<map> [x y]", L"Viajar a un mapa", false},
    {L"/war", L"<guild>", L"Pedir una guerra de guilds", false},
    {L"/battlesoccer", L"<guild>", L"Pedir un partido de Battle Soccer", false},
    {L"/language", L"<isoCode>", L"Cambiar el idioma de los mensajes del servidor", false},
    {L"/offlevel", L"", L"Subir de nivel sin conexi\u00f3n", false},
    {L"/pkclear", L"[char]", L"Limpiar el estado de PK", false},
    {L"/openware", L"", L"Abrir el almac\u00e9n", true},
    {L"/npc", L"[id]", L"Abrir una tienda NPC", true},
    {L"/get", L"<stat> [char]", L"Leer una caracter\u00edstica", true},
    {L"/set", L"<stat> <value> [char]", L"Fijar una caracter\u00edstica", true},
    {L"/getlevel", L"[char]", L"Leer un nivel", true},
    {L"/setlevel", L"<level> [char]", L"Fijar un nivel", true},
    {L"/getmoney", L"[char]", L"Leer el zen", true},
    {L"/setmoney", L"<amount> [char]", L"Fijar el zen", true},
    {L"/getresets", L"[char]", L"Leer los resets", true},
    {L"/setresets", L"<count> [char]", L"Fijar los resets", true},
    {L"/getleveluppoints", L"[char]", L"Leer los puntos de nivel", true},
    {L"/setleveluppoints", L"<points> [char]", L"Fijar los puntos de nivel", true},
    {L"/getmasterlevel", L"[char]", L"Leer el nivel de master", true},
    {L"/setmasterlevel", L"<level> [char]", L"Fijar el nivel de master", true},
    {L"/getmasterleveluppoints", L"[char]", L"Leer los puntos de master", true},
    {L"/setmasterleveluppoints", L"<points> [char]", L"Fijar los puntos de master", true},
    {L"/item", L"<group> <number> [lvl ex sk lu opt anc ancBonuslvl]", L"Soltar un objeto", true},
    {L"/clearinv", L"[char]", L"Vaciar el inventario", true},
    {L"/skin", L"<number>", L"Ponerse la piel de un monstruo", true},
    {L"/charinfo", L"<char>", L"Mostrar un personaje", true},
    {L"/online", L"", L"Qui\u00e9n est\u00e1 conectado", true},
    {L"/trace", L"<char>", L"Viajar hasta un personaje", true},
    {L"/track", L"<char>", L"Traer un personaje hasta ti", true},
    {L"/teleport", L"<x> <y>", L"Teletransportarse en este mapa", true},
    {L"/hide", L"", L"Volverse invisible", true},
    {L"/unhide", L"", L"Volverse visible", true},
    {L"/disconnect", L"<char>", L"Desconectar a un personaje", true},
    {L"/banacc", L"<acc>", L"Banear una cuenta", true},
    {L"/unbanacc", L"<acc>", L"Quitar el baneo de una cuenta", true},
    {L"/banchar", L"<char>", L"Banear un personaje", true},
    {L"/unbanchar", L"<char>", L"Quitar el baneo de un personaje", true},
    {L"/chatban", L"<char> <minutes>", L"Silenciar a un personaje", true},
    {L"/chatunban", L"<char>", L"Quitar el silencio a un personaje", true},
    {L"/pk", L"<char> <pk_lvl> <pk_count>", L"Fijar el estado de PK", true},
    {L"/guildmove", L"<guild> <map> [x y]", L"Llevar un guild a otro mapa", true},
    {L"/guilddisconnect", L"<guild>", L"Desconectar un guild", true},
    {L"/goldnotice", L"<message>", L"Aviso dorado para todos", true},
    {L"/fireworks", L"<x> <y>", L"Fuegos artificiales", true},
    {L"/xmasfireworks", L"<x> <y>", L"Fuegos artificiales de Navidad", true},
    {L"/createmonster", L"<number> [intelligence]", L"Invocar un monstruo", true},
    {L"/movemonster", L"<id> <x> <y>", L"Mover un monstruo", true},
    {L"/walkmonster", L"<id> <x> <y>", L"Hacer caminar a un monstruo", true},
    {L"/removenpc", L"<id>", L"Quitar un NPC", true},
    {L"/showids", L"", L"Mostrar los ids de los NPC", true},
    {L"/startbc", L"", L"Empezar Blood Castle", true},
    {L"/startcc", L"", L"Empezar Chaos Castle", true},
    {L"/startds", L"", L"Empezar Devil Square", true},
};
class CommandCompletion
{
public:
    static constexpr int VisibleRows = 8;
    bool Sync(std::wstring_view text, bool gm, bool focused);
    void Move(int direction);
    int Count() const { return count; }
    int First() const { return first; }
    int Selected() const { return selected; }
    const CommandHint& Row(int index) const { return CommandHints[matches[index]]; }
    std::wstring Complete(int index) const;
private:
    std::wstring previous;
    bool previousGm = false, previousFocus = false;
    std::array<int, std::size(CommandHints)> matches{};
    int count = 0, selected = 0, first = 0;
};
}
