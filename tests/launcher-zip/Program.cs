using ParisLauncher.Core;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text.Json;
var root=Path.Combine(Path.GetTempPath(),"paris-zip-test-"+Guid.NewGuid());Directory.CreateDirectory(root);
var workspace=Path.Combine(root,"stage");Directory.CreateDirectory(workspace);
var archive=Path.Combine(root,"base.zip");var data="verified-base"u8.ToArray();
using(var z=ZipFile.Open(archive,ZipArchiveMode.Create)){using(var s=z.CreateEntry("Data/base.bin").Open())s.Write(data);using(var s=z.CreateEntry("Launcher.exe").Open())s.Write("old"u8);}
var f=new UpdateFile{Path="Vulkan/Data/base.bin",Size=data.Length,Sha256=Convert.ToHexString(SHA256.HashData(data)).ToLowerInvariant()};
var b=new UpdateBundle{Url="base.zip",Size=new FileInfo(archive).Length,Sha256=Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(archive))).ToLowerInvariant(),Files=[f.Path],EntryMap=new(){{"Data/base.bin",f.Path}}};
var manifest=new UpdateManifest{Files=[f],Bundles=[b]};BundleUpdates.Validate(manifest,root);
var extracted=await BundleUpdates.Extract(archive,b,manifest.Files,new HashSet<string>{f.Path},workspace,root,new Progress<UpdateProgress>(),default);
if(extracted.Count!=1||!File.ReadAllBytes(extracted[0].Staged).SequenceEqual(data))throw new Exception("Mapped base extraction failed");
f.Sha256=new string('0',64);try{await BundleUpdates.Extract(archive,b,manifest.Files,new HashSet<string>{f.Path},workspace,root,new Progress<UpdateProgress>(),default);throw new Exception("Bad hash accepted");}catch(InvalidDataException){}
b.EntryMap=new(){{"../escape",f.Path}};try{BundleUpdates.Validate(manifest,root);throw new Exception("Traversal accepted");}catch(InvalidDataException){}
Console.WriteLine("PASS: mapped base ZIP, ignored superseded entry, target checksum, traversal rejection.");
if(args.Length>0){var live=JsonSerializer.Deserialize<UpdateManifest>(File.ReadAllText(args[0]))!;BundleUpdates.Validate(live,root);foreach(var client in new[]{"modern","classic","both"}){var files=live.Files.Where(f=>client=="both"||f.Client=="common"||f.Client==(client=="classic"?"classic":"modern")).ToList();var selected=BundleUpdates.Select(live.Bundles,files);var covered=selected.SelectMany(b=>b.Files).ToHashSet();var missing=files.Where(f=>!covered.Contains(f.Path)).ToList();Console.WriteLine($"{client}: {files.Count} files / {selected.Count} ZIP / {missing.Count} loose");if(missing.Count!=0)throw new Exception("Loose files remain");}}
