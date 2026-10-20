"""Copy PARIS map resources and music from an OpenGL client's Data directory.

Usage: python tools/sync_paris_maps.py --source <OpenGL/Data> --apply
Without --apply, only report differences. Existing changed files are backed up.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil


ROOT = Path(__file__).resolve().parents[1]
MAP_FOLDER = re.compile(r"(?:world|object)\d+", re.IGNORECASE)
RESOURCE_EXTENSIONS = {
    ".att", ".map", ".obj", ".cws", ".bmd", ".ozb", ".ozj", ".ozt",
    ".tga", ".jpg", ".png", ".bmp", ".smd", ".ini", ".txt",
    ".mp3", ".ogg", ".wav",
}


def checksum(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def resources(data):
    folders = sorted(p for p in data.iterdir() if p.is_dir()
                     and (MAP_FOLDER.fullmatch(p.name) or p.name.lower() == "music"))
    for folder in folders:
        for path in sorted(folder.rglob("*")):
            if path.is_file() and path.suffix.lower() in RESOURCE_EXTENSIONS:
                yield path


def sync_resource(source, target, backup, digest):
    if target.exists() and checksum(target) == digest:
        return "same"
    status = "changed" if target.exists() else "added"
    if status == "changed" and not backup.exists():
        backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(target, backup)
    target.parent.mkdir(parents=True, exist_ok=True)
    # New timestamps let the incremental build notice imported older resources.
    shutil.copyfile(source, target)
    if checksum(target) != digest:
        raise OSError(f"Content verification failed: {target}")
    return status


def synchronize(source, target, backup, apply):
    files = []
    counts = {"same": 0, "changed": 0, "added": 0}
    for source_file in resources(source):
        relative = source_file.relative_to(source)
        digest = checksum(source_file)
        target_file = target / relative
        if apply:
            status = sync_resource(source_file, target_file, backup / relative, digest)
        elif not target_file.exists():
            status = "added"
        else:
            status = "same" if checksum(target_file) == digest else "changed"
        counts[status] += 1
        files.append({"file": relative.as_posix(), "size": source_file.stat().st_size,
                      "sha256": digest, "status": status})
    return {"schema": 1, "source": "PARIS OpenGL client", "counts": counts, "files": files}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--target", type=Path, default=ROOT / "src/bin/Data")
    parser.add_argument("--backup", type=Path, default=ROOT / "out/paris-gold/map-backup/Data")
    parser.add_argument("--report", type=Path, default=ROOT / "out/paris-gold/map-manifest.json")
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    source, target = args.source.resolve(strict=True), args.target.resolve()
    if source == target or source in target.parents or target in source.parents:
        parser.error("Source and target must be separate Data directories")
    report = synchronize(source, target, args.backup.resolve(), args.apply)
    if not report["files"]:
        parser.error("No map resources found in the source Data directory")
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"applied": args.apply, **report["counts"],
                      "files": len(report["files"]),
                      "bytes": sum(item["size"] for item in report["files"])}, ensure_ascii=False))


if __name__ == "__main__":
    main()
