#!/usr/bin/env python3
"""Create-only raw capture of the small rational calendar model, no engine."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
SOURCE = ROOT / 'morsehgp3D_v7/tests/full_gabriel_descent_comparison_gate.py'
PIN = '5e357ead0d626121cf66e15d17f0817475520663db7fe5237d9cbd7f25448a16'
DEST = BASE / 'descent_permanent_capture'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    require(sys.argv[1:] == ['--execute'], 'inert_without_execute')
    require(sha(SOURCE) == PIN and not DEST.exists(), 'pinned_source_and_fresh_destination')
    files = [SOURCE, Path(__file__).resolve(),
             ROOT / 'morsehgp3D_v7/tests/full_gabriel_minima_quotient_gate.py',
             ROOT / 'morsehgp3D_v7/src/forest/full_gabriel.hpp',
             ROOT / 'morsehgp3D_v7/src/forest/silent_incidence.hpp',
             ROOT / 'morsehgp3D_v7/src/core/morton.hpp']
    before = {str(path.relative_to(ROOT)): sha(path) for path in files}
    DEST.mkdir()
    for path in files:
        (DEST / path.name).write_bytes(path.read_bytes())
    records, errors = [], []
    for optimized in (False, True):
        for argument, expected in (('--selftest', 0), ('--unknown', 2)):
            name = ('optimized' if optimized else 'normal') + ('_selftest' if expected == 0 else '_unknown')
            argv = [sys.executable, '-B'] + (['-O'] if optimized else []) + [str(SOURCE), argument]
            result = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=10, check=False)
            (DEST / (name + '.stdout')).write_bytes(result.stdout)
            (DEST / (name + '.stderr')).write_bytes(result.stderr)
            records.append(dict(name=name, argv=argv, cwd=str(ROOT), expected=expected,
                                timeout_seconds=10, exit_code=result.returncode,
                                stdout_sha256=sha(DEST / (name + '.stdout')),
                                stderr_sha256=sha(DEST / (name + '.stderr'))))
            if result.returncode != expected or result.stderr:
                errors.append(name + ':code_or_stderr')
            if expected == 0 and result.returncode == 0:
                value = json.loads(result.stdout)
                if not (value['status'] == 'passed' and value['models'] == 17
                        and len(value['rows']) == 51 and value['engine_invoked'] is False
                        and value['timing_claim'] is False and value['public_status'] == 'not_claimed'):
                    errors.append(name + ':scope_or_counts')
    after = {str(path.relative_to(ROOT)): sha(path) for path in files}
    parity = all((DEST / ('normal_' + mode + '.stdout')).read_bytes()
                 == (DEST / ('optimized_' + mode + '.stdout')).read_bytes() for mode in ('selftest', 'unknown'))
    if before != after or not parity:
        errors.append('source_drift_or_output_parity')
    receipt = dict(schema='mhgp7-rational-descent-calendar-capture-v1',
                   status='completed' if not errors else 'failed', public_status='not_claimed',
                   engine_invoked=False, sources_before=before, sources_after=after,
                   normal_optimized_byte_parity=parity, commands=records, errors=errors)
    (DEST / 'receipt.json').write_text(json.dumps(receipt, sort_keys=True, indent=2) + '\n')
    (DEST / 'SHA256SUMS').write_text(''.join(sha(path) + '  ' + path.name + '\n'
        for path in sorted(DEST.iterdir()) if path.is_file()))
    print(json.dumps(dict(status=receipt['status'], receipt_sha256=sha(DEST / 'receipt.json'),
                         manifest_sha256=sha(DEST / 'SHA256SUMS'), commands=len(records)), sort_keys=True))
    return int(bool(errors))


if __name__ == '__main__':
    raise SystemExit(main())
