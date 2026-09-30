"""Fresh isolated builds and native normal/-O consumers; no product build touched."""
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent


def need(ok,text):
    if not ok:
        raise RuntimeError(text)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(argv,name,expected=0):
    result = subprocess.run([str(a) for a in argv],cwd=ROOT,text=True,capture_output=True,timeout=30)
    (ROOT/(name+'.stdout')).write_text(result.stdout)
    (ROOT/(name+'.stderr')).write_text(result.stderr)
    row = {'argv':[str(a) for a in argv],'returncode':result.returncode,'expected':expected,
           'stdout_sha256':sha(ROOT/(name+'.stdout')),'stderr_sha256':sha(ROOT/(name+'.stderr'))}
    commands.append(row)
    need(result.returncode == expected,'command failed: '+name)
    return result.stdout


def deps(text):
    return {(ROOT/p).resolve() for p in text.replace('\\\n',' ').split(':',1)[1].split()}


def dynamic_paths(binary):
    result = subprocess.run(['ldd',str(binary)],text=True,capture_output=True,timeout=10)
    need(result.returncode == 0,'ldd failed')
    return {Path(p).resolve() for p in re.findall(r'(/[^\s()]+)',result.stdout)}


commands = []


def main():
    need(not (ROOT/'capture_receipt.json').exists(),'refuse to overwrite capture')
    # Preserve the C++ frontend argv[0]; resolving clang++ to clang changes
    # implicit linkage even when the executable bytes are identical.
    compiler = Path(shutil.which('clang++'))
    build = Path(tempfile.mkdtemp(prefix='v10-audit-streaming-euler-20260930.',dir='/workspaces/E-HGP/build'))
    version = command([compiler,'--version'],'compiler_version')
    resource = Path(command([compiler,'-print-resource-dir'],'compiler_resource').strip())
    flags = ['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-Wconversion','-Wsign-conversion']
    dep_text = command([compiler,*flags,'-M',ROOT/'probe.cpp'],'dependency_preflight')
    paths = deps(dep_text)|{compiler,Path(sys.executable).resolve(),ROOT/'record.py',ROOT/'check.py'}
    paths |= dynamic_paths(compiler)
    paths |= {p.resolve() for p in resource.rglob('*ubsan*') if p.is_file()}
    before = {str(p):sha(p) for p in sorted(paths)}
    binaries = {}
    variants = {'release':['-O2'],
                'ubsan':['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all'],
                'mutant':['-O2','-DAUDIT_MUTANT_DROP_TIN=1']}
    for name,extra in variants.items():
        binary = build/name
        dep = build/(name+'.d')
        command([compiler,*flags,*extra,'-MD','-MF',dep,ROOT/'probe.cpp','-o',binary],'compile_'+name)
        need(deps(dep.read_text()) <= paths,'compiled dependency escaped initial closure')
        binaries[name] = binary
    after_compile = {str(p):sha(p) for p in sorted(paths)}
    need(before == after_compile,'input changed during compilation')
    output_paths = set(binaries.values())|set(build.glob('*.d'))
    for binary in binaries.values():
        output_paths |= dynamic_paths(binary)
    output_before = {str(p):sha(p) for p in sorted(output_paths)}
    build_report = {'status':'PASS','fresh_build':str(build),'compiler':str(compiler),
                    'compiler_version':version,'commands':commands.copy(),
                    'dependencies_before':before,'dependencies_after':after_compile,
                    'binary_and_runtime_before':output_before,
                    'scope':'isolated helper; no product includes/library/build, no LCA implementation'}
    (ROOT/'build_receipt.json').write_text(json.dumps(build_report,indent=2,sort_keys=True)+'\n')
    for name,opt in (('normal',[]),('optimized',['-O'])):
        command([sys.executable,'-B',*opt,ROOT/'check.py',
                 '--release='+str(binaries['release']),'--ubsan='+str(binaries['ubsan']),
                 '--mutant='+str(binaries['mutant']),'--build-receipt='+str(ROOT/'build_receipt.json'),
                 '--out='+str(ROOT/name)],'consumer_'+name)
    a = json.loads((ROOT/'normal/receipt.json').read_text())
    b = json.loads((ROOT/'optimized/receipt.json').read_text())
    need([a.pop('optimize_flag'),b.pop('optimize_flag')] == [0,1] and a == b,'normal/-O receipts disagree')
    need((ROOT/'normal/release.stdout').read_bytes() == (ROOT/'optimized/release.stdout').read_bytes(),
         'normal/-O native stdout differs')
    final_input = {str(p):sha(p) for p in sorted(paths)}
    output_after = {str(p):sha(p) for p in sorted(output_paths)}
    need(before == final_input and output_before == output_after,'inputs/binaries changed during capture')
    report = {'status':'PASS','commands':commands,'dependencies_before':before,
              'dependencies_after':final_input,'binary_and_runtime_before':output_before,
              'binary_and_runtime_after':output_after,'normal_optimized_equal':True,
              'counts':a['counts'],'refusals':a['refusals'],'mutant_native_returncode':0,
              'mutant_wrong_value':a['mutant_native_global'],'GCP_used':False,
              'scope':build_report['scope']}
    (ROOT/'capture_receipt.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'status':'PASS','counts':a['counts'],'refusals':a['refusals'],
                      'mutant_wrong_native_value':True,'normal_optimized_equal':True,
                      'closed_compilation_dependencies':len(before)},sort_keys=True))


if __name__ == '__main__':
    main()
