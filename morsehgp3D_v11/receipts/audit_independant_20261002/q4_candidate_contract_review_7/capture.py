from pathlib import Path
import subprocess,hashlib,json,datetime
R=Path(__file__).resolve().parent
DEV=Path('/workspaces/E-HGP/build/v11-development-20261002')
COMMIT='9df77494732b03ddf11dbcf1dcb11d96bef54a3b'
paths=subprocess.check_output(['git','ls-tree','-r','--name-only',COMMIT],cwd=DEV,text=True).splitlines()
selected=[p for p in paths if p.startswith(('morsehgp3D_v11/src/catalogue/','morsehgp3D_v11/src/num/','morsehgp3D_v11/src/core/','morsehgp3D_v11/tests/catalogue/','morsehgp3D_v11/tests/num/')) or p in ('AGENTS.md','morsehgp3D_v11/docs/ARCHITECTURE.md','morsehgp3D_v11/docs/CATALOGUE.md','morsehgp3D_v11/docs/DEVELOPPEMENT.md','morsehgp3D_v11/docs/CONCEPTION_MOTEUR.md','morsehgp3D_v11/tests/support/test.hpp','morsehgp3D_v11/tests/mutants/catalogue.json','morsehgp3D_v11/tests/mutants/num.json')]
entries=[]
for p in selected:
 b=subprocess.check_output(['git','show',COMMIT+':'+p],cwd=DEV);live=(DEV/p).read_bytes();t=R/'sources'/p;t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(b)
 entries.append({'group':'product_commit','path':p,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'live_sha256_before':hashlib.sha256(live).hexdigest(),'live_matches_snapshot_before':live==b,'double_read_live_stable':(DEV/p).read_bytes()==live})
p='morsehgp3D_v11/docs/DEVELOPPEMENT.md';b=(DEV/p).read_bytes();t=R/'wip_docs'/p;t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(b)
entries.append({'group':'wip_docs','path':p,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'live_sha256_before':hashlib.sha256(b).hexdigest(),'live_matches_snapshot_before':True,'double_read_live_stable':(DEV/p).read_bytes()==b})
if not all(e['double_read_live_stable'] for e in entries):raise RuntimeError('drift during capture')
m={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commit':COMMIT,'development_head_at_capture':subprocess.check_output(['git','rev-parse','HEAD'],cwd=DEV,text=True).strip(),'source_status':subprocess.check_output(['git','status','--short','--','morsehgp3D_v11/src/num','morsehgp3D_v11/src/catalogue'],cwd=DEV,text=True).splitlines(),'entries':entries,'scope':'Proposed port contract from current source; no candidate code is present in snapshot. Static inspection only, no native build/test/GCP.'}
(R/'SOURCE_BEFORE.json').write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps({'files':len(entries),'commit':COMMIT,'source_status':m['source_status'],'wip_doc_sha256':entries[-1]['sha256']}))
