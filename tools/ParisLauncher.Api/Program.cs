using Microsoft.Extensions.Caching.Memory;
using System.Threading.RateLimiting;
using Microsoft.AspNetCore.RateLimiting;
using Npgsql;
using ParisLauncher.Api;

var builder = WebApplication.CreateBuilder(args);
builder.WebHost.ConfigureKestrel(options => options.Limits.MaxRequestBodySize = 4096);
builder.Services.AddSingleton(NpgsqlDataSource.Create(builder.Configuration.GetConnectionString("Accounts") ?? throw new InvalidOperationException("Missing accounts connection")));
builder.Services.AddSingleton<AccountStore>();
builder.Services.AddMemoryCache();
builder.Services.AddRateLimiter(options =>
{
    options.RejectionStatusCode = 429;
    options.AddPolicy("login", context => RateLimitPartition.GetFixedWindowLimiter(ClientKey(context), _ => new() { PermitLimit = 12, Window = TimeSpan.FromMinutes(1), QueueLimit = 0 }));
    options.AddPolicy("register", context => RateLimitPartition.GetFixedWindowLimiter(ClientKey(context), _ => new() { PermitLimit = 10, Window = TimeSpan.FromMinutes(15), QueueLimit = 0 }));
    options.GlobalLimiter = PartitionedRateLimiter.Create<HttpContext, string>(_ => RateLimitPartition.GetConcurrencyLimiter("all", _ => new() { PermitLimit = 4, QueueLimit = 0 }));
});
var app = builder.Build();
app.UseExceptionHandler(handler => handler.Run(async context => { context.Response.StatusCode = 500; await context.Response.WriteAsJsonAsync(new { Message = "No se pudo completar la operación." }); }));
app.Use(async (context, next) => { context.Response.Headers.CacheControl = "no-store"; await next(); });
app.UseRateLimiter();
app.MapGet("/servers", async (NpgsqlDataSource database, IMemoryCache cache, CancellationToken token) => Results.Ok(await cache.GetOrCreateAsync("server-directory", async entry => { entry.AbsoluteExpirationRelativeToNow = TimeSpan.FromSeconds(30); return await ServerDirectory.Read(database,token); })));
app.MapGet("/health", () => Results.Ok(new { Status = "ok" }));
app.MapPost("/register", async (AccountRequest request, AccountStore accounts, CancellationToken cancellation) =>
{
    if (!request.IsValid(true)) return Results.BadRequest(new { Message = "Usuario: 3–10 caracteres. Contraseña: 3–20. Indica un correo válido." });
    return await accounts.Register(request, cancellation) ? Results.StatusCode(201) : Results.Conflict(new { Message = "Ese usuario ya está registrado." });
}).RequireRateLimiting("register");
app.MapPost("/login", async (AccountRequest request, AccountStore accounts, CancellationToken cancellation) =>
{
    if (!request.IsValid(false)) return Results.Unauthorized();
    var account = await accounts.Verify(request, cancellation);
    return account is { } value ? Results.Ok(await accounts.CreateSession(value.Id, value.Name, request.Remember, cancellation)) : Results.Unauthorized();
}).RequireRateLimiting("login");
app.MapGet("/session", async (HttpContext context, AccountStore accounts, CancellationToken cancellation) =>
    await accounts.Session(Token(context), cancellation) is { } session ? Results.Ok(session) : Results.Unauthorized()).RequireRateLimiting("login");
app.MapGet("/profile", async (HttpContext context,AccountStore accounts,CancellationToken cancellation) => await accounts.Profile(Token(context),cancellation) is { } profile ? Results.Ok(profile) : Results.Unauthorized()).RequireRateLimiting("login");
app.MapPost("/logout", async (HttpContext context, AccountStore accounts, CancellationToken cancellation) =>
{ await accounts.Revoke(Token(context), cancellation); return Results.NoContent(); }).RequireRateLimiting("login");
app.Run();

static string ClientKey(HttpContext context) => context.Request.Headers["X-Paris-Client-IP"].FirstOrDefault() ?? context.Connection.RemoteIpAddress?.ToString() ?? "unknown";
static string Token(HttpContext context) => context.Request.Headers.Authorization.ToString() is var value && value.StartsWith("Bearer ", StringComparison.Ordinal) ? value[7..] : "";
