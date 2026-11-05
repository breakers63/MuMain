#pragma once
#include "GameLogic/Character/StatAllocationState.h"
#include "Network/Paris/StatLimitsSnapshot.h"
#include <RmlUi/Core/DataModelHandle.h>
#include <mutex>
#include <optional>
#include <vector>
namespace UI::Character {
class ParisStatAllocationController
{
public:
    ParisStatAllocationController();
    ~ParisStatAllocationController();
    void Bind(Rml::DataModelConstructor& constructor);
    bool IsOpen() const { return m_state.open; }
    void Open();
    void Hide();
    void Tick();
private:
    struct Row { Rml::String name, amount = "0"; int index = 0; bool full = false, enabled = true; };
    void Dirty(const char* variable);
    void RequestLimits();
    bool ReadFields();
    void WriteFields();
    void Apply();
    void Close();
    void ToggleFull(int index);
    void SetMode(bool percent);
    void ToggleAutomatic();
    void ConsumeLimits();
    void UpdateAutomatic(unsigned int points);
    void UpdateAssignment(unsigned int points, unsigned long now);
    std::array<int,5> Room() const;
    unsigned int Base(int index) const;
    int Available() const;
    void SetStatus();
    GameLogic::Character::StatAllocationState m_state;
    std::vector<Row> m_rows;
    Rml::String m_status, m_available;
    Rml::DataModelHandle m_model;
    std::wstring m_owner;
    std::array<int,5> m_limits{};
    std::uint64_t m_textHandler = 0;
    std::uint32_t m_request = 0;
    unsigned long m_limitsAt = 0;
    unsigned int m_previousPoints = 0, m_previousLevel = 0, m_previousReset = 0;
    int m_autoBudget = 0;
    bool m_limitsReady = false, m_limitsPending = false;
    std::mutex m_receiveMutex;
    std::optional<Network::Paris::StatLimitsSnapshot> m_receivedLimits;
};
}
