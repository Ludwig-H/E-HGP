#!/usr/bin/env python3
"""Strict paired finite qualification, private overlays and all failures retained."""
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

def pins(path: Path) -> dict[str, str]:
    return {p.relative_to(path).as_posix(): sha(p) for p in sorted(path.rglob('*')) if p.is_file()}

def main() -> None:
    if len(sys.argv) not in (2, 3) or (len(sys.argv) == 3 and sys.argv[2] != '--san'):
        raise RuntimeError('name [--san]')
    san = len(sys.argv) == 3
    out = HERE / sys.argv[1]
    out.mkdir(exist_ok=False)
    commands = []
    status, error = 'failed', None
    def command(name: str, argv: list[str], expected: int = 0) -> None:
        start = time.time_ns()
        env = dict(os.environ)
        if san:
            env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        with (out / (name + '.stdout')).open('wb') as stdout, (out / (name + '.stderr')).open('wb') as stderr:
            result = subprocess.run(argv, stdout=stdout, stderr=stderr, env=env, check=False)
        commands.append(dict(name=name, argv=argv, started_ns=start, ended_ns=time.time_ns(),
            exit_code=result.returncode, stdout_sha256=sha(out / (name + '.stdout')),
            stderr_sha256=sha(out / (name + '.stderr')),
            sanitizer_environment={key: env[key] for key in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')} if san else {}))
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        print(name, result.returncode, flush=True)
        if result.returncode != expected:
            raise RuntimeError('unexpected_exit:' + name)
    source_pins = {}
    try:
        for kind in ('baseline', 'candidate'):
            shutil.copytree(HERE / kind, out / 'source' / kind)
            header = out / 'source' / kind / 'morsehgp3D_v7/src/forest/full_ball_tower.hpp'
            original = header.read_text()
            old = 'if (kmax > 1 && !geometry_threads) resolver_cache.configure(ix.upos.size());'
            new = 'if (kmax > 1 && !geometry_threads && incremental_full_test::cache_enabled) resolver_cache.configure(ix.upos.size());'
            if original.count(old) != 1:
                raise RuntimeError('cache_test_only_hook')
            header.write_text(original.replace(old, new))
        shutil.copyfile(HERE / 'qualification.cpp', out / 'source/qualification.cpp')
        shutil.copyfile(Path(__file__), out / 'record_qualification.py.source')
        source_pins = pins(out / 'source')
        (out / 'sources_before.json').write_text(json.dumps(source_pins, indent=2, sort_keys=True) + '\n')
        flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if san else ['-O2', '-DNDEBUG']
        for kind in ('baseline', 'candidate'):
            base = out / 'source' / kind / 'morsehgp3D_v7'
            command('compile_' + kind, ['g++', *flags, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-pthread', '-isystem', str(ROOT / 'build/v7_boost_gate/extracted/usr/include'),
                '-DMHGP7_FULL_HEADER="' + str(base / 'src/forest/full_ball_tower.hpp') + '"',
                '-DMHGP7_GATE_SOURCE="' + str(base / 'tests/full_ball_tower_gate.cpp') + '"',
                '-MMD', '-MF', str(out / (kind + '.d')), str(out / 'source/qualification.cpp'), '-o', str(out / kind)])
            for mode in ('cache', 'no_cache', 'static1', 'static4'):
                command(kind + '_' + mode, [str(out / kind), mode, str(out / (kind + '_' + mode + '.physical'))])
            command(kind + '_bad_arguments', [str(out / kind)], 2)
        for mode in ('cache', 'no_cache', 'static1', 'static4'):
            for suffix in ('physical', 'stdout', 'stderr'):
                if (out / ('baseline_' + mode + '.' + suffix)).read_bytes() != (out / ('candidate_' + mode + '.' + suffix)).read_bytes():
                    raise RuntimeError('paired_divergence:' + mode + ':' + suffix)
        status = 'passed'
    except BaseException as exc:
        error = str(exc)
    finally:
        after = pins(out / 'source')
        (out / 'sources_after.json').write_text(json.dumps(after, indent=2, sort_keys=True) + '\n')
        stable = source_pins == after
        if not stable:
            status, error = 'failed', 'source_changed'
        (out / 'receipt.json').write_text(json.dumps(dict(status=status, error=error, sources_stable=stable,
            sanitizer=san, commands=len(commands), public_status='not_claimed', GCP_used=False), indent=2) + '\n')
    if status != 'passed':
        raise RuntimeError(error)

if __name__ == '__main__':
    main()
