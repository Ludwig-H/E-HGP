"""Fresh normal/-O captures of the bounded grid audit."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
before = {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in [root / 'check.py', *sorted((root / 'source').glob('*.py'))]}
results = []
for name, options in [('normal', []), ('optimized', ['-O'])]:
    argv = [sys.executable, '-B', *options, str(root / 'check.py')]
    run = subprocess.run(argv, capture_output=True, text=True, check=False)
    for suffix, output in [('stdout', run.stdout), ('stderr', run.stderr)]:
        with (root / (name + '.' + suffix)).open('x') as stream:
            stream.write(output)
    results.append(dict(name=name, argv=argv, returncode=run.returncode))
after = {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in before}
status = 'PASS' if before == after and all(r['returncode'] == 0 for r in results) else 'FAIL'
if status == 'PASS':
    normal = json.loads((root / 'normal.stdout').read_text())
    optimized = json.loads((root / 'optimized.stdout').read_text())
    normal.pop('optimize_flag')
    optimized.pop('optimize_flag')
    if normal != optimized:
        status = 'FAIL'
with (root / 'execution.json').open('x') as stream:
    json.dump(dict(status=status, commands=results, source_before=before, source_after=after,
                   paired_results_equal_except_optimize=status == 'PASS', GCP_used=False),
              stream, sort_keys=True, indent=2)
print(json.dumps(dict(status=status, commands=len(results), scope='metric audit only')))
raise SystemExit(0 if status == 'PASS' else 1)
