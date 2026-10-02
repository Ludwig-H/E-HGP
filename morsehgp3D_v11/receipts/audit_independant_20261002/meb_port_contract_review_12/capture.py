from pathlib import Path
import subprocess,hashlib,json,datetime
D=Path('/workspaces/E-HGP/build/v11-development-20261002')
O=Path('/workspaces/E-HGP/build/v11-independent-audit-20261002')
R=Path(__file__).resolve().parent
if (R/'SOURCE_BEFORE.json').exists():raise RuntimeError('Capture already exists')
def git(*a):return subprocess.check_output(['git','-C',str(D),*a])
head=git('rev-parse','HEAD').decode().strip()
files=set()
for directory in ('src/tower','src/core','src/index','src/cloud','src/num','tests/tower'):
 for p in (D/'morsehgp3D_v11'/directory).rglob('*'):
  if p.is_file() and '__pycache__' not in p.parts:files.add(p.relative_to(D).as_posix())
for rel in ('AGENTS.md','morsehgp3D_v11/CMakeLists.txt','morsehgp3D_v11/README.md','morsehgp3D_v11/docs/MEB.md','morsehgp3D_v11/docs/INDEX.md','morsehgp3D_v11/docs/ARCHITECTURE.md','morsehgp3D_v11/docs/CONCEPTION_MOTEUR.md','morsehgp3D_v11/docs/DEVELOPPEMENT.md','morsehgp3D_v11/tests/support/test.hpp','morsehgp3D_v11/tests/index/unit.cpp','morsehgp3D_v11/tests/index/fault.cpp'):
 if (D/rel).is_file():files.add(rel)
rows=[]
for rel in sorted(files):
 b=(D/rel).read_bytes();p=R/'sources'/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b)
 q=subprocess.run(['git','-C',str(D),'show',head+':'+rel],capture_output=True)
 rows.append({'path':rel,'sha256':hashlib.sha256(b).hexdigest(),'size':len(b),'head_blob_sha256':hashlib.sha256(q.stdout).hexdigest() if q.returncode==0 else None,'matches_head_blob':b==q.stdout if q.returncode==0 else False,'tracked_at_head':q.returncode==0})
m={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'developer_root':str(D),'developer_head':head,'own_head':subprocess.check_output(['git','-C',str(O),'rev-parse','HEAD']).decode().strip(),'sources':rows,'scope':'LIVE snapshots before review, WIP distinct from developer HEAD; no native/build/GCP qualification'}
(R/'SOURCE_BEFORE.json').write_text(json.dumps(m,indent=2)+'\n')
print(json.dumps({'status':'CAPTURED','developer_head':head,'own_head':m['own_head'],'sources':len(rows),'wip_paths':[x['path'] for x in rows if not x['matches_head_blob']]}))
