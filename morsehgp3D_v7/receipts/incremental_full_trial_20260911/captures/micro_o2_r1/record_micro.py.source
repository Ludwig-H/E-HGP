#!/usr/bin/env python3
"""Paired private mono-thread allocation/residence micro, n200/400/800 only."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def pins(path: Path):
    return {p.relative_to(path).as_posix(): sha(p) for p in sorted(path.rglob('*')) if p.is_file()}

def main() -> None:
    if len(sys.argv) != 2: raise RuntimeError('new output name required')
    out = HERE / sys.argv[1]; out.mkdir(exist_ok=False)
    for kind in ('baseline', 'candidate'):
        shutil.copytree(HERE / kind, out / 'source' / kind)
    # Reuse the complete historical measurement script, without inheriting its
    # old results. Its new/delete epoch measures requested C++ bytes, not RSS.
    origin = ROOT / 'morsehgp3D_v7/receipts/full_tower_residence_20260910/capture/probe.cpp'
    shutil.copyfile(origin, out / 'source/micro.cpp')
    shutil.copyfile(Path(__file__), out / 'record_micro.py.source')
    before = pins(out / 'source')
    (out / 'sources_before.json').write_text(json.dumps(before, indent=2, sort_keys=True) + '\n')
    commands = []; status, error = 'failed', None
    def run(name, argv):
        start = time.time_ns()
        with (out / (name + '.stdout')).open('wb') as so, (out / (name + '.stderr')).open('wb') as se:
            result = subprocess.run(argv, stdout=so, stderr=se, check=False)
        commands.append(dict(name=name, argv=argv, started_ns=start, ended_ns=time.time_ns(), exit_code=result.returncode,
            stdout_sha256=sha(out / (name + '.stdout')), stderr_sha256=sha(out / (name + '.stderr'))))
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        print(name, result.returncode, flush=True)
        if result.returncode != 0: raise RuntimeError('command_failed:' + name)
    comparison = []
    try:
        for kind in ('baseline', 'candidate'):
            run('compile_' + kind, ['g++', '-O2', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-pthread', '-isystem', str(ROOT / 'build/v7_boost_gate/extracted/usr/include'),
                '-I', str(out / 'source' / kind), '-MMD', '-MF', str(out / (kind + '.d')),
                str(out / 'source/micro.cpp'), '-o', str(out / kind)])
            run('run_' + kind, ['/usr/bin/time', '-v', str(out / kind)])
        def rows(kind):
            result = []
            for line in (out / ('run_' + kind + '.stdout')).read_text().splitlines():
                if line.startswith('n='): result.append(dict(field.split('=', 1) for field in line.split()))
            if [r['n'] for r in result] != ['200', '400', '800']: raise RuntimeError('micro_sizes:' + kind)
            return result
        for a, b in zip(rows('baseline'), rows('candidate')):
            stable = ('n', 'balls', 'nodes', 'parents', 'contributions', 'populations', 'physical_digest')
            if any(a[k] != b[k] for k in stable): raise RuntimeError('physical_micro_mismatch')
            comparison.append(dict(n=int(a['n']), matched_fields=list(stable), all_matched=True,
                baseline=a, candidate=b, shared_host=True, speedup_claim=False))
        (out / 'comparison.json').write_text(json.dumps(comparison, indent=2) + '\n')
        status = 'passed'
    except BaseException as exc: error = str(exc)
    after = pins(out / 'source')
    (out / 'sources_after.json').write_text(json.dumps(after, indent=2, sort_keys=True) + '\n')
    if before != after: status, error = 'failed', 'source_changed'
    (out / 'receipt.json').write_text(json.dumps(dict(status=status, error=error, sources_stable=before == after,
        commands=len(commands), modes='cache_default_mono', sizes=[200,400,800], shared_host=True,
        public_status='not_claimed', contract_qualified=False, GCP_used=False), indent=2) + '\n')
    if status != 'passed': raise RuntimeError(error)

if __name__ == '__main__': main()
