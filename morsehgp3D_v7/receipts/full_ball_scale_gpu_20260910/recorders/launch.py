#!/usr/bin/env python3
"""Only the pinned guarded controller owns this exact short SPOT session."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path('/workspaces/E-HGP')
OUT = Path(__file__).resolve().parent
SESSION = Path(sys.argv[1]).resolve()
if not SESSION.is_dir() or SESSION.stat().st_mode & 0o777 != 0o700:
    raise SystemExit('private session required')
pins = json.loads((OUT / 'input_r1/pins.json').read_text())
for item in pins.values():
    if hashlib.sha256(Path(item['path']).read_bytes()).hexdigest() != item['sha256']:
        raise SystemExit('pinned source changed')
argv = ['python3', pins['controller']['path'], '--execute', '--session-dir', str(SESSION),
        '--ssh-key', str(SESSION / 'session_key')]
for key in ('snapshot', 'manifest', 'worker'):
    argv += ['--' + key, pins[key]['path'], '--' + key + '-sha256', pins[key]['sha256']]
argv += ['--expected-controller-sha256', pins['controller']['sha256']]
with (OUT / 'launch.json').open('x') as stream:
    json.dump(dict(argv=argv, session=str(SESSION), source_manifest=pins['manifest']), stream, indent=2)
with (OUT / 'controller.stdout').open('xb') as stdout, (OUT / 'controller.stderr').open('xb') as stderr:
    result = subprocess.run(argv, cwd=ROOT, stdout=stdout, stderr=stderr)
print(json.dumps(dict(exit_code=result.returncode, session=str(SESSION)), sort_keys=True))
raise SystemExit(result.returncode)
