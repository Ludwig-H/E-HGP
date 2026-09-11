#!/usr/bin/env python3
"""Create-only local qualification, compiled snapshots and closed raw captures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent
BOOST = Path('/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include')


def pin(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sources():
    paths = [ROOT / name for name in ('record.py', 'prepare.py', 'import.json')]
    paths += sorted(p for p in (ROOT / 'prototype').rglob('*') if p.is_file())
    return {str(p.relative_to(ROOT)): pin(p) for p in paths}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--mode', choices=('o2', 'san'), required=True)
    args = parser.parse_args()
    if Path(args.out).name != args.out:
        raise RuntimeError('simple output name required')
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    before = sources()
    snapshot = out / 'source_snapshot'
    for name in before:
        target = snapshot / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, target)
    receipt = dict(status='running', mode=args.mode, sources_before=before, commands=[],
                   device_executed=False, GCP_used=False)

    def save():
        (out / 'receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')

    def run(name, argv, expected=0):
        command = dict(name=name, argv=[str(x) for x in argv], expected_exit_code=expected,
                       started_ns=time.time_ns())
        env = os.environ.copy()
        if args.mode == 'san':
            env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
        with (out / (name + '.stdout')).open('wb') as stdout, (out / (name + '.stderr')).open('wb') as stderr:
            process = subprocess.run(command['argv'], stdout=stdout, stderr=stderr, env=env, check=False)
        command.update(exit_code=process.returncode, ended_ns=time.time_ns(),
                       stdout_sha256=pin(out / (name + '.stdout')), stderr_sha256=pin(out / (name + '.stderr')))
        receipt['commands'].append(command)
        save()
        print(name, process.returncode, flush=True)
        if process.returncode != expected:
            raise RuntimeError('unexpected exit: ' + name)

    save()
    try:
        run('compiler', ['g++', '--version'])
        flags = ['-O2'] if args.mode == 'o2' else ['-O1', '-g', '-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        run('compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
            '-DMHGP7_FAKE_DEVICE', '-isystem', BOOST, *flags,
            snapshot / 'prototype/batch_t2_gate.cpp', '-o', out / 'gate'])
        receipt['binary_sha256'] = pin(out / 'gate')
        for fixture in ('line12', 'shell14', 'spatial12'):
            run(fixture, [out / 'gate', '--' + fixture])
        run('unknown', [out / 'gate', '--unknown'], 2)
        run('missing', [out / 'gate'], 2)
        receipt['status'] = 'passed'
    except BaseException as error:
        receipt.update(status='failed', failure=str(error))
        raise
    finally:
        receipt['sources_after'] = sources()
        receipt['sources_stable'] = receipt['sources_after'] == before
        receipt['snapshot_stable'] = all(pin(snapshot / name) == value for name, value in before.items())
        if not receipt['sources_stable'] or not receipt['snapshot_stable']:
            receipt['status'] = 'failed'
        save()
    if receipt['status'] != 'passed':
        raise RuntimeError('source instability')


if __name__ == '__main__':
    main()
