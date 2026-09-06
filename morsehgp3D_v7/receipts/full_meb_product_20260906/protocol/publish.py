#!/usr/bin/env python3
"""Publish closed evidence only; no C++ compilation, Git mutation or GCP."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess

ROOT = Path('/workspaces/E-HGP')
BASE = Path(__file__).resolve().parent
DEST = ROOT / 'morsehgp3D_v7/receipts/full_meb_product_20260906'
CORE = ROOT / 'build/v7_meb_product_qualification_20260906_r3'
EXTRA = ROOT / 'build/v7_meb_product_extra_qualification_20260906/run_r1'
MUTANT = ROOT / 'build/v7_meb_product_mutation_qualification_20260906/run_r1'


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def put(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as out:
        out.write(data)


def encoded(value: object) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def copy(source: Path, dest: Path) -> None:
    require(source.is_file() and not source.is_symlink(), 'regular file: ' + str(source))
    raw = source.read_bytes()
    put(dest, raw)
    require(source.read_bytes() == raw and dest.read_bytes() == raw, 'copy changed: ' + str(source))


def tree(source: Path, dest: Path) -> None:
    for p in sorted(source.rglob('*')):
        rel = p.relative_to(source)
        if p.is_file() and rel.parts[0] not in ('bin', 'tmp'):
            copy(p, dest / rel)


def check(name: str, script: Path, capture: Path, output: Path) -> None:
    outputs = []
    for opt in (False, True):
        label = name + ('_optimized' if opt else '_normal')
        args = ['/usr/bin/python3', '-B'] + (['-O'] if opt else []) + [str(script), str(capture)]
        start = datetime.now(timezone.utc).isoformat()
        proc = subprocess.Popen(args, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                start_new_session=True, env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
        try:
            out, err = proc.communicate(timeout=60)
        except BaseException:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.communicate()
            raise
        put(output / (label + '.stdout'), out)
        put(output / (label + '.stderr'), err)
        closed = False
        try:
            os.killpg(proc.pid, 0)
        except ProcessLookupError:
            closed = True
        else:
            os.killpg(proc.pid, signal.SIGKILL)
        put(output / (label + '.json'), encoded(dict(argv=args, cwd=str(ROOT), pid=proc.pid,
            started=start, ended=datetime.now(timezone.utc).isoformat(), exit_code=proc.returncode,
            process_group_closed=closed, script_sha256=sha(script),
            stdout_sha256=hashlib.sha256(out).hexdigest(), stderr_sha256=hashlib.sha256(err).hexdigest())))
        require(proc.returncode == 0 and not err and closed, 'checker: ' + label)
        outputs.append(out)
    require(outputs[0] == outputs[1], 'normal/-O equality: ' + name)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--core-sha', required=True)
    args = parser.parse_args()
    require(not DEST.exists(), 'publication must be fresh')
    admissions = {'core': (CORE / 'capture', args.core_sha),
                  'extra': (EXTRA, 'a709d26382b55820a6ee268e0aec49098f659869ed75419ba82241ee2f456ac2'),
                  'mutations': (MUTANT, 'dbbe577e7ca392b680844c098b5a4fb122122ea2ccb10cd3629694af4c71044f')}
    for name, (folder, pin) in admissions.items():
        require(sha(folder / 'run.json') == pin, 'run admission: ' + name)
        report = json.loads((folder / 'run.json').read_text())
        require(report['status'] == ('passed' if name == 'core' else 'completed') and
                report['sources_stable'] is True, 'closed source-stable run: ' + name)
    checks = BASE / 'checks'
    checks.mkdir(exist_ok=False)
    for name, folder in [('cmake', CORE / 'capture'), ('extra', EXTRA), ('mutations', MUTANT)]:
        check(name, BASE / ('verify_' + name + '.py'), folder, checks)
    DEST.mkdir(exist_ok=False)
    for name, (folder, _) in admissions.items():
        tree(folder, DEST / name)
    for trial in ('v7_meb_product_qualification_20260906', 'v7_meb_product_qualification_20260906_r2'):
        path = ROOT / 'build' / trial
        tree(path / 'capture', DEST / 'failed_attempts' / trial / 'capture')
        copy(path / 'record.py', DEST / 'failed_attempts' / trial / 'record.py')
    pins = json.loads((CORE / 'capture/sources_before.json').read_text())
    for name, pin in pins.items():
        require(sha(ROOT / name) == pin, 'current core source matches capture: ' + name)
        copy(ROOT / name, DEST / 'core/source_snapshot' / name)
    bindings = {}
    for mode in ('release', 'san'):
        build = CORE / mode
        for name in ('compile_commands.json', 'CMakeCache.txt'):
            copy(build / name, DEST / 'core/compilation' / mode / name)
        deps = {}
        for path in sorted((build / 'CMakeFiles').rglob('*.o.d')):
            rel = path.relative_to(build)
            copy(path, DEST / 'core/compilation' / mode / rel)
            text = path.read_text().replace('\\\n', ' ')
            for part in shlex.split(text.split(':', 1)[1]):
                source = Path(part)
                require(source.is_absolute() and source.is_file(), 'absolute dependency')
                deps[str(source)] = sha(source)
        bindings[mode] = deps
    # Post-capture dependency identities, not a hermetic system-header proof.
    put(DEST / 'core/compilation/dependency_identities_after.json', encoded(bindings))
    tree(checks, DEST / 'checks')
    for name in ('publish.py', 'verify_cmake.py', 'verify_extra.py', 'verify_mutations.py'):
        copy(BASE / name, DEST / 'protocol' / name)
    put(DEST / 'publication.json', encoded(dict(schema='mhgp7-full-meb-product-publication-v1',
        public_status='not_claimed', gcp='not_used', cpp_executed_during_publication=False,
        created=datetime.now(timezone.utc).isoformat(),
        admitted_runs={name: pin for name, (_, pin) in admissions.items()},
        binaries_distributed=False, private_extra_judge_requires_original_path_and_ELF=True,
        portable_verifiers='protocol/verify_cmake.py, verify_extra.py, verify_mutations.py',
        failed_attempts_preserved=2)))
    print(json.dumps({'status': 'copied_and_rejudged', 'destination': str(DEST)}))


if __name__ == '__main__':
    main()
