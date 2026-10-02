from pathlib import Path
import subprocess,json,hashlib
R=Path(__file__).resolve().parent
DEV=Path('/workspaces/E-HGP/build/v11-development-20261002')
m=json.loads((R/'SOURCE_BEFORE.json').read_text())
paths=['morsehgp3D_v11/tests/mutants/num.json','morsehgp3D_v11/tests/mutants/run_mutants.py','morsehgp3D_v11/cmake/gates.cmake','morsehgp3D_v11/tests/support/test.hpp','morsehgp3D_v11/tools/g4_matrix.py','morsehgp3D_v11/tools/g4_matrix.json','morsehgp3D_v11/bench/plans/catalogue_leaf16_g4.json','morsehgp3D_v11/bench/plans/catalogue_profiles_g4.json','morsehgp3D_v11/bench/plans/fondations_g4.json','morsehgp3D_v11/README.md']
for p in paths:
 b=subprocess.check_output(['git','show',m['commit']+':'+p],cwd=DEV);live=(DEV/p).read_bytes();t=R/'sources'/p
 if t.exists():
  if t.read_bytes()!=b:raise RuntimeError('captured bytes differ '+p)
 else:t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(b)
 if not any(e['path']==p for e in m['entries']):m['entries'].append({'path':p,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'live_matches_commit_before':live==b,'double_read_live_stable':(DEV/p).read_bytes()==live,'live_sha256_before':hashlib.sha256(live).hexdigest(),'capture_extra':'before first content read'})
(R/'SOURCE_BEFORE.json').write_text(json.dumps(m,indent=2)+'\n')
pins=json.loads((R/'sources/morsehgp3D_v11/tests/num/source_pins.json').read_text())['power_revision'];entries=[]
for p in pins['baseline_sources']:
 path='morsehgp3D_v11/'+p['path'];b=subprocess.check_output(['git','show',pins['baseline_commit']+':'+path],cwd=DEV)
 t=R/'baseline_sources'/path;t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(b)
 h=hashlib.sha256(b).hexdigest();entries.append({'path':path,'declared_sha256':p['sha256'],'actual_sha256':h,'size':len(b),'matches':h==p['sha256']})
 if h!=p['sha256']:raise RuntimeError('baseline pin mismatch '+path)
(R/'PINS_VERIFICATION.json').write_text(json.dumps({'baseline_commit':pins['baseline_commit'],'entries':entries,'scope':'Git objects read only, exact baseline bytes included; no inherited qualification.'},indent=2)+'\n')
print(json.dumps({'snapshot_files':len(m['entries']),'baseline_files':len(entries),'pins_match':all(e['matches'] for e in entries)}))
