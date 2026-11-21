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

    public void Commit(IReadOnlyList<(UpdateFile File, string Staged)> files, Action<int, int, string>? report = null)
    {
        if (files.Count == 0) return;
        try
        {
            // Back up every original before publishing the complete recovery plan.
            // A crash can then restore both applied and not-yet-applied entries safely.
            var journal = PrepareJournal(files);
            WriteJournal(journal);
            for (int index = 0; index < files.Count; index++)
            {
                var (file, staged) = files[index];
                string target = UpdatePaths.Resolve(root, file.Path);
                File.Move(staged, target, overwrite: true);
                report?.Invoke(index + 1, files.Count, file.Path);
            }
            File.Delete(Journal);
        }
        catch
        {
            Recover();
            throw;
        }
    }

    private List<UpdateJournalEntry> PrepareJournal(IReadOnlyList<(UpdateFile File, string Staged)> files)
    {
        var journal = new List<UpdateJournalEntry>(files.Count);
        foreach (var (file, staged) in files)
        {
            if (!File.Exists(staged)) throw new FileNotFoundException("Falta el archivo preparado: " + file.Path, staged);
            string target = UpdatePaths.Resolve(root, file.Path);
            Directory.CreateDirectory(Path.GetDirectoryName(target)!);
            bool hadOriginal = File.Exists(target);
            string backup = Guid.NewGuid().ToString("N") + ".bak";
            if (hadOriginal) File.Copy(target, Path.Combine(Workspace, backup));
            journal.Add(new(file.Path, backup, hadOriginal));
        }
        return journal;
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
