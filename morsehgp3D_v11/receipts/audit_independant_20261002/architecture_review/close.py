from pathlib import Path
import hashlib,json
p=Path(__file__).resolve().parent
a=json.loads((p/'SOURCE_BEFORE.json').read_text());b=json.loads((p/'SOURCE_AFTER.json').read_text())
if a['files']!=b['files'] or not b['named_docs_unchanged']:raise SystemExit('opening documents changed')
files=sorted(f for f in p.rglob('*') if f.is_file() and f.name!='SHA256SUMS')
(p/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+str(f.relative_to(p))+'\n' for f in files))
print(json.dumps({'status':'PASS','artifacts':len(files),'manifest_sha256':hashlib.sha256((p/'SHA256SUMS').read_bytes()).hexdigest()}))
