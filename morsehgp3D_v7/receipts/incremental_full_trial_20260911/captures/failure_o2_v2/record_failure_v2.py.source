#!/usr/bin/env python3
"""Corrected new/nothrow interposer; original failed SAN and v1 stay untouched."""
from __future__ import annotations
import hashlib
import json
import os
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
    if len(sys.argv) not in (2, 3) or (len(sys.argv) == 3 and sys.argv[2] != '--san'):
        raise RuntimeError('name [--san]')
    san = len(sys.argv) == 3
    out = HERE / sys.argv[1]; out.mkdir(exist_ok=False)
    # Reuse exactly the previously tested hook overlay, not an active header.
    original = HERE / 'failure_o2_r1/source'
    shutil.copytree(original, out / 'source')
    shutil.copyfile(original / 'failure_gate.cpp', out / 'source/failure_gate_v1.cpp')
    shutil.copyfile(HERE / 'failure_gate_v2.cpp', out / 'source/failure_gate_v2.cpp')
    shutil.copyfile(HERE / 'failure_o2_r1/fixtures.txt', out / 'fixtures.txt')
    shutil.copyfile(Path(__file__), out / 'record_failure_v2.py.source')
    before = pins(out / 'source')
    (out / 'sources_before.json').write_text(json.dumps(before, indent=2, sort_keys=True) + '\n')
    commands = []; status, error = 'failed', None
    def run(name, argv, expected=0):
        start = time.time_ns(); env = dict(os.environ)
        if san: env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        with (out / (name + '.stdout')).open('wb') as so, (out / (name + '.stderr')).open('wb') as se:
            result = subprocess.run(argv, stdout=so, stderr=se, env=env, check=False)
        commands.append(dict(name=name, argv=argv, started_ns=start, ended_ns=time.time_ns(), exit_code=result.returncode,
            stdout_sha256=sha(out / (name + '.stdout')), stderr_sha256=sha(out / (name + '.stderr')),
            sanitizer_environment={k: env[k] for k in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')} if san else {}))
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        print(name, result.returncode, flush=True)
        if result.returncode != expected: raise RuntimeError('command_failed:' + name)
    try:
        flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if san else ['-O2', '-DNDEBUG']
        run('compile', ['g++', *flags, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
            '-DMHGP7_FULL_HEADER="' + str(out / 'source/morsehgp3D_v7/src/forest/full_ball_tower.hpp') + '"',
            '-MMD', '-MF', str(out / 'gate.d'), str(out / 'source/failure_gate_v2.cpp'), '-o', str(out / 'gate')])
        run('run', [str(out / 'gate'), str(out / 'fixtures.txt')])
        run('arguments', [str(out / 'gate')], 2)
        status = 'passed'
    except BaseException as exc: error = str(exc)
    after = pins(out / 'source')
    (out / 'sources_after.json').write_text(json.dumps(after, indent=2, sort_keys=True) + '\n')
    if before != after: status, error = 'failed', 'source_changed'
    (out / 'receipt.json').write_text(json.dumps(dict(status=status, error=error, sources_stable=before == after,
        sanitizer=san, commands=len(commands), interposer='v2_includes_nothrow',
        public_status='not_claimed', GCP_used=False), indent=2) + '\n')
    if status != 'passed': raise RuntimeError(error)

if __name__ == '__main__': main()
