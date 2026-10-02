from pathlib import Path
import subprocess,hashlib,json,datetime
live=Path('/workspaces/E-HGP/build/v11-development-20261002')
out=Path(__file__).resolve().parent
pin='d0dc9cd8b8551e559ef8baa9fbcb1d8f2050dcbe'
def git(*a): return subprocess.check_output(['git','-C',str(live),*a])
paths=git('ls-tree','-r','--name-only',pin,'morsehgp3D_v11/src','morsehgp3D_v11/tests/cloud','morsehgp3D_v11/tests/catalogue').decode().splitlines()
paths+=['AGENTS.md','morsehgp3D_v11/README.md','morsehgp3D_v11/CMakeLists.txt','morsehgp3D_v11/docs/ARCHITECTURE.md','morsehgp3D_v11/docs/PROVENANCE.md','morsehgp3D_v11/docs/CATALOGUE.md','morsehgp3D_v11/docs/DEVELOPPEMENT.md','morsehgp3D_v11/docs/CONCEPTION_MOTEUR.md','morsehgp3D_v11/docs/MATHEMATIQUES.md','morsehgp3D_v11/tests/support/test.hpp']
rows=[]
for rel in sorted(set(paths)):
 b=git('show',pin+':'+rel)
 dest=out/'sources'/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(b)
 p=live/rel; lb=p.read_bytes() if p.is_file() else None
 rows.append({'path':rel,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'live_sha256':None if lb is None else hashlib.sha256(lb).hexdigest(),'live_matches_pin':lb==b})
manifest={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'pin':pin,'live_head':git('rev-parse','HEAD').decode().strip(),'live_root':str(live),'sources':rows,'index_source_paths':[r['path'] for r in rows if '/src/index/' in r['path']],'scope':'Git-pinned plan and existing product; no index native code or performance qualification'}
(out/'SOURCE_BEFORE.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'pin':pin,'live_head':manifest['live_head'],'files':len(rows),'index_source_paths':manifest['index_source_paths'],'live_drift':[r['path'] for r in rows if not r['live_matches_pin']]}))
