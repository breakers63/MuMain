// <copyright file="ConnectionManager.Telemetry.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.Client.Library;

using System.Runtime.InteropServices;

/// <summary>Exposes connection latency to the native HUD.</summary>
public unsafe partial class ConnectionManager
{
    /// <summary>Reads the measured latency for an existing connection.</summary>
    /// <param name="handle">The connection handle.</param>
    /// <returns>Milliseconds, or -1 when unavailable.</returns>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_GetTcpPing")]
    public static int GetTcpPing(int handle) => Connections.TryGetValue(handle, out var connection) ? connection.GetTcpPingMilliseconds() : -1;
}
