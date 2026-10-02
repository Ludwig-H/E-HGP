"""Four published opening docs only; no product/build/git mutations."""
from pathlib import Path
import datetime,hashlib,json,subprocess,sys
ROOT=Path(__file__).resolve().parent
REPO=Path('/workspaces/E-HGP/build/v11-worktree')
PATHS=['morsehgp3D_v11/README.md','morsehgp3D_v11/docs/ARCHITECTURE.md','morsehgp3D_v11/docs/PROVENANCE.md','morsehgp3D_v11/audits/README.md']
def sha(b):return hashlib.sha256(b).hexdigest()
head=subprocess.check_output(['git','-C',str(REPO),'rev-parse','HEAD'],text=True).strip()
mode=sys.argv[1]
entries=[]
for rel in PATHS:
 b=(REPO/rel).read_bytes();copy=ROOT/'sources'/rel
 if mode=='open':
  if copy.exists():raise SystemExit('snapshot already exists')
  copy.parent.mkdir(parents=True,exist_ok=True);copy.write_bytes(b)
 entries.append({'path':rel,'bytes':len(b),'sha256':sha(b),'snapshot_sha256':sha(copy.read_bytes()),'committed_sha256_at_head':sha(subprocess.check_output(['git','-C',str(REPO),'show',head+':'+rel]))})
out={'mode':mode,'read_time_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'head_readonly':head,'files':entries,'named_docs_unchanged':all(e['sha256']==e['snapshot_sha256'] for e in entries),'scope':'Only opening architecture documents, no Session/Buffer/IO implementation present at this audit.'}
(ROOT/f'SOURCE_{"BEFORE" if mode=="open" else "AFTER"}.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({'mode':mode,'head':head,'docs':len(entries),'unchanged':out['named_docs_unchanged']}))
if not out['named_docs_unchanged']:raise SystemExit('opening docs changed')
