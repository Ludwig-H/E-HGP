from pathlib import Path
import subprocess,json,hashlib
R=Path(__file__).resolve().parent
DEV=Path('/workspaces/E-HGP/build/v11-development-20261002')
m=json.loads((R/'SOURCE_BEFORE.json').read_text())
for p in ['morsehgp3D_v11/bench/catalogue_probe.cpp','morsehgp3D_v11/receipts/catalogue_20261002/check.py','morsehgp3D_v11/bench/catalogue_g4.py']:
 b=subprocess.check_output(['git','show',m['commit']+':'+p],cwd=DEV);live=(DEV/p).read_bytes();t=R/'sources'/p
 if t.exists():
  if t.read_bytes()!=b:raise RuntimeError('captured bytes changed '+p)
 else:t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(b)
 if not any(e['group']=='product_commit' and e['path']==p for e in m['entries']):m['entries'].append({'group':'product_commit','path':p,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'live_sha256_before':hashlib.sha256(live).hexdigest(),'live_matches_snapshot_before':live==b,'double_read_live_stable':(DEV/p).read_bytes()==live,'capture_extra':'before first content read'})
(R/'SOURCE_BEFORE.json').write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps({'files':len(m['entries'])}))
