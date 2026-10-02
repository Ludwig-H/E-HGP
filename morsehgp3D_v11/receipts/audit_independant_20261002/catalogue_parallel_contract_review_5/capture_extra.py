from pathlib import Path
import hashlib,json,subprocess
R=Path(__file__).resolve().parent
DEV=Path('/workspaces/E-HGP/build/v11-development-20261002')
WT=Path('/workspaces/E-HGP/build/v11-independent-audit-20261002')
m=json.loads((R/'SOURCE_BEFORE.json').read_text())
for group,p in [('development_live','morsehgp3D_v11/receipts/catalogue_20261002/catalogue2/catalogue.json'),('product_e6','morsehgp3D_v11/bench/catalogue_probe.cpp')]:
 b=(DEV/p).read_bytes() if group=='development_live' else subprocess.check_output(['git','show',m['product_commit']+':'+p],cwd=WT)
 h=hashlib.sha256(b).hexdigest();target=R/'sources'/group/p
 if target.exists():
  if target.read_bytes()!=b:raise RuntimeError('already captured bytes changed: '+p)
 else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(b)
 if not any(e['group']==group and e['path']==p for e in m['entries']):m['entries'].append({'group':group,'path':p,'sha256':h,'size':len(b),'capture_extra':'before reading this dependency'})
 if group=='development_live' and hashlib.sha256((DEV/p).read_bytes()).hexdigest()!=h:raise RuntimeError('drift '+p)
(R/'SOURCE_BEFORE.json').write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps({'captured_files':len(m['entries'])}))
