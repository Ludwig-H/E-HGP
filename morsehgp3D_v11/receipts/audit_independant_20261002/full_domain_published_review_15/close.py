from pathlib import Path
import datetime,hashlib,json,subprocess,sys
R=Path(__file__).resolve().parent
if (R/'SHA256SUMS').exists():raise ValueError('already closed')
b=json.loads((R/'SOURCE_BEFORE.json').read_text());D=Path(b['developer_root']);rows=[]
for row in b['sources']:
 p=D/row['path'];digest=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
 rows.append({'path':row['path'],'snapshot_sha256':row['sha256'],'live_sha256':digest,'live_matches_snapshot':digest==row['sha256']})
current=set()
for directory in ('src','tests/tower'):
 for p in (D/'morsehgp3D_v11'/directory).rglob('*'):
  if p.is_file() and '__pycache__' not in p.parts:current.add(p.relative_to(D).as_posix())
known={x['path'] for x in b['sources']}
a={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'developer_head_after':subprocess.check_output(['git','-C',str(D),'rev-parse','HEAD']).decode().strip(),'sources':rows,'drift':[x['path'] for x in rows if not x['live_matches_snapshot']],'new_source_paths_unreviewed':sorted(current-known),'scope':'Later LIVE observation; published Git7f snapshots only reviewed, subsequent WIP explicitly unreviewed'}
(R/'SOURCE_AFTER.json').write_text(json.dumps(a,indent=2)+'\n')
with (R/'COMMANDS.txt').open('a') as f:f.write('\npython3 -B judge.py > reader.stdout.json 2> reader.stderr\npython3 -B -O judge.py > reader_opt.stdout.json 2> reader_opt.stderr\npython3 -B close.py (captures LIVE after, runs those independent receipt readers and closes SHA256SUMS)\nFinal readonly: python3 -B judge.py ; python3 -B -O judge.py ; sha256sum -c SHA256SUMS\n')
for opt,suffix in ((False,''),(True,'_opt')):
 p=subprocess.run([sys.executable,'-B']+(['-O'] if opt else [])+[str(R/'judge.py')],capture_output=True)
 (R/('reader'+suffix+'.stdout.json')).write_bytes(p.stdout);(R/('reader'+suffix+'.stderr')).write_bytes(p.stderr)
 if p.returncode:raise ValueError('reader failure retained '+str(p.returncode))
if (R/'reader.stdout.json').read_bytes()!=(R/'reader_opt.stdout.json').read_bytes():raise ValueError('normal/-O reader mismatch')
files=sorted(p for p in R.rglob('*') if p.is_file() and p!=R/'SHA256SUMS')
(R/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(R).as_posix()+'\n' for p in files))
print(json.dumps({'status':'CLOSED','payloads':len(files),'manifest_sha256':hashlib.sha256((R/'SHA256SUMS').read_bytes()).hexdigest(),'developer_head_after':a['developer_head_after'],'source_drift':a['drift'],'new_source_paths_unreviewed':a['new_source_paths_unreviewed']}))
