#pragma once

namespace UI::OverlayInput
{
    inline bool waitingForRelease = false;
    inline DWORD blockUntil = 0;

    // A close button still owns its mouse press after its window disappears.
    inline void Capture()
    {
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) || MouseLButtonPush)
            waitingForRelease = true;
        blockUntil = timeGetTime() + 150;
    }

    struct Scope
    {
        Scope() { Capture(); }
        ~Scope() { MouseLButton = MouseLButtonPush = MouseLButtonPop = false; }
    };

    inline bool BlocksWorld()
    {
        if (waitingForRelease)
        {
            if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
            {
                waitingForRelease = false;
                blockUntil = timeGetTime() + 150;
            }
            return true;
        }
        return static_cast<LONG>(blockUntil - timeGetTime()) > 0;
    }
}
