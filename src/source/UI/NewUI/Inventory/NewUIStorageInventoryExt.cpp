//*****************************************************************************
// File: NewUIStorageInventory.cpp
//*****************************************************************************

#include "stdafx.h"
#include "UI/NewUI/Inventory/VaultPanelArt.h"
namespace { UI::Inventory::VaultPanelArt extendedVaultArt; }
#include "UI/NewUI/Inventory/NewUIStorageInventoryExt.h"
#include "I18N/All.h"

#include "Audio/DSPlaySound.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Dialogs/NewUICustomMessageBox.h"
#include "Engine/Object/ZzzInventory.h"
#include "GameLogic/Items/TradeRestrictions.h"

using namespace SEASON3B;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// cppcheck-suppress uninitMemberVar
CNewUIStorageInventoryExt::CNewUIStorageInventoryExt()
{
    m_pNewUIMng = nullptr;
    m_pNewInventoryCtrl = nullptr;
    m_Pos.x = m_Pos.y = 0;
    m_nBackupSourceInvenIndex = -1;
}

CNewUIStorageInventoryExt::~CNewUIStorageInventoryExt()
{
    Release();
}

bool CNewUIStorageInventoryExt::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (nullptr == pNewUIMng || nullptr == g_pNewUI3DRenderMng || nullptr == g_pNewItemMng)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_STORAGE_EXT, this);

    m_pNewInventoryCtrl = new CNewUIInventoryCtrl;
    if (false == m_pNewInventoryCtrl->Create(STORAGE_TYPE::VAULT, g_pNewUI3DRenderMng, g_pNewItemMng, this, x + 15,
                                             y + 36, 8, 15, MAX_SHOP_INVENTORY))
    {
        SAFE_DELETE(m_pNewInventoryCtrl);
        return false;
    }

    m_pNewInventoryCtrl->SetGoldTheme(true);
    SetPos(x, y);
    LoadImages();
    m_BtnExit.ChangeButtonImgState(true, IMAGE_INVENTORY_EXIT_BTN, false);
    m_BtnExit.ChangeToolTipText(&I18N::Game::Close388, true);
    SetItemAutoMove(false);

    Show(false);

    return true;
}

void CNewUIStorageInventoryExt::Release()
{
    UnloadImages();

    SAFE_DELETE(m_pNewInventoryCtrl);

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CNewUIStorageInventoryExt::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    if (m_pNewInventoryCtrl) m_pNewInventoryCtrl->SetPos(x + 15, y + 46);
    m_BtnExit.ChangeButtonInfo(x + UI::Inventory::VaultPanelArt::CloseLeft,
        y + UI::Inventory::VaultPanelArt::FooterTop,
        UI::Inventory::VaultPanelArt::ButtonSize, UI::Inventory::VaultPanelArt::ButtonSize);
}

bool CNewUIStorageInventoryExt::UpdateMouseEvent()
{
    if (m_pNewInventoryCtrl && false == m_pNewInventoryCtrl->UpdateMouseEvent())
        return false;

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(INTERFACE_STORAGE_EXT);
        return false;
    }

    ProcessInventoryCtrl();

    if (ProcessBtns())
        return false;

    if (CheckMouseIn(m_Pos.x, m_Pos.y, STORAGE_WIDTH, STORAGE_HEIGHT))
    {
        if (IsPress(VK_RBUTTON))
        {
            MouseRButton = false;
            MouseRButtonPop = false;
            MouseRButtonPush = false;
            return false;
        }

        if (IsNone(VK_LBUTTON) == false)
        {
            return false;
        }
    }

    return true;
}

bool CNewUIStorageInventoryExt::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(INTERFACE_STORAGE) == true)
    {
        if (IsPress(VK_ESCAPE) == true)
        {
            g_pNewUISystem->Hide(INTERFACE_STORAGE);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }
    }
    return true;
}

bool CNewUIStorageInventoryExt::Update()
{
    if (m_pNewInventoryCtrl && false == m_pNewInventoryCtrl->Update())
        return false;

    return true;
}

bool CNewUIStorageInventoryExt::Render()
{
    EnableAlphaTest();

    RenderBackImage();
    RenderText();

    if (m_pNewInventoryCtrl)
    {
        m_pNewInventoryCtrl->Render();
    }

    extendedVaultArt.Close(m_Pos);

    DisableAlphaBlend();

    return true;
}

void CNewUIStorageInventoryExt::RenderBackImage() const
{
    extendedVaultArt.Frame(m_Pos);
}

void CNewUIStorageInventoryExt::RenderText() const
{
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(255, 220, 130, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + 37, I18N::Game::ExpandedVault, STORAGE_WIDTH, 0, RT3_SORT_CENTER);
}

float CNewUIStorageInventoryExt::GetLayerDepth()
{
    return 2.2f;
}

CNewUIInventoryCtrl* CNewUIStorageInventoryExt::GetInventoryCtrl() const
{
    return m_pNewInventoryCtrl;
}

void CNewUIStorageInventoryExt::LoadImages() const
{
    extendedVaultArt.Load();
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_STORAGE_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_STORAGE_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_STORAGE_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_STORAGE_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_STORAGE_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_INVENTORY_EXIT_BTN, GL_LINEAR);
}

void CNewUIStorageInventoryExt::UnloadImages()
{
    extendedVaultArt.Unload();
    DeleteBitmap(IMAGE_INVENTORY_EXIT_BTN);
    DeleteBitmap(IMAGE_STORAGE_BOTTOM);
    DeleteBitmap(IMAGE_STORAGE_RIGHT);
    DeleteBitmap(IMAGE_STORAGE_LEFT);
    DeleteBitmap(IMAGE_STORAGE_TOP);
    DeleteBitmap(IMAGE_STORAGE_BACK);
}

bool CNewUIStorageInventoryExt::ProcessClosing() const
{
    if (EquipmentItem)
        return false;

    CNewUIInventoryCtrl::BackupPickedItem();
    DeleteAllItems();
    SocketClient->ToGameServer()->SendVaultClosed();
    return true;
}

bool CNewUIStorageInventoryExt::InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket) const
{
    if (m_pNewInventoryCtrl)
        return m_pNewInventoryCtrl->AddItem(iIndex, pbyItemPacket);

    return false;
}

void CNewUIStorageInventoryExt::DeleteAllItems() const
{
    if (m_pNewInventoryCtrl)
        m_pNewInventoryCtrl->RemoveAllItems();
}

void CNewUIStorageInventoryExt::ProcessInventoryCtrl()
{
    if (nullptr == m_pNewInventoryCtrl)
    {
        return;
    }

    if (const auto pPickedItem = CNewUIInventoryCtrl::GetPickedItem())
    {
        ITEM* pItemObj = pPickedItem->GetItem();
        if (pItemObj == nullptr)
        {
            return;
        }

        if (IsPress(VK_LBUTTON) || IsRelease(VK_LBUTTON))
        {
            const int nDstIndex = pPickedItem->GetTargetLinealPos(m_pNewInventoryCtrl);

            if (nDstIndex >= 0 && m_pNewInventoryCtrl->CanMove(nDstIndex, pItemObj))
            {
                const int nSrcIndex = pPickedItem->GetSourceLinealPos();
                const auto sourceStorageType = pPickedItem->GetSourceStorageType();
                const auto targetStorageType = m_pNewInventoryCtrl->GetStorageType();
                SendRequestEquipmentItem(sourceStorageType, nSrcIndex, pItemObj, targetStorageType, nDstIndex);
            }
        }
        else
        {
            if (GameLogic::Items::IsStoreBan(pItemObj))
            {
                m_pNewInventoryCtrl->SetSquareColorNormal(1.0f, 0.0f, 0.0f);
            }
            else
            {
                m_pNewInventoryCtrl->SetSquareColorNormal(0.1f, 0.4f, 0.8f);
            }
        }
    }
    else if (IsPress(VK_RBUTTON))
    {
        ProcessStorageItemAutoMove();
    }
}

void CNewUIStorageInventoryExt::ProcessStorageItemAutoMove()
{
    if (g_pPickedItem)
        if (g_pPickedItem->GetItem())
            return;

    if (IsItemAutoMove())
        return;

    if (const auto pItemObj = m_pNewInventoryCtrl->FindItemAtPt(MouseX, MouseY))
    {
        const int nDstIndex = g_pMyInventory->FindEmptySlotIncludingExtensions(pItemObj);
        if (-1 != nDstIndex)
        {
            SetItemAutoMove(true);

            const int nSrcIndex = m_pNewInventoryCtrl->GetIndexByItem(pItemObj);
            g_pStorageInventory->SendRequestItemToMyInven(pItemObj, nSrcIndex, nDstIndex);

            PlayBuffer(SOUND_GET_ITEM01);
        }
    }
}

bool CNewUIStorageInventoryExt::ProcessMyInvenItemAutoMove(CNewUIInventoryCtrl* sourceCtrl)
{
    if (g_pPickedItem && g_pPickedItem->GetItem())
    {
        return false;
    }

    if (IsItemAutoMove())
    {
        return false;
    }

    if (sourceCtrl == nullptr)
    {
        sourceCtrl = g_pMyInventory->GetInventoryCtrl();
    }

    if (sourceCtrl == nullptr)
    {
        return false;
    }

    if (const auto pItemObj = sourceCtrl->FindItemAtPt(MouseX, MouseY))
    {
        if (pItemObj->Type == ITEM_WIZARDS_RING)
            return false;

        const int emptySlotIndex = FindEmptySlot(pItemObj);
        if (emptySlotIndex != -1)
        {
            const int nSrcIndex = sourceCtrl->GetIndexByItem(pItemObj);
            if (nSrcIndex < 0)
            {
                return false;
            }

            SetItemAutoMove(true, nSrcIndex);
            g_pStorageInventory->SendRequestItemToStorage(pItemObj, nSrcIndex, emptySlotIndex);
            PlayBuffer(SOUND_GET_ITEM01);
            return true;
        }
    }

    return false;
}

bool CNewUIStorageInventoryExt::ProcessBtns() const
{
    if (!extendedVaultArt.ClosePressed(m_Pos)) return false;
    g_pNewUISystem->Hide(INTERFACE_STORAGE_EXT);
    return true;
}

void CNewUIStorageInventoryExt::SetItemAutoMove(bool bItemAutoMove, int nSourceInvenIndex)
{
    m_bItemAutoMove = bItemAutoMove;

    if (bItemAutoMove)
    {
        m_nBackupMouseX = MouseX;
        m_nBackupMouseY = MouseY;
        m_nBackupSourceInvenIndex = nSourceInvenIndex;
    }
    else
    {
        m_nBackupMouseX = m_nBackupMouseY = 0;
        m_nBackupSourceInvenIndex = -1;
    }
}

int CNewUIStorageInventoryExt::FindEmptySlot(const ITEM* pItemObj) const
{
    if (pItemObj == nullptr)
        return -1;

    const ITEM_ATTRIBUTE* pItemAttr = &ItemAttribute[pItemObj->Type];

    if (m_pNewInventoryCtrl)
        return m_pNewInventoryCtrl->FindEmptySlot(pItemAttr->Width, pItemAttr->Height);

    return -1;
}

void CNewUIStorageInventoryExt::ProcessToReceiveStorageItems(int nIndex, std::span<const BYTE> pbyItemPacket)
{
    CNewUIInventoryCtrl::DeletePickedItem();

    if (IsItemAutoMove())
    {
        if (m_nBackupSourceInvenIndex >= MAX_EQUIPMENT_INDEX && m_nBackupSourceInvenIndex < MAX_MY_INVENTORY_INDEX)
        {
            g_pMyInventory->DeleteItem(m_nBackupSourceInvenIndex);
        }
        else if (m_nBackupSourceInvenIndex >= MAX_MY_INVENTORY_INDEX &&
                 m_nBackupSourceInvenIndex < MAX_MY_INVENTORY_EX_INDEX)
        {
            g_pMyInventoryExt->DeleteItem(m_nBackupSourceInvenIndex);
        }
        else
        {
            CNewUIInventoryCtrl* pMyInvenCtrl = g_pMyInventory->GetInventoryCtrl();
            ITEM* pItemObj = pMyInvenCtrl->FindItemAtPt(m_nBackupMouseX, m_nBackupMouseY);
            g_pMyInventory->GetInventoryCtrl()->RemoveItem(pItemObj);
        }

        SetItemAutoMove(false);
    }

    InsertItem(nIndex, pbyItemPacket);
}

void CNewUIStorageInventoryExt::ProcessStorageItemAutoMoveSuccess()
{
    if (!IsVisible())
        return;

    if (IsItemAutoMove())
    {
        ITEM* pItemObj = m_pNewInventoryCtrl->FindItemAtPt(m_nBackupMouseX, m_nBackupMouseY);
        m_pNewInventoryCtrl->RemoveItem(pItemObj);

        SetItemAutoMove(false);
    }
}

void CNewUIStorageInventoryExt::ProcessStorageItemAutoMoveFailure()
{
    if (!IsVisible())
        return;

    SetItemAutoMove(false);
}

int CNewUIStorageInventoryExt::GetPointedItemIndex() const
{
    return m_pNewInventoryCtrl->GetPointedSquareIndex();
}
