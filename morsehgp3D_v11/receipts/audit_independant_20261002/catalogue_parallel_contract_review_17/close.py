"""One-shot closure: log LIVE drift, save readers, inventory every payload."""
from pathlib import Path
import datetime,hashlib,json,platform,subprocess,sys
root=Path(__file__).resolve().parent
manifest=root/'SHA256SUMS'
if manifest.exists(): raise SystemExit('immutable capsule already closed')
before=json.loads((root/'SOURCE_BEFORE.json').read_text())
selected=json.loads((root/'SELECTION.json').read_text())['selected_sources']
lookup={r['path']:r for r in before['sources']}
dev=Path(before['developer_root'])
rows=[]
for rel in selected:
    p=dev/rel
    live=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
    rows.append({'path':rel,'before_sha256':lookup[rel]['sha256'],'live_sha256':live,
                 'changed':live!=lookup[rel]['sha256']})
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=dev,text=True).strip()
known=set(lookup)
new_paths=[]
for base in ('morsehgp3D_v11/src','morsehgp3D_v11/tests/catalogue'):
    new_paths.extend(str(p.relative_to(dev)) for p in (dev/base).rglob('*')
                     if p.is_file() and '__pycache__' not in p.parts and str(p.relative_to(dev)) not in known)
(root/'SOURCE_AFTER.json').write_text(json.dumps({'developer_head_after':head,
    'closed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope':'LIVE drift logged, not source replacement or qualification','sources':rows,
    'new_paths_unreviewed':sorted(new_paths)},indent=2)+'\n')
(root/'ENVIRONMENT.json').write_text(json.dumps({'python':sys.version,'platform':platform.platform(),
    'native':False,'build':False,'gcp':False,'massive_allocation':False,'product_imports':False},indent=2)+'\n')
(root/'COMMANDS.txt').write_text('Before review: stdlib pathlib copy of selected LIVE source bytes; git rev-parse HEAD; git show 7f1922c7743d8682e2665a491b01d32e8f2d546c:<path> and SHA256 per source.\nV10 before reading: git show 865f5e64ddd08bedf6ab8f94e8bb94812e380e79:morsehgp3D_v10/src/catalogue/{generator.cpp,support.hpp,catalogue.hpp}; SHA checked against copied source_pins.\npython3 -B scalar_model.py > scalar.stdout.json 2> scalar.stderr\npython3 -B -O scalar_model.py > scalar_opt.stdout.json 2> scalar_opt.stderr\nPool sub-capsule copied after root authorization, all 32 files including SHA256SUMS compared byte hashes to root closed copy.\npython3 -B close.py\npython3 -B judge.py\npython3 -B -O judge.py\nsha256sum -c SHA256SUMS\n')
for mode,stem in [([], 'reader'),(['-O'],'reader_opt')]:
    p=subprocess.run([sys.executable,'-B',*mode,str(root/'judge.py')],cwd=root,capture_output=True)
    (root/(stem+'.stdout.json')).write_bytes(p.stdout)
    (root/(stem+'.stderr')).write_bytes(p.stderr)
    if p.returncode: raise SystemExit('receipt reader failure preserved '+stem)
if (root/'reader.stdout.json').read_bytes()!=(root/'reader_opt.stdout.json').read_bytes():
    raise SystemExit('normal/-O receipt reader differs')
files=sorted(p for p in root.rglob('*') if p.is_file() and p!=manifest)
manifest.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(root))+'\n' for p in files))
print(json.dumps({'status':'CLOSED','payloads':len(files),
 'manifest_sha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),
 'source_drift':[r['path'] for r in rows if r['changed']],
 'developer_head_after':head,'new_paths_unreviewed':sorted(new_paths)},sort_keys=True))
