from pathlib import Path
import datetime,hashlib,json,subprocess,sys
p=Path(__file__).resolve().parent
mode=sys.argv[1]
binary=Path('/tmp/mhgp11-core-independent-20261002'+('-ubsan' if mode=='ubsan' else ''))
commands=[]
def call(command,name,expected):
 r=subprocess.run(command,cwd=p,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 (p/(name+'.stdout')).write_bytes(r.stdout);(p/(name+'.stderr')).write_bytes(r.stderr)
 commands.append({'argv':command,'returncode':r.returncode,'expected_returncode':expected,'stdout':name+'.stdout','stderr':name+'.stderr'})
 if r.returncode!=expected:raise RuntimeError('unexpected return code '+name)
 return r
src=p/'sources/morsehgp3D_v11/src'
flags=['-std=c++20','-O1','-Wall','-Wextra','-Wpedantic','-Werror','-DMHGP11_COORD_BITS=18']
if mode=='ubsan':flags += ['-fsanitize=undefined','-fno-sanitize-recover=all']
compile_cmd=['g++',*flags,'-I'+str(src),'probe.cpp',str(src/'core/buffer.cpp'),str(src/'core/ledger.cpp'),'-o',str(binary)]
try:
 call(compile_cmd,'compile_'+mode,0)
 for case in ('budget','baseline','mutable','reset','mutable_fault','reset_fault'):
  call([str(binary),case],mode+'_'+case,42 if case.endswith('_fault') else 0)
finally:
 out={'mode':mode,'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'commands':commands,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest() if binary.exists() else None,'scope':'WIP core source snapshot; StageTimer allocation/termination and bounded Buffer ownership contracts'}
 (p/('RUN_'+mode+'.json')).write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({'mode':mode,'commands':len(commands),'expected_outcomes_observed':True}))
