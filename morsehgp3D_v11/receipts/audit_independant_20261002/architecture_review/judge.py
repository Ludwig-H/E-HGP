from pathlib import Path
import hashlib,json
p=Path(__file__).resolve().parent
def sha(b):return hashlib.sha256(b).hexdigest()
expected={'morsehgp3D_v11/README.md','morsehgp3D_v11/docs/ARCHITECTURE.md','morsehgp3D_v11/docs/PROVENANCE.md','morsehgp3D_v11/audits/README.md'}
a=json.loads((p/'SOURCE_BEFORE.json').read_text());b=json.loads((p/'SOURCE_AFTER.json').read_text())
if {e['path'] for e in a['files']}!=expected or a['files']!=b['files'] or not b['named_docs_unchanged']:raise SystemExit('opening document ledger mismatch')
if a['head_readonly']!='52687f8e532db49c9d7334b49111763a72246e1c':raise SystemExit('unexpected opening commit')
for e in a['files']:
 if sha((p/'sources'/e['path']).read_bytes())!=e['sha256'] or e['sha256']!=e['committed_sha256_at_head']:raise SystemExit('snapshot hash mismatch')
lines=(p/'SHA256SUMS').read_text().splitlines()
if len(lines)<10:raise SystemExit('empty/incomplete artifact manifest')
names=[]
for line in lines:
 h,name=line.split('  ',1);names.append(name)
 if sha((p/name).read_bytes())!=h:raise SystemExit('artifact hash mismatch '+name)
if len(names)!=len(set(names)):raise SystemExit('duplicate artifact')
if set(names)!={str(f.relative_to(p)) for f in p.rglob('*') if f.is_file() and f.name!='SHA256SUMS'}:raise SystemExit('missing artifact')
print(json.dumps({'status':'PASS','scope':'Documentary integrity only; no code qualification','opening_docs':len(expected),'artifacts':len(lines)}))
