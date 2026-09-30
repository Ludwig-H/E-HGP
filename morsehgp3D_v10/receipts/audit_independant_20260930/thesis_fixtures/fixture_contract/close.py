from pathlib import Path
import hashlib
import json

p = Path(__file__).resolve().parent
a = json.loads((p/'normal.stdout.json').read_text())
b = json.loads((p/'optimized.stdout.json').read_text())
if a != b or a['status'] != 'PASS' or a['checks'] != 35060:
    raise SystemExit('normal/optimized verdict mismatch')
if any((p/n).read_bytes() for n in ('normal.stderr.txt','optimized.stderr.txt')):
    raise SystemExit('nonempty verifier stderr')
before = json.loads((p/'SOURCE_BEFORE.json').read_text())
after = json.loads((p/'SOURCE_AFTER.json').read_text())
if not after['inputs_and_named_sources_stable'] or before['files'] != after['files']:
    raise SystemExit('named sources changed')
out = {'status':'PASS', 'checks':a['checks'], 'normal_optimized_identical':True,
       'nine_named_sources_unchanged':True,
       'scope':'Archived u18 six-site exports; no fresh native execution or ideal-equilateral qualification'}
(p/'CLOSURE.json').write_text(json.dumps(out,indent=2)+'\n')
paths = sorted(x for x in p.rglob('*') if x.is_file() and x.name != 'SHA256SUMS')
lines = [hashlib.sha256(f.read_bytes()).hexdigest()+'  '+str(f.relative_to(p)) for f in paths]
(p/'SHA256SUMS').write_text('\n'.join(lines)+'\n')
for f, line in zip(paths, lines):
    if hashlib.sha256(f.read_bytes()).hexdigest() != line[:64]:
        raise SystemExit('closure artifact changed')
print(json.dumps({'status':'PASS','artifacts':len(paths),'checks':a['checks'],
                  'SHA256SUMS_sha256':hashlib.sha256((p/'SHA256SUMS').read_bytes()).hexdigest()}))
