# ZIP recovery without duplicating the full client

The updated launcher always reads the current windows/manifest.json. It merges compatible immutable ZIPs from windows/manifest-zip.json only when the catalog file size and SHA256 match the latest manifest. Newer event/client releases win; stale catalog entries are discarded. Source-to-target entry maps reuse old client layouts without extracting superseded entries. Current target hashes, bounded extraction, path checks and symlink rejection remain mandatory. Legacy launchers continue using the normal manifest.

Validated against Windows 2026.11.20.2, including the concurrent OpenGL Events release:
- Fresh modern: 20737 files, 12 ZIPs, zero loose files.
- Fresh classic: 20262 files, 12 ZIPs, zero loose files.
- Fresh combined: 21515 files, 18 ZIPs, zero loose files.
- Updates from 2026.11.19.10: modern 3 files / 2 ZIPs, classic 50 files / 2 ZIPs, combined 53 files / 3 ZIPs; zero loose files in each.

All 16 reused catalog archives were fetched and SHA256-verified. A missing historical effects ZIP was restored with the same hash. Only a 3 MB complement and a 33 KB helper ZIP were newly assembled for coverage; no complete client archive was uploaded. The main voice delta remains about 16 MB. The new launcher is available as windows/releases/2026.11.20.2/Paris-Launcher-ZIP.zip.

Run: dotnet run --project tests/launcher-zip/Tests.csproj -- latest-manifest.json zip-catalog.json optional-previous-manifest.json
