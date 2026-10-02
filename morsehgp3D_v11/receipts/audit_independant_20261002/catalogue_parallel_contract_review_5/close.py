from pathlib import Path
import datetime,hashlib,json,subprocess
R=Path(__file__).resolve().parent
WT=Path('/workspaces/E-HGP/build/v11-independent-audit-20261002')
DEV=Path('/workspaces/E-HGP/build/v11-development-20261002')
before=json.loads((R/'SOURCE_BEFORE.json').read_text());entries=[]
for e in before['entries']:
 snap=(R/'sources'/e['group']/e['path']).read_bytes();livebase=DEV if e['group']=='development_live' else WT
 live=(livebase/e['path']).read_bytes()
 entries.append({'group':e['group'],'path':e['path'],'snapshot_sha256':hashlib.sha256(snap).hexdigest(),'snapshot_matches_before':hashlib.sha256(snap).hexdigest()==e['sha256'],'live_sha256':hashlib.sha256(live).hexdigest(),'live_matches_snapshot':live==snap})
after={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'product_head_at_close':subprocess.check_output(['git','rev-parse','HEAD'],cwd=WT,text=True).strip(),'development_head_at_close':subprocess.check_output(['git','rev-parse','HEAD'],cwd=DEV,text=True).strip(),'snapshot_intact':all(e['snapshot_matches_before'] for e in entries),'all_live_matches':all(e['live_matches_snapshot'] for e in entries),'entries':entries}
(R/'SOURCE_AFTER.json').write_text(json.dumps(after,indent=2)+'\n')
if not after['snapshot_intact']:raise RuntimeError('snapshot drift')
for args,name in [(['python3','-B','judge.py'],'reader'),(['python3','-B','-O','judge.py'],'reader_opt')]:
 result=subprocess.run(args,cwd=R,capture_output=True)
 (R/(name+'.stdout')).write_bytes(result.stdout);(R/(name+'.stderr')).write_bytes(result.stderr)
 if result.returncode:raise RuntimeError(name+' failed')
if (R/'reader.stdout').read_bytes()!=(R/'reader_opt.stdout').read_bytes():raise RuntimeError('normal/-O diverge')
paths=sorted(p for p in R.rglob('*') if p.is_file() and p.name!='SHA256SUMS')
(R/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(R))+'\n' for p in paths))
print(json.dumps({'files':len(paths),'all_live_matches':after['all_live_matches'],'snapshot_intact':after['snapshot_intact'],'manifest_sha256':hashlib.sha256((R/'SHA256SUMS').read_bytes()).hexdigest(),'reader':(R/'reader.stdout').read_text().strip()},sort_keys=True))
