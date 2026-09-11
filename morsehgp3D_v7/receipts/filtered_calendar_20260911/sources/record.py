#!/usr/bin/env python3
"""Small source-frozen O2/SAN gate capture, no packaging or cloud framework."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

BASE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, row):
    with path.open('x') as stream:
        json.dump(row, stream, indent=2, sort_keys=True)
        stream.write('\n')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--san', action='store_true')
    args = parser.parse_args()
    if not args.out.isidentifier():
        raise ValueError('fresh capture name')
    out = BASE / args.out
    out.mkdir()
    sources = ['filtered_calendar.hpp', 'gate.cpp', 'README.md', 'record.py']
    before = {name: sha(BASE / name) for name in sources}
    for name in sources:
        shutil.copyfile(BASE / name, out / name)
    save(out / 'sources_before.json', before)
    flags = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    flags += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.san else ['-O2']
    environment = dict(os.environ)
    if args.san:
        environment.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    commands, status, error = [], 'failed', None
    binary = out / 'gate'
    try:
        for name, argv, expected in [('compile', [*flags, str(out / 'gate.cpp'), '-o', str(binary)], 0),
             ('selftest', [str(binary), '--selftest'], 0), ('unknown', [str(binary), '--unknown'], 2),
             ('missing', [str(binary)], 2)]:
            with (out / (name + '.stdout')).open('xb') as so, (out / (name + '.stderr')).open('xb') as se:
                run = subprocess.run(argv, stdout=so, stderr=se, check=False, env=environment)
            row = dict(name=name, argv=argv, exit_code=run.returncode, expected_exit=expected,
                       stdout_sha256=sha(out / (name + '.stdout')), stderr_sha256=sha(out / (name + '.stderr')))
            commands.append(row)
            save(out / (name + '.command.json'), row)
            print(name, run.returncode, flush=True)
            if run.returncode != expected or (out / (name + '.stderr')).read_bytes():
                raise ValueError('gate process/diagnostics: ' + name)
            if name != 'selftest' and (out / (name + '.stdout')).read_bytes():
                raise ValueError('unexpected success payload')
        status = 'passed'
    except BaseException as exc:
        error = type(exc).__name__ + ': ' + str(exc)
    finally:
        after = {name: sha(BASE / name) for name in sources}
        copied = {name: sha(out / name) for name in sources}
        if before != after or before != copied:
            status, error = 'failed', 'source instability'
        save(out / 'sources_after.json', after)
        save(out / 'receipt.json', dict(status=status, error=error, commands=commands, source_stable=before == after == copied,
             binary_sha256=sha(binary) if binary.is_file() else None, sanitizer=args.san,
             sanitizer_environment={name: environment[name] for name in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')} if args.san else {},
             GCP_used=False, geometry_qualified=False))
    print(json.dumps(dict(status=status, error=error, output=str(out))))
    if status != 'passed':
        raise SystemExit(1)


if __name__ == '__main__':
    main()
