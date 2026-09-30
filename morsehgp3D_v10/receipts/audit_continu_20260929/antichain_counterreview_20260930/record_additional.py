"""Fresh captures of portable proof scripts; preserve the first counter-review."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parent
PACKET = ROOT.parents[1] / 'audit_independant_20260930' / 'fixed_k_antichain'
REFERENCE = ROOT.parent / 'point_condensation_cover_r2_20260930' / 'reference' / 'hgp10_ref.py'


def pins():
    paths = [ROOT / n for n in ['record_additional.py', 'geometry_default.py', 'stream_extrema.py']]
    paths += [PACKET / 'SHA256SUMS', REFERENCE]
    paths += [PACKET / line.split('  ', 1)[1]
              for line in (PACKET / 'SHA256SUMS').read_text().splitlines()]
    return {str(p.relative_to(ROOT.parents[1])): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in paths}


if (ROOT / 'additional_receipt.json').exists():
    raise RuntimeError('existing capture; use a fresh copy')
before = pins()
runs = []
for script in ['geometry_default.py', 'stream_extrema.py']:
    decoded = []
    for tag, switches in [('normal', []), ('optimized', ['-O'])]:
        argv = ['python3', '-B', *switches, script]
        started = time.time_ns()
        result = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=30)
        ended = time.time_ns()
        stem = script[:-3] + '.' + tag
        for stream, data in [('stdout', result.stdout), ('stderr', result.stderr)]:
            (ROOT / f'{stem}.{stream}').write_bytes(data)
        runs.append(dict(argv=argv, returncode=result.returncode, started_unix_ns=started,
                         ended_unix_ns=ended, stdout=f'{stem}.stdout', stderr=f'{stem}.stderr'))
        if result.returncode == 0:
            value = json.loads(result.stdout)
            value.pop('optimize', None)
            decoded.append(value)
    if len(decoded) == 2 and decoded[0] != decoded[1]:
        raise RuntimeError('normal/-O disagreement; raw files preserved')
after = pins()
receipt = dict(scope='Exact Fraction geometry and Euler structural proof; Python only',
               sources_before=before, sources_after=after, commands=runs,
               native_invocations=0, sklearn_invocations=0, GCP_used=False)
(ROOT / 'additional_receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
if before != after or any(r['returncode'] != 0 for r in runs):
    raise RuntimeError('unstable source or failed run; raw files preserved')
print(json.dumps(dict(status='PASS', captured_python_invocations=len(runs),
                      source_pins=len(before), native_invocations=0, GCP_used=False), sort_keys=True))
