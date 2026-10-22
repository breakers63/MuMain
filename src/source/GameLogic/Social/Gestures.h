#pragma once
#include "Core/Globals/_enum.h"
#include <array>
namespace GameLogic::Social
{
struct GestureDefinition
{
    const wchar_t* label;
    const wchar_t* glyph;
    int animation;
    int serverAction;
};
inline constexpr std::array<GestureDefinition, 25> Gestures = {{
    {L"Saludar", L"Hi", PLAYER_GREETING1, AT_GREETING1},
    {L"Despedirse", L"Bye", PLAYER_GOODBYE1, AT_GOODBYE1},
    {L"Aplaudir", L"Clap", PLAYER_CLAP1, AT_CLAP1},
    {L"Animar", L"Yay", PLAYER_CHEER1, AT_CHEER1},
    {L"Re\u00edr", L"^^", PLAYER_SMILE1, AT_SMILE1},
    {L"Llorar", L"T_T", PLAYER_CRY1, AT_CRY1},
    {L"Reverencia", L"Bow", PLAYER_RESPECT1, AT_RESPECT1},
    {L"Saludo militar", L"o7", PLAYER_SALUTE1, AT_SALUTE1},
    {L"Victoria", L"Win", PLAYER_WIN1, AT_WIN1},
    {L"Ven aqu\u00ed", L"Hey", PLAYER_GESTURE1, AT_GESTURE1},
    {L"Se\u00f1alar", L"->", PLAYER_DIRECTION1, AT_DIRECTION1},
    {L"No s\u00e9", L"?", PLAYER_UNKNOWN1, AT_UNKNOWN1},
    {L"Disculparse", L";", PLAYER_AWKWARD1, AT_AWKWARD1},
    {L"Mirar", L"-_-", PLAYER_SEE1, AT_SEE1},
    {L"Dormir", L"Zzz", PLAYER_SLEEP1, AT_SLEEP1},
    {L"Fr\u00edo", L"Brr", PLAYER_COLD1, AT_COLD1},
    {L"Otra vez", L"Agn", PLAYER_AGAIN1, AT_AGAIN1},
    {L"Adelante", L"Go!", PLAYER_RUSH1, AT_RUSH1},
    {L"Tijera", L"Sci", PLAYER_SCISSORS, AT_SCISSORS},
    {L"Piedra", L"Rck", PLAYER_ROCK, AT_ROCK},
    {L"Papel", L"Ppr", PLAYER_PAPER, AT_PAPER},
    {L"Bailar", L"Hus", PLAYER_HUSTLE, AT_HUSTLE},
    {L"Provocar", L"Prv", PLAYER_PROVOCATION, AT_PROVOCATION},
    {L"Mirar alrededor", L"Look", PLAYER_LOOK_AROUND, AT_LOOK_AROUND},
    {L"Celebrar", L"Chr", PLAYER_CHEERS, AT_CHEERS},
}};
// Only idle characters may start a gesture; the existing action pipeline sends it once.
bool PerformGesture(int index);
}
