from pathlib import Path
import datetime,hashlib,json,subprocess,sys
R=Path(__file__).resolve().parent
if (R/'SHA256SUMS').exists():raise ValueError('already closed')
b=json.loads((R/'SOURCE_BEFORE.json').read_text());D=Path(b['developer_root']);rows=[]
for row in b['sources']:
 p=D/row['path'];digest=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
 rows.append({'path':row['path'],'snapshot_sha256':row['sha256'],'live_sha256':digest,'live_matches_snapshot':digest==row['sha256']})
a={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'developer_head_after':subprocess.check_output(['git','-C',str(D),'rev-parse','HEAD']).decode().strip(),'sources':rows,'drift':[x['path'] for x in rows if not x['live_matches_snapshot']],'scope':'Subsequent LIVE observation; copied pin24e4 remains sole reviewed source'}
(R/'SOURCE_AFTER.json').write_text(json.dumps(a,indent=2)+'\n')
with (R/'COMMANDS.txt').open('a') as f:f.write('\nHash baseline comparison: git -C DEV show e8520481d1745627e156723ad995ac5175a8163f:<path>, for each core/cloud/index/num captured dependency; metadata in BASELINE_DEPENDENCIES.json. No source executed.\npython3 -B capacity.py > CAPACITY.json 2> capacity.stderr\npython3 -B -O capacity.py > CAPACITY_opt.json 2> capacity_opt.stderr\npython3 -B judge.py > reader.stdout.json 2> reader.stderr\npython3 -B -O judge.py > reader_opt.stdout.json 2> reader_opt.stderr\npython3 -B close.py (captures LIVE after, executes those autonomous commands, closes exhaustive SHA256SUMS)\nFinal readonly: python3 -B judge.py ; python3 -B -O judge.py ; sha256sum -c SHA256SUMS\n')
for opt,suffix in ((False,''),(True,'_opt')):
 p=subprocess.run([sys.executable,'-B']+(['-O'] if opt else [])+[str(R/'capacity.py')],capture_output=True)
 (R/('CAPACITY'+suffix+'.json')).write_bytes(p.stdout);(R/('capacity'+suffix+'.stderr')).write_bytes(p.stderr)
 if p.returncode:raise ValueError('capacity failure retained '+str(p.returncode))
for opt,suffix in ((False,''),(True,'_opt')):
 p=subprocess.run([sys.executable,'-B']+(['-O'] if opt else [])+[str(R/'judge.py')],capture_output=True)
 (R/('reader'+suffix+'.stdout.json')).write_bytes(p.stdout);(R/('reader'+suffix+'.stderr')).write_bytes(p.stderr)
 if p.returncode:raise ValueError('reader failure retained '+str(p.returncode))
if (R/'reader.stdout.json').read_bytes()!=(R/'reader_opt.stdout.json').read_bytes():raise ValueError('reader normal/-O mismatch')
files=sorted(p for p in R.rglob('*') if p.is_file() and p!=R/'SHA256SUMS')
(R/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(R).as_posix()+'\n' for p in files))
print(json.dumps({'status':'CLOSED','payloads':len(files),'manifest_sha256':hashlib.sha256((R/'SHA256SUMS').read_bytes()).hexdigest(),'developer_head_before':b['developer_head'],'developer_head_after':a['developer_head_after'],'source_drift':a['drift']}))
