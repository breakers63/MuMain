#pragma once

namespace Network::Paris::Voice
{
    bool IsMicrophoneEnabled();
    void Toggle();
    void Receive(const BYTE* packet, int size);
    void Update();
}

