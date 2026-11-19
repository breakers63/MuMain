"""Install the verified Paris dungeon resources before building the native client."""
from pathlib import Path, PurePosixPath
import argparse, hashlib, json, os, shutil, tempfile, urllib.request, zipfile

repository = Path(__file__).resolve().parents[1]
manifest = json.loads((repository / 'docs/dungeons_assets.json').read_text(encoding='utf-8'))
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--destination', type=Path, default=repository / 'src/bin')
args = parser.parse_args()
base = args.destination.resolve()
base.mkdir(parents=True, exist_ok=True)
if not manifest['Url'].startswith('https://patch.hispanogaming.com/windows/releases/'):
    raise ValueError('Unexpected resource origin')
with tempfile.TemporaryFile() as download:
    digest, size = hashlib.sha256(), 0
    request = urllib.request.Request(manifest['Url'], headers={'User-Agent': 'MUParis-AssetInstaller/1.0'})
    with urllib.request.urlopen(request, timeout=120) as response:
        while block := response.read(1024 * 1024):
            size += len(block)
            if size > manifest['Bytes']:
                raise ValueError('Resource ZIP exceeds declared size')
            digest.update(block)
            download.write(block)
    if size != manifest['Bytes'] or digest.hexdigest() != manifest['Sha256']:
        raise ValueError('Resource ZIP checksum mismatch')
    download.seek(0)
    with zipfile.ZipFile(download) as archive:
        planned = []
        for entry in archive.infolist():
            if entry.is_dir():
                continue
            member = PurePosixPath(entry.filename)
            if member.is_absolute() or '..' in member.parts or '\\' in entry.filename or ':' in entry.filename:
                raise ValueError('Invalid archive path')
            if (entry.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError('Archive link rejected')
            if not member.parts:
                continue
            direct = member.parts[0] == manifest['ArchivePrefix']
            shared = (member.parts[0] == manifest.get('SharedArchivePrefix')
                      and len(member.parts) > 1 and member.parts[1] == 'Data')
            if not direct and not shared:
                continue
            relative = PurePosixPath(*member.parts[1:])
            if not relative.parts or relative.parts[0] not in ('Data', 'fonts', 'shaders'):
                continue
            target = base / Path(*relative.parts)
            if not target.resolve().is_relative_to(base) or entry.file_size > 128 * 1024 * 1024:
                raise ValueError('Invalid resource member')
            planned.append((entry, target))
        for entry, target in planned:
            target.parent.mkdir(parents=True, exist_ok=True)
            descriptor, name = tempfile.mkstemp(prefix='.paris-dungeon-', dir=target.parent)
            try:
                with os.fdopen(descriptor, 'wb') as output, archive.open(entry) as source:
                    shutil.copyfileobj(source, output)
                os.replace(name, target)
            finally:
                if os.path.exists(name):
                    os.unlink(name)
print('Verified dungeon resources installed. Kanturu remains pending.')
