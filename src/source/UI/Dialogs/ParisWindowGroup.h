#pragma once

namespace mu::ui::window
{
    class CObject;
    // Top-level Paris panels replace one another; confirmations remain independent.
    bool HasOpenParisWindow();
    void CloseOtherParisWindows(CObject* opening);
}
