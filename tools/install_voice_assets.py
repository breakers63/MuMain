"""Restore voice artwork from the published delta ZIP, validating every asset."""
from pathlib import Path
import argparse,hashlib,json,zipfile
parser=argparse.ArgumentParser();parser.add_argument('--archive',required=True)
args=parser.parse_args();root=Path(__file__).resolve().parents[1]
assets=json.loads((root/'docs/paris-voice-assets.json').read_text())
with zipfile.ZipFile(args.archive) as archive:
 for item in assets['files']:
  data=archive.read(item['zipPath'])
  if hashlib.sha256(data).hexdigest()!=item['sha256']:raise ValueError('Artwork checksum mismatch: '+item['path'])
  target=root/item['path'];target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
print('Verified voice artwork restored.')
