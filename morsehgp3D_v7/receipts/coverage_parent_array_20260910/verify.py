"""Portable read-only check of this source/command capture; no binary execution."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parent
    manifest = json.loads((root / 'manifest.json').read_text())
    observed = {path.relative_to(root).as_posix(): sha(path) for path in root.rglob('*')
                if path.is_file() and path.name != 'manifest.json'}
    need(not any(path.is_symlink() for path in root.rglob('*')), 'no symlinks')
    need(observed == manifest, 'portable inventory and hashes')
    receipt = json.loads((root / 'receipt.json').read_text())
    need(receipt['status'] == 'passed' and receipt['sources_stable'] is True and
         receipt['commands'] == 11 and receipt['GCP_used'] is False and receipt['CMake_used'] is False,
         'scope and completion')
    before = json.loads((root / 'sources_before.json').read_text())
    after = json.loads((root / 'sources_after.json').read_text())
    need(before == after == receipt['sources'], 'unchanged source capture')
    need({name: sha(root / 'snapshot' / name) for name in before} == before, 'source bytes')
    need(sha(root / 'record.py') == receipt['recorder_sha256'], 'recorder pin')
    commands = json.loads((root / 'commands.json').read_text())
    expected = {'compiler': 0, 'head': 0, 'dependencies': 0, 'compile_o2': 0,
                'selftest_o2': 0, 'argument_o2': 2, 'compile_san': 0, 'selftest_san': 0,
                'argument_san': 2, 'compile_parent_zero_mutant': 0, 'selftest_parent_zero_mutant': 1}
    need(len(commands) == len(expected) and len({row['name'] for row in commands}) == len(expected), 'command identity')
    for row in commands:
        name = row['name']
        need(name in expected and row['exit_code'] == row['expected_exit_code'] == expected[name], 'exact exit code')
        need(row['finished_ns'] >= row['started_ns'], 'command chronology')
        need(row['stdout_sha256'] == sha(root / (name + '.stdout')) and
             row['stderr_sha256'] == sha(root / (name + '.stderr')), 'command output pins')
        if name in ('selftest_san', 'argument_san'):
            need(row['environment_overrides'] == {
                'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
                'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}, 'active sanitizers and leaks')
    raw = (root / 'selftest_o2.stdout').read_text()
    need(raw == (root / 'selftest_san.stdout').read_text(), 'O2/SAN output agreement')
    match = re.fullmatch(r'full_coverage_certificate checks=(\d+) rejects=(\d+) replay_cuts=30 gamma_cuts=40 '
                         r'allocation_rejects=(\d+) authority=structural_only\n', raw)
    need(match is not None and int(match[1]) >= 710 and int(match[2]) >= 30 and int(match[3]) >= 20,
         'nonvacuity and square K2 extension')
    need(not (root / 'selftest_o2.stderr').read_bytes() and not (root / 'selftest_san.stderr').read_bytes(),
         'no successful-run diagnostics')
    header = Path('morsehgp3D_v7/src/forest/full_coverage_certificate.hpp')
    original = (root / 'snapshot' / header).read_text()
    mutant = root / 'mutant' / header
    old, new = 'out.parents_.push_back(parent);', 'out.parents_.push_back(0);'
    need(original.count(old) == 1 and mutant.read_text() == original.replace(old, new), 'exact sole parent mutant')
    need(sha(mutant) == receipt['mutant_header_sha256'], 'mutant source pin')
    need((root / 'selftest_parent_zero_mutant.stderr').read_text() == receipt['expected_mutant_diagnostic'] ==
         'FAIL arena.parent_value\n', 'causal parent-arena rejection')
    print(json.dumps(dict(status='passed', files=len(manifest), commands=len(commands),
                          checks=int(match[1]), rejects=int(match[2]), replay_cuts=30, gamma_cuts=40,
                          allocation_rejects=int(match[3]), parent_zero_mutant_rejected=True,
                          authority='structural_only', binary_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
