from pathlib import Path
import datetime,hashlib,json,subprocess,sys
ROOT=Path(__file__).resolve().parent
REPO=Path('/workspaces/E-HGP/build/v11-worktree')
PRODUCT=REPO/'morsehgp3D_v11'
FILES=sorted((PRODUCT/'src/core').glob('*'))+sorted((PRODUCT/'tests/core').glob('*'))
FILES += [PRODUCT/'tests/support/test.hpp',PRODUCT/'docs/ARCHITECTURE.md']
mode=sys.argv[1]
FILES=[f for f in FILES if f.is_file()]
if mode=='close':
 FILES=[REPO/e['path'] for e in json.loads((ROOT/'SOURCE_BEFORE.json').read_text())['files']]
def sha(b):return hashlib.sha256(b).hexdigest()
entries=[]
for f in FILES:
 rel=f.relative_to(REPO);b=f.read_bytes();snapshot=ROOT/'sources'/rel
 if mode=='open':
  if snapshot.exists():raise SystemExit('snapshot exists')
  snapshot.parent.mkdir(parents=True,exist_ok=True);snapshot.write_bytes(b)
 entries.append({'path':str(rel),'bytes':len(b),'sha256':sha(b),'snapshot_sha256':sha(snapshot.read_bytes())})
changed=[e['path'] for e in entries if e['sha256']!=e['snapshot_sha256']]
out={'mode':mode,'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'head_readonly':subprocess.check_output(['git','-C',str(REPO),'rev-parse','HEAD'],text=True).strip(),'git_status_named_scope':subprocess.check_output(['git','-C',str(REPO),'status','--short','--','morsehgp3D_v11/src/core','morsehgp3D_v11/tests/core'],text=True),'files':entries,'stable':not changed,'changed_paths':changed,'ledger_body_live_matches_snapshot':all(e['sha256']==e['snapshot_sha256'] for e in entries if e['path'].endswith(('/ledger.hpp','/ledger.cpp'))),'scope':'Uncommitted WIP core sources and tests, captured before any native execution; live drift preserved, frozen native dependencies never overwritten'}
(ROOT/f'SOURCE_{"BEFORE" if mode=="open" else "AFTER"}.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({'mode':mode,'files':len(entries),'stable':out['stable']}))
if changed:print(json.dumps({'live_source_drift_preserved':changed,'no_current_WIP_requalification':True}))
