"""One-shot closure of sources/analysis; never executes product."""
from pathlib import Path
import datetime,hashlib,json,platform,subprocess,sys
root=Path(__file__).resolve().parent
manifest=root/'SHA256SUMS'
if manifest.exists(): raise SystemExit('closed receipt is immutable')
before=json.loads((root/'SOURCE_BEFORE.json').read_text())
extra=json.loads((root/'ADDITIONAL_PIN.json').read_text())
rows=[];dev=Path(before['root'])
for row in before['sources']+extra['sources']:
    p=dev/row['path'];live=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
    rows.append({'path':row['path'],'before_sha256':row['sha256'],'live_sha256':live,'changed':live!=row['sha256']})
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=dev,text=True).strip()
(root/'SOURCE_AFTER.json').write_text(json.dumps({'head_after':head,'closed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
 'scope':'LIVE after recorded separately; only frozen pin analyzed','sources':rows},indent=2)+'\n')
(root/'ENVIRONMENT.json').write_text(json.dumps({'python':sys.version,'platform':platform.platform(),'native':False,'build':False,
 'gcp':False,'massive_allocation':False,'product_imports':False},indent=2)+'\n')
(root/'READ_LOG.json').write_text(json.dumps({'lookup_only_failure':{'command':'nl -ba morsehgp3D_v11/bench/catalogue_parallel_diagnostics.py','exit_code':1,
 'stderr':'No such file or directory','interpretation':'No such separate driver in captured inventory; review uses catalogue_parallel.py v2. No product or test executed.'},
 'test_or_model_failures':[]},indent=2)+'\n')
(root/'COMMANDS.txt').write_text('Initial capture before reading: stdlib pathlib copy, SHA256 per file, git rev-parse HEAD and git show HEAD:<path> comparison.\nAdditional document before reading: git show a7cd34ee2a5edbefc6ad98d9854e56e4df278b2e:morsehgp3D_v11/docs/CATALOGUE_PARALLELE.md\nSource-only reads: nl -ba, rg; absent lookup recorded in READ_LOG.json. No product imported or invoked.\npython3 -B scalar_model.py > scalar.stdout.json 2> scalar.stderr\npython3 -B -O scalar_model.py > scalar_opt.stdout.json 2> scalar_opt.stderr\npython3 -B close.py\npython3 -B judge.py\npython3 -B -O judge.py\nsha256sum -c SHA256SUMS\n')
for mode,stem in [([], 'reader'),(['-O'],'reader_opt')]:
    p=subprocess.run([sys.executable,'-B',*mode,str(root/'judge.py')],cwd=root,capture_output=True)
    (root/(stem+'.stdout.json')).write_bytes(p.stdout);(root/(stem+'.stderr')).write_bytes(p.stderr)
    if p.returncode: raise SystemExit('reader failure preserved '+stem)
if (root/'reader.stdout.json').read_bytes()!=(root/'reader_opt.stdout.json').read_bytes(): raise SystemExit('reader normal/-O differs')
files=sorted(p for p in root.rglob('*') if p.is_file() and p!=manifest)
manifest.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(root))+'\n' for p in files))
print(json.dumps({'status':'CLOSED','payloads':len(files),'manifest_sha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),
 'head_after':head,'source_drift':[r['path'] for r in rows if r['changed']]},sort_keys=True))
