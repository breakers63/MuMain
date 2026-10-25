#pragma once

namespace mu::ui::window
{
    class CObject;
    // Top-level Paris panels replace one another; confirmations remain independent.
    void CloseOtherParisWindows(CObject* opening);
}
