"""Capture the archive reader once; never execute engine binaries or GCP."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
require_paths = ('geometry', 'filters_scalar', 'filters_native', 'interfaces',
                 'grid_metric', 'grid_metric_review')
before = {name: hashlib.sha256((root / name / 'SHA256SUMS').read_bytes()).hexdigest()
          for name in require_paths}
commands = []
parsed = []
for name, options in [('normal', []), ('optimized', ['-O'])]:
    argv = [sys.executable, '-B', *options, str(root / 'verify.py'), '--inner-only']
    run = subprocess.run(argv, capture_output=True, text=True, check=False)
    for suffix, output in [('stdout', run.stdout), ('stderr', run.stderr)]:
        with (root / ('verify.' + name + '.' + suffix)).open('x') as stream:
            stream.write(output)
    commands.append(dict(name=name, argv=argv, exit_code=run.returncode))
    if run.returncode == 0:
        parsed.append(json.loads(run.stdout))
after = {name: hashlib.sha256((root / name / 'SHA256SUMS').read_bytes()).hexdigest()
         for name in require_paths}
status = 'PASS' if (before == after and len(parsed) == 2 and parsed[0] == parsed[1] and
                    all(c['exit_code'] == 0 for c in commands)) else 'FAIL'
with (root / 'publication_validation.json').open('x') as stream:
    json.dump(dict(status=status, commands=commands, manifests_before=before,
                   manifests_after=after, reader_results=parsed, GCP_used=False,
                   scope='archive read/rejudgment; no new native engine run'),
              stream, sort_keys=True, indent=2)
paths = sorted(path for path in root.rglob('*')
               if path.is_file() and path != root / 'SHA256SUMS')
with (root / 'SHA256SUMS').open('x') as stream:
    for path in paths:
        stream.write(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' +
                     str(path.relative_to(root)) + '\n')
print(json.dumps(dict(status=status, archived_files=len(paths), GCP_used=False)))
raise SystemExit(0 if status == 'PASS' else 1)
