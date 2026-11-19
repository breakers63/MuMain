"""Restore byte-exact auxiliary files from this source checkpoint."""
from pathlib import Path,PurePosixPath
import base64,hashlib,io,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
meta=json.loads((ROOT/'source-backup/manifest.json').read_text())
parts=sorted((ROOT/'source-backup/binary').glob('*.b64'))
if parts:
 raw=base64.b64decode(''.join(p.read_text().strip() for p in parts),validate=True)
 if hashlib.sha256(raw).hexdigest()!=meta['binaryArchiveSha256']:raise ValueError('Backup checksum mismatch')
 with zipfile.ZipFile(io.BytesIO(raw)) as archive:
  for entry in archive.infolist():
   rel=PurePosixPath(entry.filename)
   if rel.is_absolute() or '..' in rel.parts or '\\' in entry.filename or ':' in entry.filename:raise ValueError('Unsafe backup path')
   target=ROOT.joinpath(*rel.parts).resolve()
   if not target.is_relative_to(ROOT):raise ValueError('Backup escapes repository')
   content=archive.read(entry)
   expected=next(x['sha256'] for x in meta['files'] if x['path']==entry.filename)
   if hashlib.sha256(content).hexdigest()!=expected:raise ValueError('File checksum mismatch')
   target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(content)
print('Auxiliary files restored. See source-backup/README.md for submodule patches and data resources.')
