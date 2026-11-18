from pathlib import Path
import hashlib,json,subprocess,argparse,tempfile,os,shutil
parser=argparse.ArgumentParser(description="Install verified MU Paris registration and sale textures")
parser.add_argument("--destination",type=Path,default=Path(__file__).resolve().parents[1]/"src/bin")
args=parser.parse_args();base=args.destination.resolve();manifest=Path(__file__).resolve().parents[1]/"docs/registration-assets.json"
for entry in json.loads(manifest.read_text())["Files"]:
 target=(base/entry["Path"]).resolve()
 if not target.is_relative_to(base):raise ValueError("Asset path leaves destination")
 target.parent.mkdir(parents=True,exist_ok=True)
 if target.exists() and hashlib.sha256(target.read_bytes()).hexdigest()==entry["Sha256"]:continue
 descriptor,name=tempfile.mkstemp(prefix="paris-asset-",dir=target.parent)
 try:
  os.close(descriptor)
  curl=shutil.which("curl")
  if not curl:raise RuntimeError("curl is required to download the assets")
  subprocess.run([curl,"--fail","--silent","--show-error","--retry","2","--max-time","60",entry["Url"],"--output",name],check=True)
  content=Path(name).read_bytes()
  if len(content)!=entry["Size"] or hashlib.sha256(content).hexdigest()!=entry["Sha256"]:raise ValueError("Asset checksum failed "+entry["Path"])
  os.replace(name,target)
 finally:
  if os.path.exists(name):os.unlink(name)
 print("Verified",entry["Path"])
