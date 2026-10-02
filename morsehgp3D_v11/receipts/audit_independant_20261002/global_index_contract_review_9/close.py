from pathlib import Path
import json,subprocess,datetime,hashlib,sys
ROOT=Path(__file__).resolve().parent
if (ROOT/'SHA256SUMS').exists():raise RuntimeError('Already closed')
b=json.loads((ROOT/'SOURCE_BEFORE.json').read_text());live=Path(b['live_root']);rows=[]
for row in b['sources']:
 p=live/row['path'];s=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
 rows.append({'path':row['path'],'captured_sha256':row['sha256'],'live_sha256':s,'changed':s!=row['sha256']})
after={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'pin':b['pin'],'live_head':subprocess.check_output(['git','-C',str(live),'rev-parse','HEAD'],text=True).strip(),'checks':rows,'drift':[r['path'] for r in rows if r['changed']],'scope':'WIP after initial capture is reported separately; only Git d0 snapshots were reviewed here.'}
(ROOT/'SOURCE_AFTER.json').write_text(json.dumps(after,indent=2)+'\n')
with (ROOT/'COMMANDS.txt').open('a') as f:f.write('python3 -B derive.py > MODEL.json 2> derive.stderr\npython3 -B -O derive.py > MODEL_opt.json 2> derive_opt.stderr\npython3 -B close.py\n  python3 -B judge.py > reader.stdout.json 2> reader.stderr\n  python3 -B -O judge.py > reader_opt.stdout.json 2> reader_opt.stderr\nRead-only after closure: python3 -B judge.py ; python3 -B -O judge.py ; sha256sum -c --quiet SHA256SUMS\n')
for opt,name in ((False,'reader'),(True,'reader_opt')):
 result=subprocess.run([sys.executable,'-B']+(['-O'] if opt else [])+[str(ROOT/'judge.py')],cwd=ROOT,capture_output=True)
 (ROOT/(name+'.stdout.json')).write_bytes(result.stdout);(ROOT/(name+'.stderr')).write_bytes(result.stderr)
 if result.returncode:raise RuntimeError('reader failed '+name)
if (ROOT/'reader.stdout.json').read_bytes()!=(ROOT/'reader_opt.stdout.json').read_bytes():raise RuntimeError('reader modes differ')
files=sorted(p for p in ROOT.rglob('*') if p.is_file() and p.name!='SHA256SUMS' and '__pycache__' not in p.parts)
(ROOT/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(ROOT))+'\n' for p in files))
print(json.dumps({'files':len(files),'manifest_sha256':hashlib.sha256((ROOT/'SHA256SUMS').read_bytes()).hexdigest(),'after_head':after['live_head'],'drift':after['drift'],'reader':json.loads((ROOT/'reader.stdout.json').read_text())},indent=2))
