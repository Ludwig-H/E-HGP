"""Archived Fraction geometry only; no native or LIVE generator invocation."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parent
WANTED = {'check.py', 'PROTOCOL.txt', 'receipt.json', 'normal.stdout.json',
          'optimized.stdout.json', 'normal.stderr', 'optimized.stderr',
          'CAPTURE_SHA256SUMS', 'README.md', 'verify.py'}


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


pins = {}
for line in (ROOT / 'SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    require(name not in pins and name in WANTED, 'unexpected manifest member')
    require(sha(ROOT / name) == digest, 'hash mismatch: ' + name)
    pins[name] = digest
require(set(pins) == WANTED, 'incomplete manifest')
capture = {}
for line in (ROOT / 'CAPTURE_SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    require(name not in capture and pins.get(name) == digest, 'original closure differs')
    capture[name] = digest
require(len(capture) == 7, 'original capture inventory')
r = json.loads((ROOT / 'receipt.json').read_text())
require(r['sources_unchanged_after_execution'] is True, 'capture source instability')
require(r['source_and_protocol_frozen_before_execution'] ==
        {n: pins[n] for n in ['PROTOCOL.txt', 'check.py']}, 'source pins differ')
require(r['qualification_transfers'] == {n: False for n in [
        'new_generator_execution', 'real_leaf_gain', 'LiDAR_gain', 'FULL',
        'subquadratic_global', 'GPU_G4']}, 'qualification scope changed')
for tag, argv in [('normal', ['python3', '-B', 'check.py']),
                  ('optimized', ['python3', '-B', '-O', 'check.py'])]:
    row = r['runs'][0 if tag == 'normal' else 1]
    require(row['command'] == argv and row['return_code'] == 0 and row['checks'] == 503,
            'wrong captured invocation')
    require(row['stdout'] == tag + '.stdout.json' and row['stderr'] == tag + '.stderr',
            'wrong captured streams')
    require((ROOT / row['stderr']).read_bytes() == b'', 'nonempty stderr')
    require(sha(ROOT / row['stdout']) == r['stdout_identical_sha256'], 'captured outputs differ')
buffer = io.StringIO()
with contextlib.redirect_stdout(buffer):
    runpy.run_path(str(ROOT / 'check.py'), run_name='__main__')
require(buffer.getvalue() == (ROOT / 'normal.stdout.json').read_text(), 'geometry replay differs')
require(json.loads(buffer.getvalue())['checks'] == 503, 'wrong control count')
require(all(sha(ROOT / n) == digest for n, digest in pins.items()), 'archive changed')
print(json.dumps(dict(status='ARCHIVE_PASS', manifest_entries=len(pins),
                      fraction_controls_now=503, native_invocations=0,
                      generator_invocations=0, GCP_used=False), sort_keys=True))
