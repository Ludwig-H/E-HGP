"""Bounded private native capture; prints JSON, writes only compiler binaries."""
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT=Path(__file__).resolve().parent
REPO=Path('/workspaces/E-HGP/build/v9-open-worktree')
COMMIT='8bb4618e5c6c7c2bc8a5d7a1c2e189a4249d0047'
BINS=Path('/tmp/mhgp10-point-plateau-binaries-20260930.vQNNp8mf')
SOURCES=('head/head.cpp','head/head.hpp','points/dendrogram.cpp','points/dendrogram.hpp',
         'core/status.hpp','core/types.hpp','core/reasons.def')
FILES=['source/'+name for name in SOURCES]+['PROTOCOL.txt','probe.cpp','judge.py','record.py']


def h(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def now(): return dt.datetime.now(dt.timezone.utc).isoformat()


def main():
    before={name:h(ROOT/name) for name in FILES}; frozen=now(); commands=[]; errors=[]; binaries={}
    compiler=Path(shutil.which('g++')).resolve(); compiler_before=h(compiler)
    def run(name,argv):
        start=now(); ns=time.monotonic_ns()
        result=subprocess.run(argv,cwd=ROOT,text=True,capture_output=True,check=False,timeout=30,
                              env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
        commands.append(dict(name=name,argv=argv,start_utc=start,end_utc=now(),start_ns=ns,
                             end_ns=time.monotonic_ns(),code=result.returncode,
                             stdout=result.stdout,stderr=result.stderr))
        if result.returncode: raise ValueError('command failed: '+name)
        return result
    try:
        for name in SOURCES:
            blob=subprocess.run(['git','show',COMMIT+':morsehgp3D_v10/src/'+name],cwd=REPO,
                                capture_output=True,check=True).stdout
            if hashlib.sha256(blob).hexdigest()!=before['source/'+name]: raise ValueError('Git blob: '+name)
        run('compiler',[str(compiler),'--version'])
        for mode in ('normal','ubsan'):
            binary=BINS/('plateau_'+mode)
            flags=['-O2'] if mode=='normal' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
            argv=[str(compiler),'-std=c++20',*flags,'-Wall','-Wextra','-Wpedantic','-Werror',
                  '-I'+str(ROOT/'source'),str(ROOT/'probe.cpp'),str(ROOT/'source/head/head.cpp'),
                  str(ROOT/'source/points/dendrogram.cpp'),'-o',str(binary)]
            result=run('compile_'+mode,argv)
            if result.stdout or result.stderr: raise ValueError('compile diagnostic')
            pin=h(binary); result=run('native_'+mode,[str(binary)])
            binaries[mode]=dict(path=str(binary),before=pin,after=h(binary))
            if binaries[mode]['before']!=binaries[mode]['after'] or result.stderr:
                raise ValueError('native diagnostic/pin')
    except Exception as exc:
        errors.append(dict(type=type(exc).__name__,error=str(exc)))
    after={name:h(ROOT/name) for name in FILES}
    compiler_after=h(compiler)
    native=[c for c in commands if c['name'].startswith('native_')]
    ok=not errors and before==after and compiler_before==compiler_after and len(native)==2
    if ok and native[0]['stdout']!=native[1]['stdout']:
        errors.append(dict(type='ValueError',error='normal/UBSan captures differ')); ok=False
    print(json.dumps(dict(schema='mhgp10.point_plateau.native.v1',status='CAPTURED' if ok else 'FAIL',
                         source_commit=COMMIT,source_frozen_utc=frozen,closed_utc=now(),
                         sources_before=before,sources_after=after,compiler=str(compiler),
                         compiler_before=compiler_before,compiler_after=compiler_after,
                         binary_hashes=binaries,commands=commands,errors=errors,
                         native_invocations=len(native),GCP_used=False,shared_sources_modified=False),
                     sort_keys=True,indent=2))
    return 0 if ok else 1


if __name__=='__main__':
    raise SystemExit(main())
