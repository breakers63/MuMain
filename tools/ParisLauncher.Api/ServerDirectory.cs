using Npgsql;
using System.Net.Sockets;
namespace ParisLauncher.Api;
public static class ServerDirectory
{
    public sealed record Entry(int Id,string Name,bool Available,int[] Ports);
    public static async Task<IReadOnlyList<Entry>> Read(NpgsqlDataSource database,CancellationToken token)
    {
        var rows=new List<(int Id,string Name,int Port)>();
        await using(var command=database.CreateCommand("SELECT l.server_id,COALESCE(NULLIF(g.\"Description\",''),l.server_name),l.port FROM public.paris_launcher_servers l LEFT JOIN config.\"GameServerDefinition\" g ON g.\"ServerID\"=l.server_id ORDER BY l.server_id,l.port"))
        await using(var reader=await command.ExecuteReaderAsync(token))
            while(await reader.ReadAsync(token))rows.Add((reader.GetInt16(0),reader.GetString(1),reader.GetInt32(2)));
        var results=new List<Entry>();
        foreach(var group in rows.GroupBy(r=>r.Id))
        {
            bool available=false;
            foreach(var row in group)available |= await Reachable(row.Port,token);
            results.Add(new(group.Key,group.First().Name,available,group.Select(row=>row.Port).Distinct().ToArray()));
        }
        return results;
    }
    static async Task<bool> Reachable(int port,CancellationToken token)
    {
        using var client=new TcpClient();using var limit=CancellationTokenSource.CreateLinkedTokenSource(token);limit.CancelAfter(TimeSpan.FromSeconds(1));
        try{await client.ConnectAsync("paris-production-server",port,limit.Token);return true;}
        catch(SocketException){return false;}
        catch(OperationCanceledException) when(!token.IsCancellationRequested){return false;}
    }
}
