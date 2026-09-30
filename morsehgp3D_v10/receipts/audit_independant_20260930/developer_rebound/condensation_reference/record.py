"""Freeze published sources, build two private tiny probes, retain captures."""
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import check
import reference as r

ROOT = Path(__file__).resolve().parent
PIN = '33fcb53a0'


def digest(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def command(args, stdin=None):
    result = subprocess.run(args,input=stdin,text=True,capture_output=True)
    if result.returncode:
        raise ValueError(dict(argv=args,returncode=result.returncode,stdout=result.stdout,stderr=result.stderr))
    return result


def main():
    r.require(not (ROOT/'release.stdout').exists(), 'refuse overwriting captured run')
    work = Path(tempfile.mkdtemp(prefix='mhgp10-cohort-reference-20260930-'))
    source = work/'src'
    source.mkdir()
    tracked = ['head/head.cpp','head/head.hpp','points/dendrogram.cpp','points/dendrogram.hpp',
               'core/types.hpp','core/status.hpp','core/reasons.def']
    blobs = {}
    for rel in tracked:
        p = ROOT/'observed'/'src'/rel
        p.parent.mkdir(parents=True,exist_ok=True)
        data = subprocess.run(['git','show',PIN+':morsehgp3D_v10/src/'+rel],capture_output=True,check=True).stdout
        p.write_bytes(data)
        q = source/rel
        q.parent.mkdir(parents=True,exist_ok=True)
        q.write_bytes(data)
        blobs[rel] = digest(q)
    shutil.copyfile(ROOT/'probe.cpp',source/'probe.cpp')
    compiler = shutil.which('clang++')
    r.require(compiler is not None, 'clang++ required')
    compile_files = [source/'probe.cpp',source/'head/head.cpp',source/'points/dendrogram.cpp']
    build = dict(pin=PIN,workdir=str(work),compiler=compiler,compiler_sha256=digest(compiler),
                 compiler_version=command([compiler,'--version']).stdout,source_sha256=blobs,variants={})
    binaries = {}
    for name, extra in [('release',['-O2']),('ubsan',['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'])]:
        flags = ['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(source)]+extra
        dependencies = set()
        for i, f in enumerate(compile_files):
            args = [compiler]+flags+['-M','-MT','probe',str(f)]
            out = command(args)
            (ROOT/(name+'_dependencies_%d.stdout'%i)).write_text(out.stdout)
            # These private paths contain no whitespace.
            dependencies.update(out.stdout.replace('\\\n',' ').split(':',1)[1].split())
        before = {p:digest(p) for p in sorted(dependencies)}
        binary = work/name
        args = [compiler]+flags+[str(f) for f in compile_files]+['-o',str(binary)]
        out = command(args)
        (ROOT/(name+'_compile.stdout')).write_text(out.stdout)
        (ROOT/(name+'_compile.stderr')).write_text(out.stderr)
        r.require(before == {p:digest(p) for p in before}, 'compile dependency changed')
        ldd = command(['ldd',str(binary)])
        (ROOT/(name+'_ldd.stdout')).write_text(ldd.stdout)
        libs = re.findall(r'(?:=>\s+)?(/\S+)\s+\(',ldd.stdout)
        runtimes = {p:digest(p) for p in libs}
        build['variants'][name] = dict(argv=args,dependencies=before,binary=str(binary),
                                       binary_sha256=digest(binary),runtimes=runtimes)
        binaries[name] = binary
    (ROOT/'build_receipt.json').write_text(json.dumps(build,sort_keys=True,indent=2)+'\n')
    rows = list(check.cases())
    text = check.input_text(rows)
    (ROOT/'native.stdin').write_text(text)
    (ROOT/'case_identity.json').write_text(json.dumps([key for key,d in rows],indent=2)+'\n')
    for name,binary in binaries.items():
        out = command([str(binary)],text)
        (ROOT/(name+'.stdout')).write_text(out.stdout)
        (ROOT/(name+'.stderr')).write_text(out.stderr)
    d = next(d for name,d,_,_ in r.fixtures() if name == 'cohort_at_geometric_split_API')
    bad = r.expand(d,flatten=False)
    mutant_input = check.input_text([(('plateau_mutant',2,2,'unflattened',False,'eom'),bad)])
    (ROOT/'plateau_mutant.stdin').write_text(mutant_input)
    out = command([str(binaries['ubsan'])],mutant_input)
    (ROOT/'plateau_mutant.stdout').write_text(out.stdout)
    (ROOT/'plateau_mutant.stderr').write_text(out.stderr)
    for name,v in build['variants'].items():
        r.require(v['binary_sha256'] == digest(v['binary']) and
                  v['dependencies'] == {p:digest(p) for p in v['dependencies']} and
                  v['runtimes'] == {p:digest(p) for p in v['runtimes']}, 'execution closure drift')
    reports = []
    for name,extra in [('normal',[]),('optimized',['-O'])]:
        args = [sys.executable,'-B']+extra+[str(ROOT/'check.py')]
        out = command(args)
        (ROOT/(name+'_receipt.json')).write_text(out.stdout)
        reports.append(json.loads(out.stdout))
    r.require(reports[0] == reports[1], 'normal/-O mismatch')
    print(json.dumps(dict(status=reports[0]['status'],exact=reports[0]['exact_condensations'],
                         native=reports[0]['native'],workdir=str(work))))


if __name__ == '__main__':
    main()
