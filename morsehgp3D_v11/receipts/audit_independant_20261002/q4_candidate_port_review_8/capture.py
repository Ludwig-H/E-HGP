from pathlib import Path
import json, hashlib, subprocess, datetime, sys
live=Path('/workspaces/E-HGP/build/v11-development-20261002')
out=Path(__file__).resolve().parent
prefix='morsehgp3D_v11/'
paths=['AGENTS.md', prefix+'CMakeLists.txt',prefix+'tests/support/test.hpp',prefix+'tests/mutants/num.json',prefix+'tests/mutants/catalogue.json',prefix+'docs/ARCHITECTURE.md',prefix+'docs/CATALOGUE.md',prefix+'docs/DEVELOPPEMENT.md',prefix+'docs/CONCEPTION_MOTEUR.md',prefix+'bench/catalogue_probe.cpp',prefix+'bench/catalogue_g4.py',prefix+'bench/plans/q4_levels_g4.json']
for tree in ('src/core','src/num','src/catalogue','tests/num','tests/catalogue'):
 for p in sorted((live/prefix/tree).rglob('*')):
  if p.is_file() and '__pycache__' not in p.parts: paths.append(str(p.relative_to(live)))
def git(*args): return subprocess.check_output(['git','-C',str(live),*args],text=True).strip()
rows=[]
for rel in sorted(set(paths)):
 p=live/rel
 if not p.is_file(): raise RuntimeError('Missing dependency '+rel)
 b=p.read_bytes(); dest=out/'sources'/rel; dest.parent.mkdir(parents=True,exist_ok=True); dest.write_bytes(b)
 again=p.read_bytes()
 rows.append({'path':rel,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'capture_stable':again==b})
if not all(x['capture_stable'] for x in rows): raise RuntimeError('Source moved during capture')
manifest={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'live_root':str(live),'head':git('rev-parse','HEAD'),'status':git('status','--short','--','AGENTS.md',prefix+'src',prefix+'tests/num',prefix+'tests/catalogue',prefix+'docs',prefix+'bench/catalogue_probe.cpp',prefix+'bench/catalogue_g4.py',prefix+'bench/plans/q4_levels_g4.json'),'sources':rows,'scope':'WIP snapshots, not a native qualification'}
(out/'SOURCE_BEFORE.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'head':manifest['head'],'count':len(rows),'status':manifest['status']}))
