#!/usr/bin/env python3
"""Closed command recorder; all checks remain active under python -O."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent
MUTANTS = ['drop_vertex_activation', 'drop_unary_contribution', 'count_keys_not_roots',
           'binary_same_level', 'vertical_open', 'admit_global_key_without_K', 'consult_partial_calendar']


def need(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    out = BASE / ('runs_optimized' if sys.flags.optimize else 'runs_normal')
    out.mkdir(exist_ok=False)
    records = []
    for optimized in (False, True):
        for mutant in ['', *MUTANTS]:
            cmd = [sys.executable, '-B', *(['-O'] if optimized else []), str(BASE / 'graph_oracle.py')]
            if mutant:
                cmd += ['--mutant', mutant]
            proc = subprocess.run(cmd, cwd=BASE, capture_output=True, check=False)
            name = ('opt_' if optimized else 'normal_') + (mutant or 'nominal')
            (out / (name + '.stdout')).write_bytes(proc.stdout)
            (out / (name + '.stderr')).write_bytes(proc.stderr)
            need(proc.returncode == (1 if mutant else 0), 'unexpected_exit:' + name)
            if not mutant:
                need(json.loads(proc.stdout)['totals']['changed_target_blocks'] > 0, 'nonvacuous_policy_change')
            records.append(dict(command=cmd, returncode=proc.returncode, name=name,
                stdout_sha256=hashlib.sha256(proc.stdout).hexdigest(),
                stderr_sha256=hashlib.sha256(proc.stderr).hexdigest()))
    need((out / 'normal_nominal.stdout').read_bytes() == (out / 'opt_nominal.stdout').read_bytes(),
         'normal_optimized_identity')
    (out / 'commands.json').write_text(json.dumps(records, indent=2) + '\n')
    print(json.dumps(dict(status='passed', commands=len(records), mutants=len(MUTANTS),
                         normal_optimized_identical=True), sort_keys=True))


if __name__ == '__main__':
    main()
