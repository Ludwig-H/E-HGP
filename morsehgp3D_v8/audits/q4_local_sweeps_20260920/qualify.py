#!/usr/bin/env python3
"""Fresh builds; no pinned build or failed capture is overwritten."""
import os
import sys
from evidence import BASE,execute,pins,sha,write


def main():
    folder=BASE/sys.argv[1]
    if folder.exists():raise RuntimeError('Capture already exists')
    folder.mkdir()
    build=BASE/'.build'/folder.name
    build.mkdir(parents=True)
    flags=['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-Wconversion','-Wsign-conversion','-Wshadow']
    release,sanitize=build/'release',build/'sanitize'
    commands=[['g++','--version'],['clang++','--version'],
        ['g++',*flags,'-O2',str(BASE/'local_sweeps.cpp'),'-o',str(release)],
        ['clang++',*flags,'-O1','-g','-fno-omit-frame-pointer','-fsanitize=address,undefined',str(BASE/'local_sweeps.cpp'),'-o',str(sanitize)],
        ['python3','-B',str(BASE/'local_gate.py')],['python3','-B','-O',str(BASE/'local_gate.py')],
        ['python3','-B',str(BASE/'verify_local.py'),str(release),str(folder/'release.json.gz')],
        ['python3','-B','-O',str(BASE/'verify_local.py'),str(release),str(folder/'optimized.json.gz')],
        ['python3','-B',str(BASE/'verify_local.py'),str(sanitize),str(folder/'sanitize.json.gz')]]
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    manifest=dict(schema='audit_local_sweeps_qualification_v1',status='started',sources=pins(),
                  scope='independent covered roots, depths and shells; product cascade not run',commands=[],
                  environment={k:env[k] for k in ('PYTHONDONTWRITEBYTECODE','ASAN_OPTIONS','UBSAN_OPTIONS')})
    for command in commands:
        result=execute(command,env)
        manifest['commands'].append(result)
        write(folder/'MANIFEST.json',manifest)
        if result['returncode']!=0:raise RuntimeError('Qualification failed; first failure preserved')
        print(f'PASS {len(manifest["commands"])}/{len(commands)}',flush=True)
    if manifest['sources']!=pins():raise RuntimeError('Source changed during qualification')
    manifest.update(status='completed',binaries={str(p.relative_to(BASE)):sha(p) for p in (release,sanitize)},
                    oracle_captures={p.name:sha(p) for p in folder.glob('*.json.gz')})
    write(folder/'MANIFEST.json',manifest)
    write(folder/'COMPLETION.json',dict(status='completed',manifest_sha256=sha(folder/'MANIFEST.json')))


if __name__=='__main__':main()
