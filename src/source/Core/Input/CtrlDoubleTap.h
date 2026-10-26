#pragma once
#include <cstdint>

namespace Core::Input
{
// Only two short, otherwise unused Ctrl presses form the toggle gesture.
class CtrlDoubleTap
{
public:
    bool Update(bool down, bool allowed, bool otherInput, std::uint64_t now)
    {
        if (!allowed || otherInput)
        {
            m_armed = false;
            m_cancelled = down;
        }
        if (down && !m_down)
        {
            m_pressed = now;
            m_cancelled = !allowed || otherInput;
        }
        bool toggle = false;
        if (!down && m_down)
        {
            if (!m_cancelled && allowed && !otherInput && now - m_pressed <= MaxHoldMs)
            {
                toggle = m_armed && now - m_released <= DoubleTapMs;
                m_armed = !toggle;
                m_released = now;
            }
            else m_armed = false;
            m_cancelled = false;
        }
        m_down = down;
        return toggle;
    }
private:
    static constexpr std::uint64_t DoubleTapMs = 300;
    static constexpr std::uint64_t MaxHoldMs = 200;
    bool m_down = false, m_armed = false, m_cancelled = false;
    std::uint64_t m_pressed = 0, m_released = 0;
};
}
