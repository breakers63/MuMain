#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"

#include "UI/NewUI/Dialogs/ParisQuestWindow.h"
#include "UI/NewUI/Character/NewUICharacterInfoWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Legacy/UIControls.h"
#include "Network/Server/SocketSystem.h"

#include <algorithm>
#include <cstring>

namespace
{
    constexpr BYTE QuestHead = 0xED;
    constexpr BYTE ProtocolVersion = 2;
    constexpr BYTE RequestCatalogCode = 0x00;
    constexpr BYTE AcceptCode = 0x01;
    constexpr BYTE ClaimCode = 0x02;
    constexpr BYTE CatalogBeginCode = 0x10;
    constexpr BYTE QuestDefinitionCode = 0x11;
    constexpr BYTE CatalogEndCode = 0x12;
    constexpr BYTE ProgressCode = 0x13;
    constexpr BYTE ActionResultCode = 0x14;
    constexpr int QuestDefinitionLength = 669;
    constexpr int ProgressLength = 22;
    constexpr int ActionResultLength = 24;
    constexpr int QuestWindowWidth = 340;
    constexpr int QuestWindowHeight = 425;
    constexpr int VisibleQuestCount = 7;
    constexpr int RowHeight = 36;
    constexpr unsigned int PanelFill = 0xB8000000u;
    constexpr unsigned int RowFill = 0x8C050608u;
    constexpr unsigned int SelectedFill = 0xA02C2619u;

    SEASON3B::CParisQuestWindow* g_ParisQuestWindow = nullptr;

    std::uint16_t ReadUInt16(const BYTE* data)
    {
        return static_cast<std::uint16_t>(data[0] | (data[1] << 8));
    }

    std::uint32_t ReadUInt32(const BYTE* data)
    {
        return static_cast<std::uint32_t>(data[0])
            | (static_cast<std::uint32_t>(data[1]) << 8)
            | (static_cast<std::uint32_t>(data[2]) << 16)
            | (static_cast<std::uint32_t>(data[3]) << 24);
    }

    std::int32_t ReadInt32(const BYTE* data)
    {
        return static_cast<std::int32_t>(ReadUInt32(data));
    }

    void DecodeUtf8(wchar_t* destination, size_t capacity, const BYTE* source, size_t length)
    {
        if (capacity == 0)
        {
            return;
        }

        size_t out = 0;
        for (size_t in = 0; in < length && source[in] != 0 && out + 1 < capacity;)
        {
            std::uint32_t code = source[in++];
            if ((code & 0xE0u) == 0xC0u && in < length)
            {
                code = ((code & 0x1Fu) << 6) | (source[in++] & 0x3Fu);
            }
            else if ((code & 0xF0u) == 0xE0u && in + 1 < length)
            {
                code = ((code & 0x0Fu) << 12)
                    | ((source[in++] & 0x3Fu) << 6)
                    | (source[in++] & 0x3Fu);
            }
            destination[out++] = static_cast<wchar_t>(code);
        }
        destination[out] = L'\0';
    }

    void SetTextColor(int red, int green, int blue)
    {
        g_pRenderText->SetTextColor(red, green, blue, 255);
    }
}

using namespace SEASON3B;

CParisQuestWindow::~CParisQuestWindow()
{
    Release();
}

bool CParisQuestWindow::Create(CNewUIManager* manager)
{
    if (manager == nullptr)
    {
        return false;
    }
    m_manager = manager;
    m_manager->AddUIObj(INTERFACE_PARIS_QUEST, this);
    m_position.x = 640 - QuestWindowWidth - 10;
    m_position.y = (480 - QuestWindowHeight) / 2;
    g_ParisQuestWindow = this;
    Show(false);
    return true;
}

void CParisQuestWindow::Release()
{
    if (m_manager != nullptr)
    {
        m_manager->RemoveUIObj(this);
        m_manager = nullptr;
    }
    if (g_ParisQuestWindow == this)
    {
        g_ParisQuestWindow = nullptr;
    }
}

void CParisQuestWindow::DispatchPacket(const BYTE* packet, int size)
{
    if (g_ParisQuestWindow != nullptr)
    {
        g_ParisQuestWindow->ReceivePacket(packet, size);
    }
}

void CParisQuestWindow::OpenWindow()
{
    Show(true);
    RequestCatalog();
}

void CParisQuestWindow::RenderFrame()
{
    UI::Paris::Frame(m_position.x, m_position.y, QuestWindowWidth, QuestWindowHeight);
    constexpr int ContentInset = 15, ContentTop = 62, ContentBottom = 118;
    RenderColorQuadARGB(m_position.x + ContentInset, m_position.y + ContentTop,
        QuestWindowWidth - ContentInset * 2, QuestWindowHeight - ContentBottom, PanelFill);
    constexpr int ButtonInset = 20, ButtonTop = 54, ButtonHeight = 30;
    UI::Paris::GoldBorder(m_position.x + ButtonInset, m_position.y + QuestWindowHeight - ButtonTop,
        QuestWindowWidth - ButtonInset * 2, ButtonHeight);
}

void CParisQuestWindow::RequestCatalog()
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        wcscpy_s(m_status, L"Sin conexión con el servidor");
        return;
    }
    const DWORD now = GetTickCount();
    if (m_lastRequest != 0 && now - m_lastRequest < 500)
    {
        return;
    }
    m_lastRequest = now;
    const BYTE packet[] = { 0xC1, 5, QuestHead, RequestCatalogCode, 1 };
    SocketClient->Send(packet, sizeof(packet));
    wcscpy_s(m_status, L"Cargando misiones...");
}

void CParisQuestWindow::SendAction(BYTE subCode, std::int32_t questId)
{
    if (SocketClient == nullptr || !SocketClient->IsConnected())
    {
        wcscpy_s(m_status, L"Sin conexión con el servidor");
        return;
    }
    const BYTE packet[] = {
        0xC1, 8, QuestHead, subCode,
        static_cast<BYTE>(questId & 0xFF),
        static_cast<BYTE>((questId >> 8) & 0xFF),
        static_cast<BYTE>((questId >> 16) & 0xFF),
        static_cast<BYTE>((questId >> 24) & 0xFF)
    };
    SocketClient->Send(packet, sizeof(packet));
    wcscpy_s(m_status, subCode == AcceptCode ? L"Enviando solicitud..." : L"Solicitando recompensa...");
}

void CParisQuestWindow::ReceivePacket(const BYTE* packet, int size)
{
    if (packet == nullptr || size < 5)
    {
        return;
    }
    const bool shortHeader = packet[0] == 0xC1 || packet[0] == 0xC3;
    const bool longHeader = packet[0] == 0xC2 || packet[0] == 0xC4;
    if ((!shortHeader && !longHeader) || size <= (shortHeader ? 3 : 4))
    {
        return;
    }
    const int headOffset = shortHeader ? 2 : 3;
    const int subOffset = shortHeader ? 3 : 4;
    if (packet[headOffset] != QuestHead)
    {
        return;
    }

    const BYTE subCode = packet[subOffset];
    if (subCode == CatalogBeginCode && size >= 8 && packet[4] == ProtocolVersion)
    {
        m_quests.clear();
        m_selected = -1;
        m_scroll = 0;
        wcscpy_s(m_status, L"Recibiendo misiones...");
        return;
    }
    if (subCode == QuestDefinitionCode && packet[0] == 0xC2 && size >= QuestDefinitionLength)
    {
        const BYTE* payload = packet + 5;
        if (payload[0] != ProtocolVersion)
        {
            return;
        }
        Quest quest;
        quest.Id = ReadInt32(payload + 1);
        quest.State = payload[7];
        quest.SortOrder = ReadUInt16(payload + 8);
        quest.Current = ReadUInt32(payload + 14);
        quest.Required = ReadUInt32(payload + 18);
        DecodeUtf8(quest.Name, _countof(quest.Name), payload + 29, 64);
        DecodeUtf8(quest.Objective, _countof(quest.Objective), payload + 285, 64);
        const auto existing = std::find_if(m_quests.begin(), m_quests.end(), [&quest](const Quest& item)
        {
            return item.Id == quest.Id;
        });
        if (existing == m_quests.end())
        {
            m_quests.push_back(quest);
        }
        else
        {
            *existing = quest;
        }
        return;
    }
    if (subCode == CatalogEndCode && size >= 5 && packet[4] == ProtocolVersion)
    {
        std::stable_sort(m_quests.begin(), m_quests.end(), [](const Quest& left, const Quest& right)
        {
            return left.SortOrder < right.SortOrder;
        });
        wcscpy_s(m_status, L"Catálogo de misiones actualizado");
        return;
    }
    if (subCode == ProgressCode && size >= ProgressLength && packet[4] == ProtocolVersion)
    {
        const std::int32_t id = ReadInt32(packet + 5);
        const auto quest = std::find_if(m_quests.begin(), m_quests.end(), [id](const Quest& item)
        {
            return item.Id == id;
        });
        if (quest != m_quests.end())
        {
            quest->State = packet[9];
            quest->Current = ReadUInt32(packet + 10);
            quest->Required = ReadUInt32(packet + 14);
        }
        return;
    }
    if (subCode == ActionResultCode && size >= ActionResultLength && packet[4] == ProtocolVersion)
    {
        if (packet[6] == 0)
        {
            wcscpy_s(m_status, L"Operación completada");
            m_lastRequest = 0;
            RequestCatalog();
        }
        else
        {
            wcscpy_s(m_status, L"El servidor rechazó la operación");
        }
    }
}

int CParisQuestWindow::SelectedQuestId() const
{
    return m_selected >= 0 && m_selected < static_cast<int>(m_quests.size())
        ? m_quests[m_selected].Id
        : -1;
}

bool CParisQuestWindow::UpdateMouseEvent()
{
    if (!IsVisible() || !CheckMouseIn(m_position.x, m_position.y, QuestWindowWidth, QuestWindowHeight))
    {
        return false;
    }
    if (!IsRelease(VK_LBUTTON))
    {
        if (MouseWheel != 0 && CheckMouseIn(m_position.x + 18, m_position.y + 68, QuestWindowWidth - 36, VisibleQuestCount * RowHeight))
        {
            const int maximum = std::max(0, static_cast<int>(m_quests.size()) - VisibleQuestCount);
            m_scroll = std::clamp(m_scroll - MouseWheel, 0, maximum);
            MouseWheel = 0;
            return false;
        }
        return false;
    }
    const int localX = MouseX - m_position.x;
    const int localY = MouseY - m_position.y;
    if (localX > QuestWindowWidth - 34 && localY < 35)
    {
        g_pNewUISystem->Hide(INTERFACE_PARIS_QUEST);
        return false;
    }
    if (localX >= 18 && localX <= QuestWindowWidth - 18 && localY >= 68 && localY < 68 + VisibleQuestCount * RowHeight)
    {
        const int row = (localY - 68) / RowHeight;
        const int questIndex = m_scroll + row;
        if (questIndex < static_cast<int>(m_quests.size()))
        {
            m_selected = questIndex;
        }
        return false;
    }
    if (localY >= QuestWindowHeight - 54)
    {
        const int id = SelectedQuestId();
        if (id < 0)
        {
            RequestCatalog();
        }
        else
        {
            SendAction(m_quests[m_selected].State == 4 ? ClaimCode : AcceptCode, id);
        }
    }
    return false;
}

bool CParisQuestWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(INTERFACE_PARIS_QUEST);
        return false;
    }
    return true;
}

bool CParisQuestWindow::Update() { return true; }

bool CParisQuestWindow::Render()
{
    if (!IsVisible())
    {
        return true;
    }
    EnableAlphaTest();
    RenderFrame();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 210, 88, 255);
    g_pRenderText->RenderText(m_position.x + 18, m_position.y + 24, L"QUESTS PARIS", QuestWindowWidth - 36, 0, RT3_SORT_CENTER);
    const int visible = std::min(VisibleQuestCount, static_cast<int>(m_quests.size()) - m_scroll);
    for (int row = 0; row < visible; ++row)
    {
        const int questIndex = m_scroll + row;
        const int rowY = m_position.y + 68 + row * RowHeight;
        RenderColorQuadARGB(static_cast<float>(m_position.x + 18), static_cast<float>(rowY),
            static_cast<float>(QuestWindowWidth - 36), static_cast<float>(RowHeight - 2),
            questIndex == m_selected ? SelectedFill : RowFill);
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(232, 206, 135, 255);
        g_pRenderText->RenderText(m_position.x + 26, rowY + 3, m_quests[questIndex].Name, QuestWindowWidth - 52, 0, RT3_SORT_LEFT);
        wchar_t progress[112] = {};
        swprintf_s(progress, L"%ls  %u / %u", m_quests[questIndex].Objective, m_quests[questIndex].Current, m_quests[questIndex].Required);
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(205, 208, 211, 255);
        g_pRenderText->RenderText(m_position.x + 26, rowY + 19, progress, QuestWindowWidth - 52, 0, RT3_SORT_LEFT);
    }
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(205, 208, 211, 255);
    g_pRenderText->RenderText(m_position.x + 16, m_position.y + QuestWindowHeight - 72, m_status, QuestWindowWidth - 32, 0, RT3_SORT_CENTER);
    const wchar_t* action = m_selected >= 0 && m_selected < static_cast<int>(m_quests.size()) && m_quests[m_selected].State == 4
        ? L"Reclamar recompensa" : (m_selected >= 0 ? L"Aceptar misión" : L"Actualizar misiones");
    g_pRenderText->SetTextColor(255, 210, 88, 255);
    g_pRenderText->RenderText(m_position.x + 30, m_position.y + QuestWindowHeight - 46, action, QuestWindowWidth - 60, 0, RT3_SORT_CENTER);
    DisableAlphaBlend();
    return true;
}

float CParisQuestWindow::GetLayerDepth() { return 8.4f; }
float CParisQuestWindow::GetKeyEventOrder() { return 8.4f; }
