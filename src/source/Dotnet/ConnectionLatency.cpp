#include "stdafx.h"
#include "ConnectionLatency.h"
#include "Connection.h"
namespace DotNetBridge
{
    int TcpPingMilliseconds(const Connection* connection)
    {
        if (!connection || connection->GetHandle() <= 0) return -1;
        using ReadPing = int32_t(CORECLR_DELEGATE_CALLTYPE*)(int32_t);
        static const auto read = munique_client_library_handle ? reinterpret_cast<ReadPing>(
            mu::platform::GetSymbol(munique_client_library_handle, "ConnectionManager_GetTcpPing")) : nullptr;
        return read ? read(connection->GetHandle()) : -1;
    }
}
