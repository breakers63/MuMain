#pragma once

#include <array>
#include <map>
#include <string>
#include <algorithm>

// Platform ids are shared with ParisClientPlatformHandlerPlugIn.
namespace ParisPlatform
{
    using Name = std::wstring;
    inline std::map<Name, int, std::less<>> Players;
    inline unsigned Received = 0;
    inline unsigned LastPoll = 0;
    inline bool InWorld = false;
    struct Badge { float X, Y, Size; int Platform; };
    inline std::array<Badge, 256> Frame;
    inline size_t Count = 0;

    inline int Own()
    {
#if defined(__EMSCRIPTEN__) || defined(MU_WEB_BROWSER)
        return 4;
#elif defined(__ANDROID__)
        return 1;
#elif defined(__APPLE__)
        return 2;
#else
        return 3;
#endif
    }

    inline void BeginFrame() { Count = 0; }

    inline void Queue(const wchar_t* name, int x, int y, int width, int height, bool own, bool centered = false)
    {
        const unsigned now = timeGetTime();
        const auto found = Players.find(name);
        const int platform = own ? Own() : (found != Players.end() && now - Received < 12000 ? found->second : 0);
        if (platform < 1 || platform > 4 || Count >= Frame.size()) return;
        const float size = (std::max)(10.f, static_cast<float>(height));
        if (centered)
        {
            SIZE measured{};
            measured = g_pRenderText->MeasureText(name, lstrlen(name));
            x += (width - static_cast<int>(measured.cx / g_fScreenRate_x)) / 2;
        }
        Frame[Count++] = { (std::max)(0.f, x - size - 3.f), y + (height - size) * 0.5f, size, platform };
    }

    inline void Flush()
    {
        // Draw after all name backgrounds so guild labels cannot cover a badge.
        const bool depth = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        glDisable(GL_DEPTH_TEST);
        EnableAlphaTest();
        static constexpr const char* masks[4][12] = {
            { "001000000100", "000100001000", "001111111100", "011011110110",
              "011111111110", "011111111110", "011111111110", "001111111100",
              "001111111100", "001111111100", "000110011000", "000110011000" },
            { "000000110000", "000001100000", "001110011100", "011111111110",
              "111111111100", "111111111000", "111111111000", "111111111110",
              "011111111110", "011111111100", "001110111000", "000100010000" },
            { "111110111111", "111110111111", "111110111111", "111110111111",
              "111110111111", "000000000000", "111110111111", "111110111111",
              "111110111111", "111110111111", "111110111111", "111110111111" },
            { "000111111000", "001001100100", "010001100010", "100001100001",
              "100001100001", "111111111111", "111111111111", "100001100001",
              "100001100001", "010001100010", "001001100100", "000111111000" }
        };
        static constexpr float colors[4][3] = { {0.55f,0.85f,0.20f}, {0.95f,0.95f,0.95f},
            {0.08f,0.75f,0.94f}, {0.75f,0.50f,1.f} };
        for (size_t i = 0; i < Count; ++i)
        {
            const auto& badge = Frame[i];
            SetRenderColor(171, 122, 48, 255);
            RenderColor(badge.X, badge.Y, badge.Size, badge.Size);
            SetRenderColor(13, 13, 13, 255);
            RenderColor(badge.X + 0.7f, badge.Y + 0.7f, badge.Size - 1.4f, badge.Size - 1.4f);
            const auto* color = colors[badge.Platform - 1];
            SetRenderColor(static_cast<BYTE>(color[0] * 255.f),
                static_cast<BYTE>(color[1] * 255.f), static_cast<BYTE>(color[2] * 255.f), 255);
            const float pixel = (badge.Size - 3.f) / 12.f;
            for (int row = 0; row < 12; ++row)
            {
                const char* mask = masks[badge.Platform - 1][row];
                int col = 0;
                while (col < 12)
                {
                    if (mask[col] != '1') { ++col; continue; }
                    const int start = col;
                    while (col < 12 && mask[col] == '1') ++col;
                    RenderColor(badge.X + 1.5f + start * pixel, badge.Y + 1.5f + row * pixel,
                        (col - start) * pixel, pixel);
                }
            }
        }
        SetRenderColor(255, 255, 255, 255);
        EndRenderColor();
        EnableAlphaTest();
        if (depth) glEnable(GL_DEPTH_TEST);
    }

    inline void Reset()
    {
        Players.clear();
        Received = LastPoll = 0;
        InWorld = false;
        Count = 0;
    }
}
