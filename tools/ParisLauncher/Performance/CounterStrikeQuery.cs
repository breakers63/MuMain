using System.Diagnostics;
using System.Net.Sockets;
using System.Text;
namespace ParisLauncher.Performance;
public static class CounterStrikeQuery
{
    public sealed record Result(string Name,double Milliseconds);
    public static async Task<Result?> Read(string host,int port,CancellationToken token)
    {
        using var limit=CancellationTokenSource.CreateLinkedTokenSource(token);limit.CancelAfter(TimeSpan.FromSeconds(2));
        using var udp=new UdpClient();udp.Connect(host,port);
        byte[] query=[255,255,255,255,84,..Encoding.ASCII.GetBytes("Source Engine Query\0")];
        var watch=Stopwatch.StartNew();
        try
        {
            await udp.SendAsync(query,limit.Token);var data=(await udp.ReceiveAsync(limit.Token)).Buffer;
            if(data.Length>=9 && data[4]==0x41)
            {
                query=[..query,..data.AsSpan(5,4).ToArray()];await udp.SendAsync(query,limit.Token);data=(await udp.ReceiveAsync(limit.Token)).Buffer;
            }
            if(data.Length<10 || !data.AsSpan(0,4).SequenceEqual(new byte[]{255,255,255,255}))return null;
            int offset=data[4] switch{0x49=>6,0x6d=>5,_=>-1};if(offset<0)return null;
            if(data[4]==0x6d){int addressEnd=Array.IndexOf(data,(byte)0,offset);if(addressEnd<0)return null;offset=addressEnd+1;}
            int end=Array.IndexOf(data,(byte)0,offset);if(end<=offset)return null;
            string name=Encoding.UTF8.GetString(data,offset,end-offset);
            return new(name,watch.Elapsed.TotalMilliseconds);
        }
        catch(SocketException){return null;}
        catch(OperationCanceledException) when(!token.IsCancellationRequested){return null;}
    }
}
