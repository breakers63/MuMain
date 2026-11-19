using System.Security.Cryptography;
using Npgsql;

namespace ParisLauncher.Api;

public sealed class AccountStore(NpgsqlDataSource database)
{
    private readonly string dummyHash = BCrypt.Net.BCrypt.HashPassword("unregistered-account", 12);

    public async Task<(Guid Id, string Name)?> Verify(AccountRequest request, CancellationToken cancellation)
    {
        await using var command = database.CreateCommand("SELECT \"Id\",\"LoginName\",\"PasswordHash\",\"State\",\"IsTemplate\",\"IsBot\" FROM data.\"Account\" WHERE lower(\"LoginName\")=lower($1) LIMIT 2");
        command.Parameters.AddWithValue(request.LoginName);
        await using var reader = await command.ExecuteReaderAsync(cancellation);
        if (!await reader.ReadAsync(cancellation)) { BCrypt.Net.BCrypt.Verify(request.Password, dummyHash); return null; }
        var id = reader.GetGuid(0);
        var name = reader.GetString(1);
        var hash = reader.GetString(2);
        bool allowed = AccountAccess.CanLogin(reader.GetInt32(3)) && !reader.GetBoolean(4) && !reader.GetBoolean(5);
        if (await reader.ReadAsync(cancellation)) return null;
        try { return BCrypt.Net.BCrypt.Verify(request.Password, hash) && allowed ? (id, name) : null; }
        catch (BCrypt.Net.SaltParseException) { return null; }
    }

    public async Task<bool> Register(AccountRequest request, CancellationToken cancellation)
    {
        await using var connection = await database.OpenConnectionAsync(cancellation);
        await using var transaction = await connection.BeginTransactionAsync(cancellation);
        await using var guard = new NpgsqlCommand("SELECT pg_advisory_xact_lock(hashtext(lower($1)))", connection, transaction);
        guard.Parameters.AddWithValue(request.LoginName);
        await guard.ExecuteNonQueryAsync(cancellation);
        await using var check = new NpgsqlCommand("SELECT count(*) FROM data.\"Account\" WHERE lower(\"LoginName\")=lower($1)", connection, transaction);
        check.Parameters.AddWithValue(request.LoginName);
        if ((long)(await check.ExecuteScalarAsync(cancellation))! != 0) return false;
        await using var insert = new NpgsqlCommand("INSERT INTO data.\"Account\" (\"Id\",\"LoginName\",\"PasswordHash\",\"SecurityCode\",\"EMail\",\"RegistrationDate\",\"State\",\"TimeZone\",\"VaultPassword\",\"IsVaultExtended\",\"LanguageIsoCode\") VALUES ($1,$2,$3,'',$4,now(),0,0,'',false,'es')", connection, transaction);
        insert.Parameters.AddWithValue(Guid.NewGuid());
        insert.Parameters.AddWithValue(request.LoginName);
        insert.Parameters.AddWithValue(BCrypt.Net.BCrypt.HashPassword(request.Password, 12));
        insert.Parameters.AddWithValue(request.Email!);
        await insert.ExecuteNonQueryAsync(cancellation);
        await transaction.CommitAsync(cancellation);
        return true;
    }

    public async Task<object> CreateSession(Guid id, string name, bool remember, CancellationToken cancellation)
    {
        string token = Convert.ToHexString(RandomNumberGenerator.GetBytes(32));
        var expires = DateTimeOffset.UtcNow.AddHours(remember ? 168 : 12);
        await using var cleanup = database.CreateCommand("DELETE FROM public.paris_launcher_sessions WHERE expires_at<now()");
        await cleanup.ExecuteNonQueryAsync(cancellation);
        await using var command = database.CreateCommand("INSERT INTO public.paris_launcher_sessions(token_hash,account_id,expires_at) VALUES ($1,$2,$3)");
        command.Parameters.AddWithValue(Hash(token)); command.Parameters.AddWithValue(id); command.Parameters.AddWithValue(expires);
        await command.ExecuteNonQueryAsync(cancellation);
        return new { Token = token, LoginName = name, ExpiresAt = expires };
    }

    public async Task<object?> Session(string token, CancellationToken cancellation)
    {
        if (token.Length != 64 || !token.All(Uri.IsHexDigit)) return null;
        await using var command = database.CreateCommand("SELECT a.\"LoginName\",s.expires_at FROM public.paris_launcher_sessions s JOIN data.\"Account\" a ON a.\"Id\"=s.account_id WHERE s.token_hash=$1 AND s.expires_at>now() AND a.\"State\">=0 AND a.\"State\"<$2 AND NOT a.\"IsTemplate\" AND NOT a.\"IsBot\"");
        command.Parameters.AddWithValue(Hash(token));
        command.Parameters.AddWithValue(AccountAccess.FirstBlockedState);
        await using var reader = await command.ExecuteReaderAsync(cancellation);
        return await reader.ReadAsync(cancellation) ? new { LoginName = reader.GetString(0), ExpiresAt = reader.GetDateTime(1) } : null;
    }

    public async Task<AccountProfile?> Profile(string token,CancellationToken cancellation)
    {
        if(token.Length!=64||!token.All(Uri.IsHexDigit))return null;
        await using var command=database.CreateCommand("SELECT p.login_name,p.email,p.vip_tier,p.vip_expires,p.legacy_vip FROM public.paris_launcher_sessions s JOIN public.paris_launcher_profile p ON p.account_id=s.account_id JOIN data.\"Account\" a ON a.\"Id\"=s.account_id WHERE s.token_hash=$1 AND s.expires_at>now() AND a.\"State\">=0 AND a.\"State\"<$2 AND NOT a.\"IsTemplate\" AND NOT a.\"IsBot\"");
        command.Parameters.AddWithValue(Hash(token));command.Parameters.AddWithValue(AccountAccess.FirstBlockedState);
        await using var reader=await command.ExecuteReaderAsync(cancellation);
        return await reader.ReadAsync(cancellation)?AccountProfile.From(reader.GetString(0),reader.IsDBNull(1)?"":reader.GetString(1),reader.IsDBNull(2)?null:reader.GetString(2),reader.IsDBNull(3)?null:reader.GetString(3),reader.GetFloat(4)):null;
    }

    public async Task Revoke(string token, CancellationToken cancellation)
    {
        await using var command = database.CreateCommand("DELETE FROM public.paris_launcher_sessions WHERE token_hash=$1");
        command.Parameters.AddWithValue(Hash(token));
        await command.ExecuteNonQueryAsync(cancellation);
    }

    private static byte[] Hash(string token) => SHA256.HashData(System.Text.Encoding.UTF8.GetBytes(token));
}
