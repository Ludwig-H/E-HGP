"""Capture four native batches and their independent exact judgments."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
import inputs

root = Path(__file__).resolve().parent
prototype = Path('/tmp/mhgp10-level-comparator-0930.jpN8t6og')
bins = {name:prototype/binary for name,binary in
        [('normal','normal'),('ubsan','ubsan'),('float_mutant','float_mutant'),('high_mutant','high_mutant')]}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name,content):
    with (root/name).open('x') as stream:
        stream.write(content)


paths = [root/'inputs.py',root/'judge.py',root/'record.py',
         prototype/'level_comparator.cpp',prototype/'PROTOCOL.txt',*bins.values()]
before = {str(path):sha(path) for path in paths}
panel = inputs.panel()
save('requests.json',json.dumps(panel,sort_keys=True)+'\n')
batch = '\n'.join(map(inputs.line,panel))+'\n'
save('requests.stdin',batch)
commands = []
for case,binary in bins.items():
    argv = [str(binary)]
    result = subprocess.run(argv,input=batch,capture_output=True,text=True,check=False,timeout=30,
                            env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
    save(case+'.stdout',result.stdout)
    save(case+'.stderr',result.stderr)
    commands.append(dict(name=case,argv=argv,exit_code=result.returncode,expected_exit_code=0))
    for mode,flags in [('normal',[]),('optimized',['-O'])]:
        name = 'judge_'+case+'_'+mode
        argv = [sys.executable,'-B',*flags,str(root/'judge.py'),str(root/'requests.json'),str(root/(case+'.stdout'))]
        result = subprocess.run(argv,capture_output=True,text=True,check=False,timeout=30)
        save(name+'.stdout',result.stdout)
        save(name+'.stderr',result.stderr)
        expected = 1 if case in ('float_mutant','high_mutant') else 0
        commands.append(dict(name=name,argv=argv,exit_code=result.returncode,expected_exit_code=expected))
after = {str(path):sha(path) for path in paths}
passed = before==after and all(row['exit_code']==row['expected_exit_code'] for row in commands)
save('execution.json',json.dumps(dict(schema='mhgp10_level_comparator_independent_v1',
      status='PASS' if passed else 'FAIL',before=before,after=after,commands=commands,requests=len(panel),
      GCP_used=False,production_sources_modified=False,
      scope='exact comparator/raw multiplication only; native geometric level generation/sort/FULL not tested'),
      sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(status='PASS' if passed else 'FAIL',requests=len(panel),commands=commands),sort_keys=True))
raise SystemExit(0 if passed else 1)
