// <copyright file="TcpLatency.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.Client.Library;

using System;
using System.Net.Sockets;

/// <summary>Reads latency from an existing socket without sending additional packets.</summary>
internal static class TcpLatency
{
    /// <summary>Reads the TCP round trip time on supported platforms.</summary>
    /// <param name="socket">The existing connected socket.</param>
    /// <returns>Milliseconds, or -1 when unavailable.</returns>
    internal static int ReadMilliseconds(Socket? socket)
    {
        if (socket is null || !OperatingSystem.IsWindows())
        {
            return -1;
        }

        try
        {
            // Windows TCP_INFO_v0: SIO_TCP_INFO, RttUs at byte offset 20.
            const int tcpInfoControl = unchecked((int)0xD8000027);
            const int roundTripOffset = 20;
            var data = new byte[104];
            var count = socket.IOControl(tcpInfoControl, BitConverter.GetBytes(0u), data);
            if (count < roundTripOffset + sizeof(uint))
            {
                return -1;
            }

            var micros = BitConverter.ToUInt32(data, roundTripOffset);
            return micros == 0 ? -1 : (int)((micros + 999UL) / 1000UL);
        }
        catch (SocketException)
        {
            return -1;
        }
        catch (ObjectDisposedException)
        {
            return -1;
        }
        catch (PlatformNotSupportedException)
        {
            return -1;
        }
    }
}
