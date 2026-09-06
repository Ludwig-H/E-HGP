#!/usr/bin/env python3
"""Create-only capture of four bounded pure-Python math gate commands."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
GATE = ROOT / 'morsehgp3D_v7/tests/full_gabriel_minima_quotient_gate.py'
PIN = 'bee615b5f8b937e11104597fd674d868828d6b850616582f5163b44454ab9434'
RUN = Path(__file__).resolve().parent / 'rational_capture'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    require(sys.argv[1:] == ['--execute'], 'inert_without_execute')
    require(sha(GATE) == PIN and not RUN.exists(), 'source_pin_and_fresh_destination')
    RUN.mkdir()
    (RUN / GATE.name).write_bytes(GATE.read_bytes())
    (RUN / 'record.py').write_bytes(Path(__file__).read_bytes())
    records, errors = [], []
    for optimized in (False, True):
        for argument, expected in (('--selftest', 0), ('--unknown', 2)):
            name = ('optimized' if optimized else 'normal') + ('_selftest' if expected == 0 else '_unknown')
            argv = [sys.executable, '-B'] + (['-O'] if optimized else []) + [str(GATE), argument]
            (RUN / (name + '.intent.json')).write_text(json.dumps(dict(argv=argv, cwd=str(ROOT), expected=expected,
                                                                      timeout_seconds=10), sort_keys=True) + '\n')
            result = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=10, check=False)
            (RUN / (name + '.stdout')).write_bytes(result.stdout)
            (RUN / (name + '.stderr')).write_bytes(result.stderr)
            row = dict(name=name, argv=argv, expected=expected, exit_code=result.returncode,
                       stdout_sha256=hashlib.sha256(result.stdout).hexdigest(),
                       stderr_sha256=hashlib.sha256(result.stderr).hexdigest())
            records.append(row)
            if result.returncode != expected or result.stderr:
                errors.append(name + ':exit_or_stderr')
            if expected == 0:
                value = json.loads(result.stdout)
                if not (value['status'] == 'passed' and value['models'] == 17
                        and value['corrected_graph_cut_checks'] == 640
                        and value['event_histories_compared'] == 68 and len(value['mutants_killed']) == 10
                        and value['engine_invoked'] is False and value['public_status'] == 'not_claimed'):
                    errors.append(name + ':nonvacuum_or_scope')
    if sha(GATE) != PIN:
        errors.append('source_changed')
    parity = all((RUN / ('normal_' + mode + '.stdout')).read_bytes()
                 == (RUN / ('optimized_' + mode + '.stdout')).read_bytes() for mode in ('selftest', 'unknown'))
    if not parity:
        errors.append('normal_optimized_output_differs')
    report = dict(schema='mhgp7-minima-quotient-math-capture-v1', status='completed' if not errors else 'failed',
                  public_status='not_claimed', engine_invoked=False, source_sha256=PIN,
                  commands=records, errors=errors, normal_optimized_byte_parity=parity)
    (RUN / 'receipt.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    lines = [sha(path) + '  ' + path.name + '\n' for path in sorted(RUN.iterdir()) if path.is_file()]
    (RUN / 'SHA256SUMS').write_text(''.join(lines))
    print(json.dumps(dict(status=report['status'], receipt_sha256=sha(RUN / 'receipt.json'),
                         manifest_sha256=sha(RUN / 'SHA256SUMS'), commands=len(records), errors=errors), sort_keys=True))
    return int(bool(errors))


if __name__ == '__main__':
    raise SystemExit(main())
