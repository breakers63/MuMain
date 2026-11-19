# Launcher ZIP updates



Launcher 1.0.8 supports optional `Bundles` alongside the existing `Files` manifest. Older launchers ignore bundles and retain individual downloads. Each bundle declares `Url`, `Size`, `Sha256` and the canonical `Files` paths it contains. ZIP entries must match the current manifest sizes and hashes. Do not reuse an older full ZIP after changing its contents or manifest file hashes.



A fresh installation prefers one complete ZIP when its compressed size is suitable. Smaller updates select a patch ZIP covering the changed files. Remaining files use bounded individual downloads. Both paths stage verified files before committing; Classic aliases retain the existing linking behavior. Bundles cannot replace configuration names protected by `UpdatePaths`.



Interrupted ZIP data is kept under `.launcher-downloads` using its expected SHA-256 as the filename. The next attempt verifies or resumes it with HTTP Range. Each extracted file is independently verified. A successful commit removes the cached archive. A server that ignores Range causes a safe restart instead of appending incompatible data.



Publish immutable ZIP objects first, verify their complete hashes and test public 206 responses. Publish the updated launcher at both existing download keys, then activate the manifest last. Keep the prior executable and manifest for recovery. The current public ZIP streaming worker has routes for the two 2026.11.01.3 archive URLs; later releases must add their archive URLs to that delivery configuration before activating a manifest.



Validation: LauncherChecks covers concurrent client starts, bounded downloads, interrupted ZIP resume, invalid archive paths and extracted-file hash mismatch. `--install-release <release directory> <new destination>` also supports ZIP-only release directories and checks all installed files and Classic aliases.

