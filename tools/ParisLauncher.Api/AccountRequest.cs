using System.ComponentModel.DataAnnotations;

namespace ParisLauncher.Api;

public sealed record AccountRequest(string LoginName, string Password, string? Email, bool Remember = false)
{
    public bool IsValid(bool registering) =>
        LoginName is { Length: >= 3 and <= 10 } && LoginName.All(c => c is >= '!' and <= '~') &&
        Password is { Length: >= 3 and <= 20 } && Password.All(c => c is >= ' ' and <= '~') &&
        (!registering || Email is { Length: > 3 and <= 254 } && new EmailAddressAttribute().IsValid(Email));
}
