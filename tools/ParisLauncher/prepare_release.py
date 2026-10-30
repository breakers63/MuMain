"""Build a portable, deduplicated Windows release. Never packages player credentials."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
from urllib.parse import quote
from manifest_validation import validate_manifest


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def link_or_copy(source, destination):
    import shutil
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        os.link(source, destination)
    except OSError:
        shutil.copyfile(source, destination)


def clean_config(source, destination, modern):
    config = configparser.ConfigParser(strict=False)
    config.optionxform = str
    config.read(source, encoding="utf-8-sig")
    for section in list(config.sections()):
        if section.lower() in {"login", "chatcommands"} or section.lower().startswith("savedaccount"):
            config.remove_section(section)
    config["LOGIN"] = {"RememberMe": "0", "SavePassword": "0", "Language": "Eng", "EncryptedUsername": "", "EncryptedPassword": ""}
    if modern:
        if not config.has_section("Render"):
            config.add_section("Render")
        config["Render"]["Backend"] = "vulkan"
    with destination.open("w", encoding="utf-8") as stream:
        config.write(stream)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modern", type=Path, required=True)
    parser.add_argument("--classic", type=Path, required=True)
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--crt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--version", required=True)
    args = parser.parse_args()
    root = args.output.resolve()
    if root.exists():
        raise SystemExit("Use a new output directory; existing releases are never overwritten.")
    root.mkdir(parents=True)
    entries = []
    shared_bytes = 0

    def add(source, target, client, aliases=None):
        destination = root / target
        link_or_copy(source, destination)
        item = {"Path": target, "Size": source.stat().st_size, "Sha256": digest(source), "Client": client, "Url": quote(target, safe="/")}
        if aliases:
            item["ClassicAliases"] = aliases
            for alias in aliases:
                link_or_copy(destination, root / alias)
        entries.append(item)
        return item["Size"]

    classic_data = {str(p.relative_to(args.classic / "Data")).replace("\\", "/").lower(): p for p in (args.classic / "Data").rglob("*") if p.is_file()}
    for source in (args.modern / "Data").rglob("*"):
        if not source.is_file():
            continue
        relative = source.relative_to(args.modern).as_posix()
        other = classic_data.pop(source.relative_to(args.modern / "Data").as_posix().lower(), None)
        if other and other.stat().st_size == source.stat().st_size and digest(other) == digest(source):
            alias = "Classic/" + other.relative_to(args.classic).as_posix()
            shared_bytes += add(source, relative, "common", [alias])
        else:
            add(source, relative, "modern")
            if other:
                add(other, "Classic/" + other.relative_to(args.classic).as_posix(), "classic")
    for source in classic_data.values():
        add(source, "Classic/" + source.relative_to(args.classic).as_posix(), "classic")
    for folder in ["fonts", "shaders"]:
        for source in (args.modern / folder).rglob("*"):
            if source.is_file():
                add(source, source.relative_to(args.modern).as_posix(), "modern")

    def runtime_files(source_root, prefix, client):
        for source in source_root.iterdir():
            if not source.is_file():
                continue
            if source.suffix.lower() == ".dll" or source.name in {"Main.exe", "Jugar.exe", "MuMainIcon.png"}:
                add(source, prefix + source.name, client)

    runtime_files(args.modern, "", "modern")
    runtime_files(args.classic, "Classic/", "classic")
    for arch, prefix, client in [("x64", "", "modern"), ("x86", "Classic/", "classic")]:
        for source in (args.crt / arch / "Microsoft.VC145.CRT").glob("*.dll"):
            add(source, prefix + source.name, client)
    for source_root, prefix, client in [(args.modern, "", "modern"), (args.classic, "Classic/", "classic")]:
        destination = root / (prefix + "config.ini.template")
        destination.parent.mkdir(parents=True, exist_ok=True)
        clean_config(source_root / "config.ini", destination, client == "modern")
        entries.append({"Path": prefix + "config.ini.template", "Size": destination.stat().st_size, "Sha256": digest(destination), "Client": client})
        for name in ["boost.ini", "Performance.ini"]:
            if (source_root / name).exists():
                add(source_root / name, prefix + name + ".template", client)

    manifest = {"Version": args.version, "Files": entries}
    validate_manifest(manifest)
    (root / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, separators=(",", ":")), encoding="utf-8")
    link_or_copy(args.launcher, root / "Launcher.exe")
    settings = {"ModernExecutable": "Jugar.exe", "ClassicExecutable": "Classic/Jugar.exe", "ManifestUrl": "https://patch.hispanogaming.com/windows/manifest.json"}
    (root / "launcher.json").write_text(json.dumps(settings, indent=2), encoding="utf-8")
    (root / "release-summary.json").write_text(json.dumps({"version": args.version, "files": len(entries), "sharedBytes": shared_bytes, "downloadBytes": sum(e["Size"] for e in entries)}, indent=2))
    print((root / "release-summary.json").read_text())


if __name__ == "__main__":
    main()
