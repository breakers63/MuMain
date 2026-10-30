"""Run on the VPS with its existing rclone configuration; publish the manifest last."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from manifest_validation import validate_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", type=Path, help="Prepared release folder")
    parser.add_argument("--remote", default="r2:parisproject/windows/")
    args = parser.parse_args()
    root = args.folder.resolve()
    manifest = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
    validate_manifest(manifest)
    for entry in manifest["Files"]:
        path = (root / entry["Path"]).resolve()
        if not path.is_relative_to(root) or path.stat().st_size != entry["Size"]:
            raise RuntimeError("Invalid path or size: " + entry["Path"])
        with path.open("rb") as stream:
            if hashlib.file_digest(stream, "sha256").hexdigest().lower() != entry["Sha256"].lower():
                raise RuntimeError("Invalid hash: " + entry["Path"])
    # Upload only entries declared in the manifest, never player configuration or logs.
    files = root / ".publish-files.txt"
    files.write_text("\n".join(entry["Path"] for entry in manifest["Files"]) + "\n", encoding="utf-8")
    remote = args.remote.rstrip("/") + "/"
    subprocess.run(["rclone", "copy", str(root), remote, "--files-from-raw", str(files), "--transfers", "16", "--checkers", "16"], check=True)
    subprocess.run(["rclone", "copyto", str(root / "Launcher.exe"), remote + "Launcher.exe", "--metadata", "--metadata-set", "cache-control=no-cache", "--metadata-set", "content-type=application/octet-stream"], check=True)
    subprocess.run(["rclone", "copyto", str(root / "manifest.json"), remote + "manifest.json", "--metadata", "--metadata-set", "cache-control=no-cache", "--metadata-set", "content-type=application/json"], check=True)
    files.unlink()
    print("Published " + manifest["Version"])


if __name__ == "__main__":
    main()
