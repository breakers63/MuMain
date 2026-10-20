#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"

#include <cstdint>
#include <vector>

namespace SEASON3B
{
    constexpr DWORD INTERFACE_PARIS_QUEST = 0x43515354; // CQST

    class CParisQuestWindow final : public CNewUIObj
    {
    public:
        struct Quest
        {
            std::int32_t Id = 0;
            BYTE State = 0;
            std::uint16_t SortOrder = 0;
            std::uint32_t Current = 0;
            std::uint32_t Required = 0;
            wchar_t Name[65] = {};
            wchar_t Objective[65] = {};
        };

        CParisQuestWindow() = default;
        ~CParisQuestWindow() override;

        bool Create(CNewUIManager* manager);
        void Release();
        void OpenWindow();
        static void DispatchPacket(const BYTE* packet, int size);

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;
        float GetLayerDepth() override;
        float GetKeyEventOrder() override;

    private:
        void RequestCatalog();
        void SendAction(BYTE subCode, std::int32_t questId);
        void ReceivePacket(const BYTE* packet, int size);
        void RenderFrame();
        int SelectedQuestId() const;

        CNewUIManager* m_manager = nullptr;
        POINT m_position{ 0, 0 };
        std::vector<Quest> m_quests;
        int m_selected = -1;
        int m_scroll = 0;
        wchar_t m_status[96] = L"Pulsa Actualizar para cargar las misiones";
        DWORD m_lastRequest = 0;
    };
}
