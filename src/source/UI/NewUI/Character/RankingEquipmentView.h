#pragma once

#include "UI/NewUI/NewUI3DRenderMng.h"
#include <array>
#include <string>

typedef struct tagITEM ITEM;

namespace UI::Character
{
    class RankingEquipmentView : public SEASON3B::INewUI3DRenderObj
    {
    public:
        void Initialize(SEASON3B::CNewUI3DRenderMng* manager);
        void Release();
        void Clear();
        void RequestProfile(const std::wstring& name);
        bool Receive(const BYTE* packet, int size);
        void Render(int x, int y, int width, int height, bool visible);
        void Render3D() override;
        bool IsVisible() const override;
        SEASON3B::CNewUIObj* GetLayoutOwner() const override;
        void RefreshOwnPreference();
        bool OwnDetailsPublic() const;
        bool PreferenceReady() const;
        void SetOwnDetailsPublic(bool value);
        void SetReferenceLayout(bool enabled, float physicalScale);

    private:
        static void TooltipCallback(LPVOID object, DWORD slot, DWORD unused);
        void SendRequest(BYTE mode, bool details, const std::wstring& name);
        SEASON3B::CNewUI3DRenderMng* m_Manager = nullptr;
        std::array<ITEM*, 12> m_Items{};
        std::wstring m_TargetName;
        std::wstring m_OwnerName;
        bool m_DetailsPublic = false;
        bool m_OwnPublic = false;
        bool m_PreferenceKnown = false;
        bool m_PreferencePending = false;
        bool m_Visible = false;
        DWORD m_PreferenceRequestTick = 0;
        bool m_ReferenceLayout = false;
        int m_FontHeight = 0;
        HFONT m_LabelFont = nullptr, m_NameFont = nullptr;
        int m_X = 0, m_Y = 0, m_Width = 0, m_Height = 0;
    };

    RankingEquipmentView& RankingEquipment();
}
