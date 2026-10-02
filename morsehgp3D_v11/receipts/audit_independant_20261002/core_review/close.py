from pathlib import Path
import hashlib,json
p=Path(__file__).resolve().parent
a=json.loads((p/'judge_normal.stdout.json').read_text());b=json.loads((p/'judge_optimized.stdout.json').read_text())
if a!=b or a['status']!='PASS':raise SystemExit('judge mismatch')
if (p/'judge_normal.stderr.txt').read_bytes() or (p/'judge_optimized.stderr.txt').read_bytes() or (p/'dependencies.stderr.txt').read_bytes():raise SystemExit('stderr not empty')
before=json.loads((p/'SOURCE_BEFORE.json').read_text());after=json.loads((p/'SOURCE_AFTER.json').read_text())
if {e['path']:e['snapshot_sha256'] for e in before['files']}!={e['path']:e['snapshot_sha256'] for e in after['files']}:raise SystemExit('compiled snapshot changed')
(p/'CLOSURE.json').write_text(json.dumps({'status':'PASS','reader_checks':a['checks'],'normal_optimized_identical':True,'frozen_sources_preserved':True,'live_source_stable':after['stable'],'live_ledger_target_stable':after['ledger_body_live_matches_snapshot'],'source_status':'uncommitted WIP; old binaries do not qualify later live edits'},indent=2)+'\n')
files=sorted(f for f in p.rglob('*') if f.is_file() and f.name!='SHA256SUMS')
(p/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+str(f.relative_to(p))+'\n' for f in files))
print(json.dumps({'status':'PASS','artifacts':len(files),'manifest_sha256':hashlib.sha256((p/'SHA256SUMS').read_bytes()).hexdigest()}))
