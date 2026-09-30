"""Capture a bounded Python counter-review. Does not invoke any native engine."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parent
PACKET = ROOT.parents[1] / 'audit_independant_20260930' / 'fixed_k_antichain'


def pins():
    paths = [ROOT / 'record.py', ROOT / 'rejudge.py', PACKET / 'SHA256SUMS']
    paths += [PACKET / line.split('  ', 1)[1]
              for line in (PACKET / 'SHA256SUMS').read_text().splitlines()]
    return {str(p.relative_to(ROOT.parents[1])): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in paths}


if (ROOT / 'receipt.json').exists():
    raise RuntimeError('closed output exists; use a fresh copy')
before = pins()
runs = []
for tag, switches in [('normal', []), ('optimized', ['-O'])]:
    argv = ['python3', '-B', *switches, 'rejudge.py']
    started = time.time_ns()
    result = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=30)
    ended = time.time_ns()
    for stream, data in [('stdout', result.stdout), ('stderr', result.stderr)]:
        (ROOT / f'{tag}.{stream}').write_bytes(data)
    runs.append(dict(argv=argv, returncode=result.returncode, started_unix_ns=started,
                     ended_unix_ns=ended, stdout=f'{tag}.stdout', stderr=f'{tag}.stderr'))
after = pins()
receipt = dict(scope='Python archive counter-review only', sources_before=before,
               sources_after=after, commands=runs, native_invocations=0,
               sklearn_invocations=0, GCP_used=False)
(ROOT / 'receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
if before != after or any(r['returncode'] != 0 for r in runs):
    raise RuntimeError('unstable sources or failed run; raw evidence retained')
outputs = [json.loads((ROOT / f'{tag}.stdout').read_text()) for tag in ['normal', 'optimized']]
for expected, result in enumerate(outputs):
    if result.pop('optimize') != expected:
        raise RuntimeError('wrong optimization mode')
if outputs[0] != outputs[1]:
    raise RuntimeError('normal/-O results differ')
print(json.dumps(outputs[0], sort_keys=True))
