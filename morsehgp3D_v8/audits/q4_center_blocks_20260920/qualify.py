#!/usr/bin/env python3
"""Record fresh builds and independent gates, with failed attempts preserved."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    capture = BASE/sys.argv[1]
    if capture.exists():
        raise RuntimeError('Refusing to overwrite a qualification')
    capture.mkdir()
    build = BASE/'.build'/capture.name
    build.mkdir(parents=True)
    names = ('center_blocks.cpp','domain_gate.py','oracle_gate.py','fixtures.py','qualify.py')
    sources = {name:sha(BASE/name) for name in names}
    flags = ['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-Wconversion','-Wsign-conversion','-Wshadow']
    release, sanitize = build/'release', build/'sanitize'
    commands = [
        ['g++','--version'], ['clang++','--version'],
        ['g++',*flags,'-O2',str(BASE/'center_blocks.cpp'),'-o',str(release)],
        ['clang++',*flags,'-O1','-g','-fno-omit-frame-pointer','-fsanitize=address,undefined',str(BASE/'center_blocks.cpp'),'-o',str(sanitize)],
        ['python3',str(BASE/'domain_gate.py')], ['python3','-O',str(BASE/'domain_gate.py')],
        ['python3',str(BASE/'oracle_gate.py'),str(release),str(capture/'oracle_release.json.gz')],
        ['python3','-O',str(BASE/'oracle_gate.py'),str(release),str(capture/'oracle_optimized.json.gz')],
        ['python3',str(BASE/'oracle_gate.py'),str(sanitize),str(capture/'oracle_sanitize.json.gz')],
    ]
    manifest = dict(status='started', sources=sources, commands=[])
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    manifest['sanitizer_options'] = {name:environment[name] for name in ('ASAN_OPTIONS','UBSAN_OPTIONS')}
    for command in commands:
        record = dict(command=command)
        try:
            child = subprocess.run(command, capture_output=True, text=True, env=environment, timeout=240)
            record.update(returncode=child.returncode, stdout=child.stdout, stderr=child.stderr)
        except BaseException as error:
            record.update(returncode=None, error=repr(error), stdout=str(getattr(error,'stdout','')),stderr=str(getattr(error,'stderr','')))
        manifest['commands'].append(record)
        (capture/'MANIFEST.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
        if record['returncode'] != 0:
            raise RuntimeError('Qualification failed; first failure preserved')
        print('PASS '+str(len(manifest['commands']))+'/'+str(len(commands)),flush=True)
    if sources != {name:sha(BASE/name) for name in names}:
        raise RuntimeError('Source changed during qualification')
    manifest.update(status='completed', binary_sha256={str(p.relative_to(BASE)):sha(p) for p in (release,sanitize)},
                    oracle_captures={p.name:sha(p) for p in capture.glob('*.json.gz')})
    (capture/'MANIFEST.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    (capture/'COMPLETION.json').write_text(json.dumps(dict(status='completed',manifest_sha256=sha(capture/'MANIFEST.json')),sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
