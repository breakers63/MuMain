#pragma once

// Shared desktop animation curve. Inputs are the speeds received from the server.
// The legacy range corrections prevent animation frame skips at high agility.
namespace ParisAnimation {
struct Rates { float attack, magic, magicHalf; };
inline Rates FromServer(unsigned attack, unsigned magic)
{

    float AttackSpeed1 = attack;
    float MagicSpeed1 = magic;
    float MagicSpeed2 = magic;

    if (attack >= 509 && attack <= 549)
    {
        AttackSpeed1 = AttackSpeed1 * 0.0026000f;
    }
    else if (attack >= 550 && attack <= 750)
    {
        AttackSpeed1 = AttackSpeed1 * 0.0017000f;
    }
    else
    {
        AttackSpeed1 = AttackSpeed1 * 0.0040000f;
    }

    if (magic >= 509 && magic <= 549)
    {
        MagicSpeed1 = MagicSpeed1 * 0.0026000f;
    }
    else if (magic >= 550 && magic <= 750)
    {
        MagicSpeed1 = MagicSpeed1 * 0.0017000f;
    }
    else
    {
        MagicSpeed1 = MagicSpeed1 * 0.0040000f;
    }

    if (magic >= 455 && magic <= 479)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0024700f;
    }
    else if (magic >= 605 && magic <= 636)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0019000f;
    }
    else if (magic >= 637 && magic <= 668)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0018000f;
    }
    else if (magic >= 669 && magic <= 688)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0017000f;
    }
    else if (magic >= 855 && magic <= 1040)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0016300f;
    }
    else if (magic >= 1041 && magic <= 1104)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0015500f;
    }
    else if (magic >= 1301 && magic <= 1500)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0017500f;
    }
    else if (magic >= 1501 && magic <= 1524)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0015000f;
    }
    else if (magic >= 1525 && magic <= 1800)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0014500f;
    }
    else if (magic >= 1801 && magic <= 1999)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0013000f;
    }
    else if (magic >= 2000 && magic <= 2167)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0012500f;
    }
    else if (magic >= 2168 && magic <= 2354)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0011500f;
    }
    else if (magic >= 2855 && magic <= 3011)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0009000f;
    }
    else if (magic >= 3011)
    {
        MagicSpeed2 = MagicSpeed2 * 0.0008100f;
    }
    else
    {
        MagicSpeed2 = MagicSpeed2 * 0.0020000f;
    }
    return {AttackSpeed1, MagicSpeed1, MagicSpeed2};
}
}
