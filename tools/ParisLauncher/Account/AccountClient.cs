using System.Net;
using System.IO;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Net.Http.Json;

namespace ParisLauncher.Account;

public sealed class AccountClient : IDisposable
{
    private readonly HttpClient http;
    public AccountClient(string address)
    {
        var uri = new Uri(address.TrimEnd('/') + "/");
        if (uri.Scheme != "https" || !string.IsNullOrEmpty(uri.UserInfo)) throw new InvalidOperationException("La API de cuentas requiere HTTPS.");
        http = new HttpClient(new HttpClientHandler { AllowAutoRedirect = false }) { BaseAddress = uri, Timeout = TimeSpan.FromSeconds(20) };
    }

    public async Task Register(string name, string password, string email)
    {
        using var response = await http.PostAsJsonAsync("register", new { LoginName = name, Password = password, Email = email });
        await Check(response);
    }

    public async Task<LauncherSession> Login(string name, string password, bool remember)
    {
        using var response = await http.PostAsJsonAsync("login", new { LoginName = name, Password = password, Remember = remember });
        await Check(response);
        return await response.Content.ReadFromJsonAsync<LauncherSession>() ?? throw new IOException("Respuesta de sesión inválida.");
    }

    public async Task<bool> Validate(LauncherSession session)
    {
        if (session.ExpiresAt <= DateTimeOffset.UtcNow) return false;
        using var request = new HttpRequestMessage(HttpMethod.Get, "session");
        request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", session.Token);
        using var response = await http.SendAsync(request);
        return response.IsSuccessStatusCode;
    }

    public async Task<AccountProfile> Profile(LauncherSession session)
    {
        using var request=new HttpRequestMessage(HttpMethod.Get,"profile");request.Headers.Authorization=new AuthenticationHeaderValue("Bearer",session.Token);
        using var response=await http.SendAsync(request);await Check(response);
        return await response.Content.ReadFromJsonAsync<AccountProfile>()??throw new IOException("Perfil de cuenta inválido.");
    }

    public async Task Logout(LauncherSession session)
    {
        using var request = new HttpRequestMessage(HttpMethod.Post, "logout");
        request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", session.Token);
        using var response = await http.SendAsync(request);
        await Check(response);
    }

    private static async Task Check(HttpResponseMessage response)
    {
        if (response.IsSuccessStatusCode) return;
        string message = response.StatusCode switch
        {
            HttpStatusCode.Unauthorized => "Usuario o contraseña incorrectos, o cuenta no disponible.",
            HttpStatusCode.Conflict => "Ese usuario ya está registrado.",
            HttpStatusCode.TooManyRequests => "Demasiados intentos. Espera unos minutos antes de reintentar.",
            _ => "No se pudo completar la operación. Revisa tu conexión e inténtalo de nuevo.",
        };
        if (response.StatusCode == HttpStatusCode.BadRequest)
        {
            var detail = await response.Content.ReadFromJsonAsync<ApiError>();
            message = detail?.Message ?? "Revisa los datos ingresados.";
        }
        throw new InvalidOperationException(message);
    }

    private sealed record ApiError(string Message);
    public void Dispose() => http.Dispose();
}
