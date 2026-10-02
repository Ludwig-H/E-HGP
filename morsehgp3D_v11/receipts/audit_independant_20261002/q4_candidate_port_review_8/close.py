from pathlib import Path
import hashlib,json,datetime,subprocess,sys
ROOT=Path(__file__).resolve().parent
if (ROOT/'SHA256SUMS').exists(): raise RuntimeError('Already closed')
b=json.loads((ROOT/'SOURCE_BEFORE.json').read_text()); e=json.loads((ROOT/'SOURCE_EXTRA_BEFORE.json').read_text()); live=Path(b['live_root']); rows=[]
for row in b['sources']+e['sources']:
 p=live/row['path']; current=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
 rows.append({'path':row['path'],'captured_sha256':row['sha256'],'live_sha256':current,'changed':current!=row['sha256']})
after={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'head':subprocess.check_output(['git','-C',str(live),'rev-parse','HEAD'],text=True).strip(),'checks':rows,'product_drift':[r['path'] for r in rows if r['changed'] and '/src/' in r['path']],'scope':'Source/document drift is reported, never rewritten into initial snapshots.'}
(ROOT/'SOURCE_AFTER.json').write_text(json.dumps(after,indent=2)+'\n')
commands=(ROOT/'COMMANDS.txt').read_text()+'python3 -B derive.py > REVIEW.json 2> derive.stderr\npython3 -B -O derive.py > REVIEW_opt.json 2> derive_opt.stderr\npython3 -B close.py\n  python3 -B judge.py > reader.stdout.json 2> reader.stderr\n  python3 -B -O judge.py > reader_opt.stdout.json 2> reader_opt.stderr\nRead-only after closure: python3 -B judge.py ; python3 -B -O judge.py ; sha256sum -c --quiet SHA256SUMS\n'
(ROOT/'COMMANDS.txt').write_text(commands)
for opt,name in ((False,'reader'),(True,'reader_opt')):
 argv=[sys.executable,'-B']+(['-O'] if opt else [])+[str(ROOT/'judge.py')]
 result=subprocess.run(argv,cwd=ROOT,capture_output=True)
 (ROOT/(name+'.stdout.json')).write_bytes(result.stdout); (ROOT/(name+'.stderr')).write_bytes(result.stderr)
 if result.returncode: raise RuntimeError('Receipt reader failed: '+name)
if (ROOT/'reader.stdout.json').read_bytes()!=(ROOT/'reader_opt.stdout.json').read_bytes(): raise RuntimeError('reader normal/optimized differ')
files=sorted(p for p in ROOT.rglob('*') if p.is_file() and p.name!='SHA256SUMS' and '__pycache__' not in p.parts)
(ROOT/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(ROOT))+'\n' for p in files))
print(json.dumps({'files':len(files),'manifest_sha256':hashlib.sha256((ROOT/'SHA256SUMS').read_bytes()).hexdigest(),'head_after':after['head'],'product_drift':after['product_drift'],'document_drift':[r['path'] for r in rows if r['changed'] and '/src/' not in r['path']],'reader':json.loads((ROOT/'reader.stdout.json').read_text())},indent=2))
