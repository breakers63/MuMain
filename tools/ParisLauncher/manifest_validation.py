"""Reject invalid update metadata and Windows destination collisions before publication."""
import string


def validate_manifest(manifest):
    files = manifest.get("Files")
    if not str(manifest.get("Version", "")).strip() or not isinstance(files, list) or len(files) > 100000:
        raise ValueError("Invalid manifest version or file list")
    destinations = {}
    for entry in files:
        path = entry.get("Path", "")
        size = entry.get("Size", -1)
        digest = entry.get("Sha256", "")
        client = entry.get("Client")
        aliases = entry.get("ClassicAliases", [])
        if not path or not isinstance(size, int) or not 0 <= size <= 32 * 1024**3 or len(digest) != 64 or any(c not in string.hexdigits for c in digest) or client not in {"common", "modern", "classic"}:
            raise ValueError("Invalid update entry: " + path)
        if not isinstance(aliases, list) or len(aliases) > 1 or (aliases and client != "common"):
            raise ValueError("Invalid shared resource: " + path)
        for destination in [path, *aliases]:
            key = destination.replace("\\", "/").casefold()
            if key in destinations:
                raise ValueError("Duplicate Windows destination: " + destination + " / " + destinations[key])
            destinations[key] = destination
