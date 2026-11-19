#pragma once



#include "UI/Core/WindowObject.h"

#include "UI/RmlBridge/RmlThemedView.h"

#include "Network/Paris/BountyBoardProtocol.h"



#include <vector>



namespace mu::ui::window

{

class CManager;



class CParisBountyBoardWindow final : public CObject

{

public:

    ~CParisBountyBoardWindow() override;

    bool Create(CManager* manager);

    void Release();

    void OpenWindow();

    void CloseWindow();

    void Show(bool show) override;

    bool UpdateMouseEvent() override;

    bool UpdateKeyEvent() override;

    bool TakesTypingFrom(const Rml::ElementDocument* document) const override

    {

        return document != nullptr && document == m_RmlView.Document();

    }

    bool Update() override;

    bool Render() override;

    float GetLayerDepth() override { return 62.5f; }

    float GetKeyEventOrder() override { return 101.f; }



private:

    struct Row

    {

        Rml::String name;

        Rml::String detail;

        Rml::String reward;

        Rml::String action;

        Rml::String index_label;

        int index = 0;

        bool enabled = false;

        bool operator==(const Row&) const = default;

    };



    struct Model

    {

        Rml::String status;

        Rml::String feature_label = "SE BUSCA";

        Rml::String feature_name = "Selecciona una cacería";

        Rml::String feature_reward = "—";

        Rml::String feature_time;

        Rml::String table_name_heading = "BUSCADO";

        Rml::String table_status_heading = "ESTADO";

        Rml::String table_reward_heading = "RECOMPENSA";

        Rml::String query;

        Rml::String amount = "1000000";

        Rml::String selected;

        Rml::String currency = "Zen";

        std::vector<Row> rows;

        int mode = 0;

        bool empty = true;

        bool enabled = false;

        bool currency_open = false;

    };



    void BindModel(Rml::DataModelConstructor& constructor, Model& model);

    void SyncModel();

    void SearchCharacter(bool typing = false);

    void SendQueuedSearch();

    void SendRequest(BYTE operation, const std::wstring& name = {},

        const Network::Paris::BountyBoard::Row* row = nullptr, long long amount = 0);

    void HandleRowAction(int index);

    void SetMode(int mode);

    void StepAmount(int direction);

    bool ReadAmount(long long& amount) const;



    CManager* m_manager = nullptr;

    DWORD m_lastReceived = 0;

    DWORD m_requestStarted = 0;

    BYTE m_pendingOperation = 0;

    int m_mode = 0;

    int m_currency = 0;

    bool m_waiting = false;

    bool m_hasSentRequest = false;

    bool m_discardSearchResponse = false;

    DWORD m_searchQueuedAt = 0;

    DWORD m_searchDelay = 0;

    std::wstring m_queuedSearch;

    std::array<BYTE, 16> m_targetId{};

    std::wstring m_targetName;

    UI::RmlBridge::ThemedView<Model> m_RmlView{"paris_bounty_board",

        [this](Rml::DataModelConstructor& c, Model& model) { BindModel(c, model); },

        {{"Data/Interface/RmlUi/paris_bounty_board.rml"}},

        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};

};



extern CParisBountyBoardWindow* g_pParisBountyBoardWindow;

}

