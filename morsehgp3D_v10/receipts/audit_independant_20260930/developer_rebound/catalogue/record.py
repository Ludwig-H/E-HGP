"""New /tmp scalar builds; this receipt only, never the product tree."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

base=Path(__file__).resolve().parent
repo=base.parents[5]
manifest=json.loads((base/'SOURCE_BEFORE.json').read_text())
build=Path(tempfile.mkdtemp(prefix='mhgp10-ext-span-audit-',dir='/tmp'))
commands=[]


def call(name,argv,env=None):
    result=subprocess.run(argv,cwd=base,capture_output=True,env=env)
    (base/(name+'.stdout')).write_bytes(result.stdout)
    (base/(name+'.stderr')).write_bytes(result.stderr)
    commands.append(dict(name=name,argv=argv,shell_display=shlex.join(argv),cwd=str(base),exit_code=result.returncode,
                         env_overrides=({'UBSAN_OPTIONS':env['UBSAN_OPTIONS']} if env else {})))
    (base/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
    if result.returncode:
        raise RuntimeError(name+' failed; captures retained')
    return result.stdout


call('compiler',['g++','--version'])
common=['g++','-std=c++20','-Wall','-Wextra','-Wconversion','-Wshadow',str(base/'span_check.cpp')]
normal=build/'normal';ubsan=build/'ubsan'
call('compile_normal',common+['-O2','-o',str(normal)])
call('compile_ubsan',common+['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=undefined','-o',str(ubsan)])
out=call('normal',[str(normal)])
sanenv=os.environ.copy();sanenv['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
sanout=call('ubsan',[str(ubsan)],sanenv)
if out!=sanout:
    raise RuntimeError('normal/UBSan output mismatch')
for name in ('normal','ubsan'):
    call('judge_'+name,['python3','-B',str(base/'judge.py'),str(base/(name+'.stdout'))])
    call('judge_'+name+'_O',['python3','-B','-O',str(base/'judge.py'),str(base/(name+'.stdout'))])
for row in manifest['sources']:
    data=(base/row['receipt_path']).read_bytes()
    gitblob=subprocess.check_output(['git','show',manifest['commit']+':'+row['git_path']],cwd=repo)
    if data!=gitblob or hashlib.sha256(data).hexdigest()!=row['sha256']:
        raise RuntimeError('source hash mismatch '+row['git_path'])
manifest['source_hashes_stable']=True
(base/'SOURCE_AFTER.json').write_text(json.dumps(manifest,indent=2)+'\n')
result=dict(status='PASS',source_commit=manifest['commit'],source_hashes_stable=True,
            native_scalar_invocations=2,normal_ubsan_identical=True,python_judgments=4,
            native_checks=json.loads(out)['checks'],no_arena_allocation=True,
            geometric_catalogue_realized=False,product_tower_executed=False,performance=False,GCP=False,
            build_directory=str(build),binary_hashes={name:hashlib.sha256(path.read_bytes()).hexdigest()
              for name,path in [('normal',normal),('ubsan',ubsan)]})
(base/'RESULT.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,sort_keys=True))
