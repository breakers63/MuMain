// <copyright file="SocketLatency.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.Client.Library;

using System;
using System.Net.Sockets;

/// <summary>Reads latency from an existing Windows TCP connection without sending probes.</summary>
internal static class SocketLatency
{
    private const int TcpInfoControl = unchecked((int)0xD8000027);
    private const int TcpInfoBytes = 104;
    private const int RoundTripMicrosecondsOffset = 20;
    private const int MinimumInfoBytes = RoundTripMicrosecondsOffset + sizeof(uint);

    internal static int Read(Socket? socket)
    {
        if (!OperatingSystem.IsWindows() || socket is null)
        {
            return -1;
        }

        try
        {
            var info = new byte[TcpInfoBytes];
            var count = socket.IOControl(TcpInfoControl, BitConverter.GetBytes(0u), info);
            if (count < MinimumInfoBytes)
            {
                return -1;
            }

            var microseconds = BitConverter.ToUInt32(info, RoundTripMicrosecondsOffset);
            return microseconds == 0 ? -1 : (int)((microseconds + 999UL) / 1000UL);
        }
        catch (Exception exception) when (exception is SocketException or ObjectDisposedException or PlatformNotSupportedException)
        {
            return -1;
        }
    }
}
