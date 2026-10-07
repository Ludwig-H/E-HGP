#!/usr/bin/env python3
"""Targeted host-only probes, synthetic data; no GPU, sanitizer or LiDAR."""
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
M2=ROOT/'microbancs/mes_m2_feuille'
V11=ROOT.parent/'morsehgp3D_v11'
REPO=ROOT.parent


def run(args):
    p=subprocess.run([str(x) for x in args],capture_output=True,text=True)
    if p.returncode:
        raise RuntimeError(json.dumps({'args':[str(x) for x in args],'exit':p.returncode,'stderr':p.stderr}))
    return p.stdout


manifest=json.loads((HERE/'MANIFEST.json').read_text())


def verify_sources():
    for relative, expected in manifest['sources'].items():
        actual=hashlib.sha256((REPO/relative).read_bytes()).hexdigest()
        if actual!=expected:
            raise RuntimeError('Source changed; no qualification for pinned input: '+relative)
    for name in ['run.py','probe.cpp']:
        if hashlib.sha256((HERE/name).read_bytes()).hexdigest()!=manifest['receipts'][name]:
            raise RuntimeError('Audit probe changed since manifest: '+name)


def dependency_closure(path):
    # -MMD excludes system headers. Reject any unpinned local dependency.
    deptext=path.read_text().replace('\\\n',' ')
    paths=[Path(p).resolve() for p in shlex.split(deptext.split(':',1)[1])]
    result=[]
    for p in paths:
        relative=str(p.relative_to(REPO))
        if p==HERE/'probe.cpp':
            expected=manifest['receipts']['probe.cpp']
        elif relative in manifest['sources']:
            expected=manifest['sources'][relative]
        else:
            raise RuntimeError('Unpinned compiled local dependency: '+relative)
        if hashlib.sha256(p.read_bytes()).hexdigest()!=expected:
            raise RuntimeError('Compiled dependency changed: '+relative)
        result.append(relative)
    return sorted(set(result))


def expected_probe_rows():
    result=[]
    for name,status,records,chunks,triples,quadruples in [
        ('sphere32',0,347,82,4960,35960),('span_exact',0,1,1,0,0),('span_plus_one',1,0,0,0,0)]:
        for form in range(2):
            result.append(dict(shape=name,form=form,status=status,records=records,
                chunks=chunks if form==0 else 0,triples=triples if form==0 else 0,
                quadruples=quadruples if form==0 else 0))
    for name in ['baseline','site_out_of_cloud','wrapped_job_begin','zero_k','profile_mismatch',
                 'coordinate_out_of_profile','short_record_population']:
        result.append(dict(dump_case=name,reader_accepted=True))
    return result


verify_sources()
current_head=run(['git','-C',REPO,'rev-parse','HEAD']).strip()
with tempfile.TemporaryDirectory(prefix='mhgp12-leaf-probe-') as td:
    temp=Path(td)
    flags=['g++','-std=c++20','-O2','-Wall','-Wextra','-Werror','-DMHGP12_COORD_BITS=21',
           '-DMHGP11_COORD_BITS=21','-I'+str(M2/'include'),'-I'+str(V11/'src')]
    run(flags+['-MMD','-MF',temp/'probe.d',HERE/'probe.cpp','-o',temp/'probe'])
    closure={'probe':dependency_closure(temp/'probe.d')}
    probe=[json.loads(line) for line in run([temp/'probe',temp]).splitlines()]
    if probe!=expected_probe_rows():
        raise RuntimeError('Probe results differ from explicit pinned expectations')
    run(flags+['-pthread','-MMD','-MF',temp/'identity.d',M2/'host/leaf_identity.cpp','-o',temp/'identity'])
    closure['identity']=dependency_closure(temp/'identity.d')
    outcomes=[]
    # The unsafe pointer/population mutations above are checked by read() only.
    # Do not execute them. These four cases have safe storage and expose the
    # missing semantic admission checks in the actual host identity executable.
    for name in ['baseline','zero_k','profile_mismatch','coordinate_out_of_profile']:
        result=subprocess.run([str(temp/'identity'),str(temp/(name+'.bin'))],capture_output=True,text=True)
        if result.returncode!=0:
            raise RuntimeError('identity result changed: '+result.stderr)
        rows=[json.loads(line) for line in result.stdout.splitlines()]
        forms=[{k:r[k] for k in ['form','resolved','unresolved','identity']} for r in rows]
        expected=[dict(form=form,resolved=0 if name=='zero_k' else 1,
                       unresolved=1 if name=='zero_k' else 0,identity=True) for form in ['j3','coherent']]
        if forms!=expected or any(r['mismatched_counts']!=0 or r['mismatched_emissions']!=0 or r['leaves']!=1 for r in rows):
            raise RuntimeError('Identity results differ from explicit pinned expectations: '+name)
        outcomes.append({'case':name,'exit':result.returncode,'forms':forms})
    verify_sources()
    if run(['git','-C',REPO,'rev-parse','HEAD']).strip()!=current_head:
        raise RuntimeError('Git HEAD changed during audit probe')
    print(json.dumps({'pin':manifest['pin'],'current_head':current_head,'profile':21,
        'source_hashes_verified_before_after':True,'explicit_expectations_verified':True,
        'compiled_local_dependencies':closure,
        'scope':'host_simulation_and_original_reader_no_gpu','probe':probe,'identity':outcomes},indent=2))
