#include "stdafx.h"
#include "Render/Renderer/MuRenderer.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "RankingEquipmentView.h"
#include "NewUICustomRanking.h"
#include "ParisFlatStyle.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Character/ParisPanelStyle.h"
#include "Network/Server/RankingEquipmentPacket.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzInventory.h"
#include "Scenes/SceneCore.h"
#include "UI/NewUI/Inventory/ParisItemPreview.h"
#include <cstring>
#include <algorithm>

namespace UI::Character
{
    namespace
    {
        constexpr DWORD PreferenceRetryMilliseconds = 4000;
        constexpr int Columns = 2;
        constexpr int Rows = 6;
        constexpr int LabelHeight = 16;
        constexpr std::array<std::size_t, 12> CardSlots{2, 3, 4, 5, 6, 0, 1, 7, 9, 10, 11, 8};
        constexpr const wchar_t* SlotLabels[]{L"ARMA", L"ARMA / ESCUDO", L"CASCO", L"ARMADURA", L"PANTALON", L"GUANTES", L"BOTAS", L"ALAS", L"MASCOTA", L"AMULETO", L"ANILLO", L"ANILLO"};
        constexpr int IconWidth = 30;
        constexpr float EquipmentIconScale = .55f;
        RankingEquipmentView equipment;

        void SplitEquipmentName(wchar_t* name, wchar_t (&secondLine)[128])
        {
            constexpr std::size_t NameLineLength = 13;
            if (wcslen(name) > NameLineLength)
            {
                std::size_t split = NameLineLength;
                while (split > 5 && name[split] != L' ') --split;
                if (split <= 5) split = NameLineLength;
                wcscpy_s(secondLine, name + split + (name[split] == L' ' ? 1 : 0));
                name[split] = 0;
                if (wcslen(secondLine) > NameLineLength)
                {
                    secondLine[NameLineLength - 3] = L'.';
                    secondLine[NameLineLength - 2] = L'.';
                    secondLine[NameLineLength - 1] = L'.';
                    secondLine[NameLineLength] = 0;
                }
            }
        }

    }

    RankingEquipmentView& RankingEquipment() { return equipment; }
    SEASON3B::CNewUIObj* RankingEquipmentView::GetLayoutOwner() const { return g_pCustomRankingWindow; }

    void RankingEquipmentView::Initialize(SEASON3B::CNewUI3DRenderMng* manager)
    {
        Release();
        m_Manager = manager;
        if (m_Manager) m_Manager->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);
    }

    void RankingEquipmentView::Release()
    {
        if (m_Manager)
        {
            m_Manager->Remove3DRenderObj(this);
            m_Manager->DeleteUI2DEffectObject(TooltipCallback);
            m_Manager = nullptr;
        }
        if (m_LabelFont) DeleteObject(m_LabelFont);
        if (m_NameFont) DeleteObject(m_NameFont);
        m_LabelFont = m_NameFont = nullptr; m_FontHeight = 0;
        Clear();
        m_OwnerName.clear();
        m_PreferenceKnown = m_PreferencePending = false;
    }

    void RankingEquipmentView::Clear()
    {
        for (auto& item : m_Items)
        {
            if (item && g_pNewItemMng) g_pNewItemMng->DeleteItem(item);
            item = nullptr;
        }
        m_TargetName.clear();
        m_DetailsPublic = m_Visible = false;
    }

    void RankingEquipmentView::SendRequest(BYTE mode, bool details, const std::wstring& name)
    {
        if (SceneFlag != MAIN_SCENE || !SocketClient || !SocketClient->IsConnected()) return;
        BYTE packet[17]{0xC1, 17, 0xED, Network::Ranking::EquipmentRequest, 1, mode, static_cast<BYTE>(details)};
        char utf8[41]{};
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), utf8, sizeof(utf8), nullptr, nullptr);
        if (bytes > 10) return;
        if (bytes > 0) std::memcpy(packet + 7, utf8, bytes);
        SocketClient->Send(packet, sizeof(packet));
    }

    void RankingEquipmentView::RequestProfile(const std::wstring& name)
    {
        Clear();
        m_TargetName = name;
        SendRequest(0, false, name);
    }

    void RankingEquipmentView::RefreshOwnPreference()
    {
        if (SceneFlag != MAIN_SCENE || !Hero) return;
        if (m_OwnerName != Hero->ID)
        {
            m_OwnerName = Hero->ID;
            m_PreferenceKnown = m_PreferencePending = false;
        }
        if (m_PreferencePending && GetTickCount() - m_PreferenceRequestTick >= PreferenceRetryMilliseconds)
            m_PreferencePending = false;
        if (m_PreferenceKnown || m_PreferencePending
            || GetTickCount() - m_PreferenceRequestTick < PreferenceRetryMilliseconds) return;
        m_PreferenceRequestTick = GetTickCount();
        m_PreferencePending = true;
        SendRequest(1, false, {});
    }

    bool RankingEquipmentView::OwnDetailsPublic() const { return m_OwnPublic; }
    bool RankingEquipmentView::PreferenceReady() const { return m_PreferenceKnown && !m_PreferencePending; }

    void RankingEquipmentView::SetOwnDetailsPublic(bool value)
    {
        if (!PreferenceReady()) return;
        m_PreferencePending = true;
        m_PreferenceRequestTick = GetTickCount();
        SendRequest(2, value, {});
    }

    bool RankingEquipmentView::Receive(const BYTE* packet, int size)
    {
        if (size < 5 || packet[3] != 0xED || packet[4] != Network::Ranking::EquipmentResponse) return false;
        if (!Network::Ranking::IsEquipmentResponse({packet, static_cast<std::size_t>(size)})) return true;
        const bool preferenceChanged=m_PreferenceKnown && m_OwnPublic!=(packet[7]!=0);
        m_OwnPublic = packet[7] != 0;
        if(Hero && m_TargetName==Hero->ID && preferenceChanged)
        {
            m_DetailsPublic=m_OwnPublic;
            SendRequest(0,false,m_TargetName);
        }
        m_PreferenceKnown = true;
        m_PreferencePending = false;
        wchar_t name[11]{};
        int length = 0;
        while (length < 10 && packet[8 + length]) ++length;
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(packet + 8), length, name, 10);
        if (m_TargetName != name || !packet[18] || !g_pNewItemMng) return true;
        const auto target = m_TargetName;
        Clear();
        m_TargetName = target;
        m_DetailsPublic = packet[6] != 0;
        for (std::size_t slot = 0; slot < m_Items.size(); ++slot)
        {
            const BYTE* record = packet + 19 + slot * Network::Ranking::EquipmentRecordSize;
            if (!record[0]) continue;
            const auto parameters = ParseItemData({record + 2, record[1]});
            if (parameters.Group < 0 || parameters.Group >= MAX_ITEM_TYPE
                || parameters.Number < 0 || parameters.Number >= MAX_ITEM_INDEX) continue;
            m_Items[slot] = g_pNewItemMng->CreateItemByParameters(&parameters);
        }
        return true;
    }

    void RankingEquipmentView::SetReferenceLayout(bool enabled, float physicalScale)
    {
        m_ReferenceLayout = enabled;
        const int height = std::max(8, static_cast<int>(11.f * physicalScale));
        if (m_FontHeight == height) return;
        if (m_LabelFont) DeleteObject(m_LabelFont);
        if (m_NameFont) DeleteObject(m_NameFont);
        m_FontHeight = height;
        m_LabelFont = CreateFontW(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
        m_NameFont = CreateFontW(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Georgia");
    }

    void RankingEquipmentView::Render(int x, int y, int width, int height, bool visible)
    {
        m_X = x; m_Y = y; m_Width = width; m_Height = height;
        m_Visible = visible;
        if (!visible) return;
        const int cellWidth = width / Columns, cellHeight = height / Rows;
        for (std::size_t card = 0; card < CardSlots.size(); ++card)
        {
            const auto slot = CardSlots[card];
            const int left = x + static_cast<int>(card % Columns) * cellWidth;
            const int top = y + static_cast<int>(card / Columns) * cellHeight;
            if (!m_ReferenceLayout)
            {
                ParisFlat::Box(left, top, cellWidth - 4, cellHeight - 4);
                ParisFlat::Box(left + 3, top + 5, IconWidth + 3, cellHeight - 13);
            }
            const int labelX = left + static_cast<int>(cellWidth * .27f);
            const int labelWidth = static_cast<int>(cellWidth * .70f);
            g_pRenderText->SetFont(m_LabelFont ? m_LabelFont : g_hFont);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->SetTextColor(255, 255, 255, 255);
            const auto slotLabel = slot == 1 && m_Items[slot]
                ? (m_Items[slot]->Type / MAX_ITEM_INDEX == 6 ? L"ESCUDO" : L"ARMA") : SlotLabels[slot];
            g_pRenderText->RenderText(labelX, top + static_cast<int>(cellHeight * .20f), slotLabel, labelWidth, 0, RT3_SORT_LEFT);
            wchar_t name[128]{};
            if (m_Items[slot])
            {
                swprintf_s(name, L"%ls", ItemAttribute[m_Items[slot]->Type].Name);
            }
            else swprintf_s(name, L"Vacio");
            g_pRenderText->SetTextColor(255, 255, 255, 255);
            wchar_t secondLine[128]{};
            SplitEquipmentName(name, secondLine);
            g_pRenderText->SetFont(m_NameFont ? m_NameFont : g_hFont);
            g_pRenderText->RenderText(labelX, top + static_cast<int>(cellHeight * (secondLine[0]?.43f:.52f)), name, labelWidth, 0, RT3_SORT_LEFT);
            if (secondLine[0]) g_pRenderText->RenderText(labelX, top + static_cast<int>(cellHeight * .69f), secondLine, labelWidth, 0, RT3_SORT_LEFT);
            if (m_DetailsPublic && m_Items[slot] && m_Manager && CheckMouseIn(left, top, cellWidth, cellHeight))
                m_Manager->RenderUI2DEffect(INVENTORY_CAMERA_Z_ORDER, TooltipCallback, this, static_cast<DWORD>(slot), 0);
        }
    }

    bool RankingEquipmentView::IsVisible() const { return m_Visible && SceneFlag == MAIN_SCENE; }

    void RankingEquipmentView::Render3D()
    {
        const int cellWidth = m_Width / Columns, cellHeight = m_Height / Rows;
        for (std::size_t card = 0; card < CardSlots.size(); ++card)
        {
            const auto* item = m_Items[CardSlots[card]];
            if (!item) continue;
            const bool chest = CardSlots[card]==3;
            const float width = cellWidth * .18f;
            const bool armorSlot = CardSlots[card]>=2 && CardSlots[card]<=6;
            const float iconTop = chest?.22f:(armorSlot?.28f:.18f);
            const float height = cellHeight * (chest?.58f:(armorSlot?.54f:.62f));
            const float scale = EquipmentIconScale;
            RenderMarketPreviewItem3D(static_cast<float>(m_X + card % Columns * cellWidth + cellWidth * .04f),
                static_cast<float>(m_Y + card / Columns * cellHeight + cellHeight * iconTop), width, height,
                item->Type, item->Level, item->ExcellentFlags, item->AncientDiscriminator, scale);
        }
    }

    void RankingEquipmentView::TooltipCallback(LPVOID object, DWORD slot, DWORD)
    {
        auto* view = static_cast<RankingEquipmentView*>(object);
        if (!view || !view->IsVisible() || !view->m_DetailsPublic || slot >= view->m_Items.size() || !view->m_Items[slot]) return;
        RenderMarketItemInfo(std::clamp(MouseX + 12, 12, 400), std::clamp(MouseY + 12, 30, 240), view->m_Items[slot]);
    }
}
