# Combat speed synchronization — 2026-10-31

OpenGL, Vulkan and Android/iOS use the same desktop animation curve and the same per-skill cooldown table. The server remains authoritative for physical and magical speed, skill range and sustained attack frequency. FPS affects visual smoothness; it does not grant permission for extra attacks.

The deployed detector uses separate physical/magic limits and a 110 ms profile for continuous warrior skills, with lag tolerance and no automatic ban or disconnect. Client animation parity is exhaustively checked for all 65,536 speed values. These checks do not replace measuring actual attack packets during gameplay on each device.

Do not tune agility or add a platform-specific delay to bypass the detector. Calibrate server profiles against valid casts for each skill before changing enforcement thresholds.
