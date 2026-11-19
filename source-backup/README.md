# Source checkpoint: vulkan

Snapshot of the local working sources on 2026-11-19, based on `4f2ad4f25e79464282a33d8431ef48208ce243fd`.

Clone this backup branch with `--recurse-submodules`. Apply any submodule patches as described below, then run `python tools/restore_source_backup.py` before building. Text changes are stored directly as normal Git files; the checked archive restores binary assets and source files in their original encoding. Newer files already present on the base branch are retained so no GitHub work is lost. See manifest.json for the complete overlay inventory and checksums.

For each submodule, initialize its pinned repository, check out patchBaseCommit from manifest.json, then apply its listed patch with `git apply` inside that submodule. The restore command also restores its untracked files.

Runtime game data, generated builds, private credentials, certificates, saved accounts and databases are outside this source backup. Use the project's pinned asset installers and published R2 client resources for game data. Do not treat this source checkpoint as a backup of production databases or signing keys.
