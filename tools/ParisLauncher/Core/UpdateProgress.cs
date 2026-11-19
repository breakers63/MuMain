namespace ParisLauncher.Core;

public sealed record UpdateProgress(string Message, double Percent, bool Indeterminate = false);
