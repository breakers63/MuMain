#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "EventDeviceProtocol.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzCharacter.h"
#include <array>
#include <wincrypt.h>

namespace Network::Paris::EventDevice
{
    namespace
    {
        constexpr DWORD ProbeInterval = 5000;
        constexpr size_t DigestLength = 32;
        constexpr BYTE DeviceSubCode = 0xD4;

        struct HashHandles
        {
            HCRYPTPROV Provider = 0;
            HCRYPTHASH Hash = 0;
            ~HashHandles()
            {
                if (Hash) CryptDestroyHash(Hash);
                if (Provider) CryptReleaseContext(Provider, 0);
            }
        };

        std::array<BYTE, DigestLength> DeviceDigest()
        {
            std::array<BYTE, DigestLength> digest{};
            wchar_t machineGuid[128]{};
            DWORD size = sizeof(machineGuid);
            if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid",
                RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, nullptr, machineGuid, &size) != ERROR_SUCCESS
                || size <= sizeof(wchar_t) || size > sizeof(machineGuid)) return digest;
            HashHandles handles;
            if (!CryptAcquireContextW(&handles.Provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)
                || !CryptCreateHash(handles.Provider, CALG_SHA_256, 0, 0, &handles.Hash)
                || !CryptHashData(handles.Hash, reinterpret_cast<const BYTE*>(machineGuid), size - sizeof(wchar_t), 0)) return digest;
            DWORD digestSize = static_cast<DWORD>(digest.size());
            if (!CryptGetHashParam(handles.Hash, HP_HASHVAL, digest.data(), &digestSize, 0) || digestSize != digest.size()) digest.fill(0);
            return digest;
        }
    }

    void Probe()
    {
        if (SceneFlag != MAIN_SCENE || !Hero || !Hero->ID[0] || !SocketClient || !SocketClient->IsConnected()) return;
        static const auto digest = DeviceDigest(); // Registry/crypto are read once, never on each frame.
        static DWORD lastProbe = 0;
        const DWORD now = timeGetTime();
        if (lastProbe && now - lastProbe < ProbeInterval) return;
        lastProbe = now;
        BYTE packet[5 + DigestLength] = {0xC1, static_cast<BYTE>(5 + DigestLength), 0xFA, DeviceSubCode, 1};
        std::copy(digest.begin(), digest.end(), packet + 5);
        SocketClient->Send(packet, sizeof(packet));
    }
}
