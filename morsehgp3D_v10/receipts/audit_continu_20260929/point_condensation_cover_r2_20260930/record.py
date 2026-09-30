"""Bounded capture: compile Git-frozen head/API sources privately; retain every attempted command."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
REPOSITORY = Path('/workspaces/E-HGP/build/v9-open-worktree')
COMMIT = '8bb4618e5c6c7c2bc8a5d7a1c2e189a4249d0047'
BINARY_DIR = Path('/tmp/mhgp10-condensation-cover-r2-binaries-20260930.1rEhq1OC')
NAMES = ('head/head.cpp','head/head.hpp','points/dendrogram.cpp','points/dendrogram.hpp',
         'core/status.hpp','core/types.hpp','core/reasons.def')


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name, text):
    with (ROOT/name).open('x') as stream:
        stream.write(text)


paths = [ROOT/'source'/name for name in NAMES] + [
    ROOT/name for name in ('probe.cpp','materialized.hpp','materialize.py','check.py','record.py',
                           'historical/internal_k3.json','historical/internal_k3.u32le',
                           'historical/receipt.json','reference/frontier_core.py','reference/hgp10_ref.py')]
before = {str(p.relative_to(ROOT)):sha(p) for p in paths}
freeze_utc = now()
save('sources_before.json',json.dumps(dict(source_frozen_utc=freeze_utc,sha256=before),
                                    sort_keys=True,indent=2)+'\n')
compiler = Path(shutil.which('g++')).resolve()
compiler_hash = sha(compiler)
commands, binaries, errors = [], {}, []


def capture(name, argv, expected=0, timeout=30):
    t = time.monotonic()
    started = now()
    result = subprocess.run(argv,cwd=ROOT,text=True,capture_output=True,check=False,timeout=timeout,
                            env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
    save(name+'.stdout',result.stdout)
    save(name+'.stderr',result.stderr)
    row = dict(name=name,argv=argv,started_utc=started,ended_utc=now(),
               exit_code=result.returncode,expected_exit_code=expected,
               stdout_sha256=sha(ROOT/(name+'.stdout')),stderr_sha256=sha(ROOT/(name+'.stderr')),
               wall_seconds=time.monotonic()-t)
    commands.append(row)
    if result.returncode != expected:
        raise ValueError('unexpected return code: '+name)
    return result


try:
    for name in NAMES:
        blob = subprocess.run(['git','show',COMMIT+':morsehgp3D_v10/src/'+name],cwd=REPOSITORY,
                              capture_output=True,check=True).stdout
        if hashlib.sha256(blob).hexdigest()!=before['source/'+name]:
            raise ValueError('not exact Git blob: '+name)
    capture('compiler',[str(compiler),'--version'])
    for python_mode, flags in (('normal',[]),('optimized',['-O'])):
        m=capture('materialize_'+python_mode,[sys.executable,'-B',*flags,str(ROOT/'materialize.py')])
        materialized=json.loads(m.stdout)
        if materialized['header']!=(ROOT/'materialized.hpp').read_text() or materialized['status']!='EXACT_HISTORICAL_COVER_MATERIALIZATION':
            raise ValueError('materialization header differs')
    for stem, source, judge, extra in (('cover','probe.cpp','check.py',[]),):
        for mode in ('normal','ubsan'):
            binary = BINARY_DIR/(stem+'_'+mode)
            flags = ['-O2'] if mode=='normal' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
            argv = [str(compiler),'-std=c++20',*flags,'-Wall','-Wextra','-Wpedantic','-Werror',
                    '-I'+str(ROOT/'source'),'-I'+str(ROOT),str(ROOT/source),str(ROOT/'source/head/head.cpp'),
                    str(ROOT/'source/points/dendrogram.cpp'),'-o',str(binary)]
            compiled = capture('compile_'+stem+'_'+mode,argv)
            if compiled.stdout or compiled.stderr:
                raise ValueError('unexpected compile diagnostic')
            binary_before = sha(binary)
            result = capture('native_'+stem+'_'+mode,[str(binary)],timeout=5)
            binary_after = sha(binary)
            binaries[str(binary)] = dict(before=binary_before,after=binary_after)
            if binary_before != binary_after or result.stderr:
                raise ValueError('native diagnostics or binary changed')
            for python_mode, flags in (('normal',[]),('optimized',['-O'])):
                name = 'judge_'+stem+'_'+mode+'_'+python_mode
                argv = [sys.executable,'-B',*flags,str(ROOT/judge),
                        str(ROOT/('native_'+stem+'_'+mode+'.stdout')),*extra]
                judged = capture(name,argv)
                observation = json.loads(judged.stdout)
                if observation['status']!='HISTORICAL_COVER_SCORE_DIFFERENCE_CONFIRMED' or judged.stderr:
                    raise ValueError('oracle did not confirm expected bounded observation')
    for stem in ('cover',):
        if (ROOT/('native_'+stem+'_normal.stdout')).read_bytes()!=(
                ROOT/('native_'+stem+'_ubsan.stdout')).read_bytes():
            raise ValueError('normal/UBSan outputs differ')
except Exception as error:
    errors.append(dict(type=type(error).__name__,error=str(error)))
after = {str(p.relative_to(ROOT)):sha(p) for p in paths}
compiler_after = sha(compiler)
closed = not errors and before==after and compiler_hash==compiler_after
save('sources_after.json',json.dumps(dict(closed_observation_utc=now(),sha256=after),
                                   sort_keys=True,indent=2)+'\n')
receipt = dict(schema='mhgp10_historical_cover_condensation_r2_v1',
               status='HISTORICAL_COVER_SCORE_DIFFERENCE_CONFIRMED' if closed else 'FAIL',
               source_commit=COMMIT,source_frozen_utc=freeze_utc,closed_observation_utc=now(),
               sources_before=before,sources_after=after,compiler=str(compiler),
               compiler_sha256_before=compiler_hash,compiler_sha256_after=compiler_after,
               source_scope='exact Git head blobs; historical native cover translation + independent Gamma3; no new generator',
               binary_hashes=binaries,commands=commands,errors=errors,
               native_invocations=sum(row['name'].startswith('native_') for row in commands),
               oracle_invocations=sum(row['name'].startswith('judge_') for row in commands),
               GCP_used=False,production_sources_modified=False,
               native_generator_invocations=0,historical_export_origin='morsehgp3D_v10/receipts/development_frontier_precision_20260930/qualification/native_normal/internal_k3.json',
               limitations=['historical export used, not a new generator execution',
                            'mcs1 is only an API positive control, not a supported sklearn parameter',
                            'system headers/libraries are not bundled',
                            'binary files remain external; this receipt records historical executions'])
save('receipt.json',json.dumps(receipt,sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(status=receipt['status'],native_invocations=receipt['native_invocations'],
                      oracle_invocations=receipt['oracle_invocations'],errors=errors),sort_keys=True))
raise SystemExit(0 if closed else 1)

