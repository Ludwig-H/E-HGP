from pathlib import Path
import subprocess,hashlib,json,datetime
R=Path(__file__).resolve().parent
DEV=Path('/workspaces/E-HGP/build/v11-development-20261002')
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=DEV,text=True).strip()
paths=subprocess.check_output(['git','ls-tree','-r','--name-only',commit],cwd=DEV,text=True).splitlines()
selected=[p for p in paths if p.startswith(('morsehgp3D_v11/src/num/','morsehgp3D_v11/src/core/','morsehgp3D_v11/tests/num/','morsehgp3D_v11/reference/')) or p in ('AGENTS.md','morsehgp3D_v11/CMakeLists.txt','morsehgp3D_v11/docs/ARCHITECTURE.md','morsehgp3D_v11/docs/DEVELOPPEMENT.md','morsehgp3D_v11/docs/PROVENANCE.md','morsehgp3D_v11/tests/test.hpp','morsehgp3D_v11/tests/mutants/manifest.json','morsehgp3D_v11/tests/mutants/run.py') or (p.startswith('morsehgp3D_v11/tests/mutants/') and 'power' in p)]
entries=[]
for p in selected:
 b=subprocess.check_output(['git','show',commit+':'+p],cwd=DEV);live=(DEV/p).read_bytes()
 t=R/'sources'/p;t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(b)
 h=hashlib.sha256(b).hexdigest()
 entries.append({'path':p,'sha256':h,'size':len(b),'live_matches_commit_before':live==b,'double_read_live_stable':hashlib.sha256((DEV/p).read_bytes()).hexdigest()==hashlib.sha256(live).hexdigest(),'live_sha256_before':hashlib.sha256(live).hexdigest()})
if not all(e['double_read_live_stable'] for e in entries):raise RuntimeError('drift during capture')
m={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commit':commit,'working_changes':subprocess.check_output(['git','status','--short','--','morsehgp3D_v11/src/num','morsehgp3D_v11/tests/num'],cwd=DEV,text=True).splitlines(),'entries':entries,'scope':'Static inspection; no native build/test or GCP, no inherited qualification.'}
(R/'SOURCE_BEFORE.json').write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps({'files':len(entries),'commit':commit,'all_live_matches_commit_before':all(e['live_matches_commit_before'] for e in entries),'working_changes':m['working_changes']}))
