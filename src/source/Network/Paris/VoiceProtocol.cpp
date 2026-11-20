#include "stdafx.h"
#include "VoiceProtocol.h"
#include "Network/Server/WSclient.h"
#include "UI/Core/WindowSystem.h"
#include "UI/HUD/ChatInputBox.h"
#include "Render/RmlUi/RmlUiRuntime.h"
#include <array>
#include <cstring>

namespace Network::Paris::Voice
{
#ifdef _WIN32
namespace
{
constexpr BYTE RequestCode = 0xD9;
constexpr BYTE TicketCode = 0xDA;
constexpr int TicketLength = 64;
constexpr int TicketPacketLength = 69;
constexpr DWORD LaunchTimeoutMs = 10000;
constexpr DWORD ControlSize = 128;
constexpr int PushToTalkKey = 'K';
constexpr wchar_t HelperName[] = L"ParisVoice\\ParisVoice.exe";
HANDLE mapping = nullptr;
volatile LONG* control = nullptr;
HANDLE process = nullptr;
DWORD requestedAt = 0;
DWORD nextAutomaticLaunch = 0;


void SendAction(BYTE action)
{
    if (!SocketClient) return;
    BYTE packet[] = {0xC1, 6, 0xFA, RequestCode, 1, action};
    SocketClient->Send(packet, sizeof(packet));
}

void Stop()
{
    if (control)
    {
        InterlockedExchange(control + 3, 0);
        InterlockedExchange(control + 1, 0);
        UnmapViewOfFile(const_cast<LONG*>(control));
        control = nullptr;
    }
    if (mapping) { CloseHandle(mapping); mapping = nullptr; }
    if (process) { CloseHandle(process); process = nullptr; }
    requestedAt = 0;
}

bool CanTalk()
{
    return SceneFlag == MAIN_SCENE && GetForegroundWindow() == g_hWnd
        && g_pNewUISystem && g_pChatInputBox && !g_pChatInputBox->HaveFocus()
        && !RmlUiRuntime::Instance().IsTextInputActive()
        && !g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_INVENTORY)
        && !g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_INVENTORY_EXT)
        && !g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_CHARACTER)
        && !g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_WINDOW_MENU);
}

bool Launch(bool showPanel = true)
{
    std::array<wchar_t, MAX_PATH> path{};
    if (!GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()))) return false;
    auto* slash = wcsrchr(path.data(), L'\\');
    if (!slash) return false;
    *(slash + 1) = 0;
    const std::wstring executable = std::wstring(path.data()) + HelperName;
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    // Unnamed mapping inherited by this helper only. No credentials in process arguments.
    mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, &security, PAGE_READWRITE, 0, ControlSize, nullptr);
    if (!mapping) return false;
    control = static_cast<volatile LONG*>(MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, ControlSize));
    if (!control) { Stop(); return false; }
    ZeroMemory(const_cast<LONG*>(control), ControlSize);
    InterlockedExchange(control, 0x3156504D);
    InterlockedExchange(control + 1, 1);
    InterlockedExchange(control + 25, showPanel ? 1 : 0); // Automatic reception keeps the panel hidden.
    InterlockedExchange(control + 2, static_cast<LONG>(GetTickCount()));
    std::wstring command = L"\"" + executable + L"\" "
        + std::to_wstring(reinterpret_cast<UINT_PTR>(mapping)) + L" "
        + std::to_wstring(reinterpret_cast<UINT_PTR>(g_hWnd)) + L" "
        + std::to_wstring(GetCurrentProcessId());
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    auto attributes = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, size));
    if (!attributes) { Stop(); return false; }
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &size))
    {
        HeapFree(GetProcessHeap(), 0, attributes); Stop(); return false;
    }
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION child{};
    const bool updated = UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &mapping, sizeof(mapping), nullptr, nullptr) != FALSE;
    const bool created = updated && CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, nullptr, path.data(), &startup.StartupInfo, &child);
    DeleteProcThreadAttributeList(attributes);
    HeapFree(GetProcessHeap(), 0, attributes);
    if (!created)
    {
        Stop();
        return false;
    }
    SetHandleInformation(mapping, HANDLE_FLAG_INHERIT, 0);
    CloseHandle(child.hThread);
    process = child.hProcess;
    return true;
}
}

bool IsMicrophoneEnabled()
{
    constexpr int MicrophoneStateIndex = 104 / sizeof(LONG);
    return control && control[MicrophoneStateIndex] != 0;
}

void Toggle()
{
    if (process)
    {
        // Bring the existing panel back instead of killing its audio session.
        if (control) InterlockedIncrement(control + 25);
        return;
    }
    if (SceneFlag != MAIN_SCENE) return;
    if (!Launch())
        MessageBoxW(g_hWnd, L"No se pudo abrir ParisVoice. Instala la carpeta ParisVoice junto a Main.exe.",
            L"Paris Project - Voz", MB_OK | MB_ICONINFORMATION);
}

void Receive(const BYTE* packet, int size)
{
    if (!control || !requestedAt || size != TicketPacketLength || packet[0] != 0xC1
        || packet[1] != TicketPacketLength || packet[2] != 0xFA
        || packet[3] != TicketCode || packet[4] != 1) return;
    std::wstring ticket;
    ticket.reserve(TicketLength);
    for (int i = 0; i < TicketLength; ++i)
    {
        const BYTE value = packet[5 + i];
        if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'))) return;
        ticket.push_back(value);
    }
    requestedAt = 0;
    InterlockedIncrement(control + 6);
    auto destination = reinterpret_cast<char*>(const_cast<LONG*>(control) + 8);
    for (int i = 0; i < TicketLength; ++i) destination[i] = static_cast<char>(ticket[i]);
    destination[TicketLength] = 0;
    InterlockedExchange(control + 7, 2);
    InterlockedIncrement(control + 6);
}

void Update()
{
    // Listen upon character entry, without opening UI or enabling microphone capture.
    const DWORD now = GetTickCount();
    if (SceneFlag != MAIN_SCENE) nextAutomaticLaunch = 0;
    else if (!control && (!nextAutomaticLaunch || static_cast<LONG>(now - nextAutomaticLaunch) >= 0))
    {
        nextAutomaticLaunch = now + 30000;
        Launch(false);
    }
    if (requestedAt && GetTickCount() - requestedAt > LaunchTimeoutMs)
    {
        requestedAt = 0;
        SendAction(1);
        if (control) InterlockedExchange(control + 7, 3);
    }
    if (!control) return;
    if (SceneFlag != MAIN_SCENE || WaitForSingleObject(process, 0) == WAIT_OBJECT_0)
    {
        SendAction(1);
        Stop();
        return;
    }
    const bool inventory = g_pNewUISystem && (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_INVENTORY)
        || g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_INVENTORY_EXT));
    InterlockedExchange(control + 3, CanTalk() && (GetAsyncKeyState(PushToTalkKey) & 0x8000) ? 1 : 0);
    InterlockedExchange(control + 4, (GetForegroundWindow() == g_hWnd ? 1 : 0) | (inventory ? 2 : 0));
    InterlockedExchange(control + 2, static_cast<LONG>(GetTickCount()));
    const auto command = InterlockedExchange(control + 5, 0);
    if (command == 1 && !requestedAt)
    {
        InterlockedExchange(control + 7, 1);
        requestedAt = GetTickCount();
        SendAction(0);
    }
    else if (command == 2)
    {
        requestedAt = 0;
        SendAction(1);
        InterlockedIncrement(control + 6);
        SecureZeroMemory(const_cast<LONG*>(control) + 8, 65);
        InterlockedExchange(control + 7, 0);
        InterlockedIncrement(control + 6);
    }
}
#else
bool IsMicrophoneEnabled() { return false; }
void Toggle() {}
void Receive(const BYTE*, int) {}
void Update() {}
#endif
}

