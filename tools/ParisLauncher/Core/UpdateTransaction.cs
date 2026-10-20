using System.IO;
using System.Text.Json;

namespace ParisLauncher.Core;

public sealed class UpdateTransaction(string root)
{
    private string Workspace => Path.Combine(root, ".launcher-update");
    private string Journal => Path.Combine(Workspace, "journal.json");

    public void Recover()
    {
        UpdatePaths.RejectLinks(root, Workspace);
        if (!File.Exists(Journal)) return;
        var entries = JsonSerializer.Deserialize<List<UpdateJournalEntry>>(File.ReadAllText(Journal))
            ?? throw new InvalidDataException("No se puede leer la recuperación de la actualización.");
        foreach (var entry in entries.AsEnumerable().Reverse())
        {
            string target = UpdatePaths.Resolve(root, entry.Path);
            string backup = Path.Combine(Workspace, Path.GetFileName(entry.Backup));
            if (entry.HadOriginal)
            {
                if (!File.Exists(backup)) throw new IOException("Falta la copia de recuperación: " + entry.Path);
                File.Copy(backup, target, overwrite: true);
            }
            else if (File.Exists(target)) File.Delete(target);
        }
        File.Delete(Journal);
    }

    public void Commit(IReadOnlyList<(UpdateFile File, string Staged)> files)
    {
        var journal = new List<UpdateJournalEntry>();
        try
        {
            foreach (var (file, staged) in files)
            {
                string target = UpdatePaths.Resolve(root, file.Path);
                Directory.CreateDirectory(Path.GetDirectoryName(target)!);
                bool hadOriginal = File.Exists(target);
                string backup = Guid.NewGuid().ToString("N") + ".bak";
                if (hadOriginal) File.Copy(target, Path.Combine(Workspace, backup));
                journal.Add(new(file.Path, backup, hadOriginal));
                WriteJournal(journal);
                File.Move(staged, target, overwrite: true);
            }
            if (File.Exists(Journal)) File.Delete(Journal);
        }
        catch
        {
            Recover();
            throw;
        }
    }

    public string Prepare()
    {
        Recover();
        if (Directory.Exists(Workspace)) Directory.Delete(Workspace, recursive: true);
        Directory.CreateDirectory(Workspace);
        return Workspace;
    }

    private void WriteJournal(List<UpdateJournalEntry> entries)
    {
        string temporary = Journal + ".tmp";
        using (var stream = new FileStream(temporary, FileMode.Create, FileAccess.Write, FileShare.None, 4096, FileOptions.WriteThrough))
        {
            JsonSerializer.Serialize(stream, entries);
            stream.Flush(flushToDisk: true);
        }
        File.Move(temporary, Journal, overwrite: true);
    }
}
