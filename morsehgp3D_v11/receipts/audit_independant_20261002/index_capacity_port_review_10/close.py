from pathlib import Path
import datetime,hashlib,json,subprocess,sys
R=Path(__file__).resolve().parent
if (R/'SHA256SUMS').exists():raise ValueError('already closed')
b=json.loads((R/'SOURCE_BEFORE.json').read_text());live=Path(b['live_root']);rows=[]
for name in ('SOURCE_BEFORE.json','SOURCE_EXTRA_BEFORE.json','SOURCE_EXTRA_2_BEFORE.json'):
 for row in json.loads((R/name).read_text())['sources']:
  p=live/row['path'];s=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
  rows.append({'path':row['path'],'snapshot_sha256':row['sha256'],'live_sha256':s,'live_matches_snapshot':s==row['sha256']})
a={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'live_head':subprocess.check_output(['git','-C',str(live),'rev-parse','HEAD']).decode().strip(),'sources':rows,'drift':[r['path'] for r in rows if not r['live_matches_snapshot']]}
(R/'SOURCE_AFTER.json').write_text(json.dumps(a,indent=2)+'\n')
with (R/'COMMANDS.txt').open('a') as f:f.write('\nIndependent model commands (already run):\npython3 -B derive.py > MODEL.json 2> derive.stderr\npython3 -B -O derive.py > MODEL_opt.json 2> derive_opt.stderr\nClosure: python3 -B close.py\nReaders: python3 -B judge.py ; python3 -B -O judge.py\nFinal read-only verification: sha256sum -c SHA256SUMS\nExtra source groups recorded in SOURCE_EXTRA*_BEFORE.json were read with git show e852:<path>; baseline_contracts with git show d0dc:<path>. They are captured, not executed.\n')
for opt,suffix in ((False,''),(True,'_opt')):
 p=subprocess.run([sys.executable,'-B']+(['-O'] if opt else [])+[str(R/'judge.py')],capture_output=True)
 (R/('reader'+suffix+'.stdout.json')).write_bytes(p.stdout);(R/('reader'+suffix+'.stderr')).write_bytes(p.stderr)
 if p.returncode:raise ValueError('reader failure retained '+str(p.returncode))
if (R/'reader.stdout.json').read_bytes()!=(R/'reader_opt.stdout.json').read_bytes():raise ValueError('reader normal/-O mismatch')
files=sorted(p for p in R.rglob('*') if p.is_file() and p!=R/'SHA256SUMS')
(R/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(R).as_posix()+'\n' for p in files))
print(json.dumps({'status':'CLOSED','payloads':len(files),'manifest_sha256':hashlib.sha256((R/'SHA256SUMS').read_bytes()).hexdigest(),'live_head':a['live_head'],'drift':a['drift']}))
