#!/usr/bin/env python3
"""Read-only replay of seven argv on pinned overlay binaries; no compilation."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
OVERLAY = ROOT / 'build/v7_meb_lazy_gate_proposal'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, label):
    if not condition:
        raise RuntimeError(label)


def main():
    cases = [
        ([], 0, 'meb_lazy_gate=passed mutant=none divergence=none'),
        (['--mutant=silent-meb-q3-reject-shell'], 4,
         'meb_lazy_gate=mutant_killed mutant=silent-meb-q3-reject-shell divergence=differential.status_reason'),
        (['--mutant=silent-meb-q4-reject-shell'], 4,
         'meb_lazy_gate=mutant_killed mutant=silent-meb-q4-reject-shell divergence=differential.status_reason'),
        (['--mutant=silent-meb-eager-materialization'], 4,
         'meb_lazy_gate=mutant_killed mutant=silent-meb-eager-materialization divergence=logical_rejected_materialization'),
        (['--mutant=unknown'], 2, None),
        (['--mutant='], 2, None),
        (['--mutant=silent-meb-q3-reject-shell', '--mutant=silent-meb-q3-reject-shell'], 2, None),
    ]
    expected_bins = {
        'release/mhgp7_meb_lazy_gate': '893b78577f199c80aa0c80bb349af1d7b3ae9c1ad3fa3bb9fd45f0bad4c33b47',
        'sanitized/mhgp7_meb_lazy_gate': 'd92c133db6fb300bd848a6c54e206a5253f75ea93ab3769cfc3b6b985ebdd123',
    }
    records = []
    for relative, expected in expected_bins.items():
        binary = OVERLAY / relative
        require(digest(binary) == expected, 'binary pre-pin')
        env = dict(os.environ)
        if relative.startswith('sanitized/'):
            env['ASAN_OPTIONS'] = 'detect_leaks=1:abort_on_error=1'
            env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        for args, code, prefix in cases:
            start = time.monotonic()
            result = subprocess.run([str(binary), *args], cwd=ROOT, env=env,
                                    text=True, capture_output=True, timeout=30)
            elapsed = time.monotonic() - start
            record = {'binary': relative, 'binary_sha256': expected,
                      'args': args, 'expected_exit_code': code, 'expected_prefix': prefix,
                      'exit_code': result.returncode, 'stdout': result.stdout,
                      'stderr': result.stderr, 'elapsed_seconds': elapsed}
            records.append(record)
            require(result.returncode == code, 'wrong return code')
            require(prefix is None or any(line.startswith(prefix) for line in result.stdout.splitlines()),
                    'missing causal prefix')
            require(not result.stderr, 'unexpected stderr or sanitizer diagnostic')
        require(digest(binary) == expected, 'binary post-pin')
    print(json.dumps({'status': 'passed', 'public_status': 'not_claimed',
                      'scope': '14 argv replays on existing source-owner overlay binaries; not a rebuild',
                      'records': records}, indent=2))


if __name__ == '__main__':
    main()
