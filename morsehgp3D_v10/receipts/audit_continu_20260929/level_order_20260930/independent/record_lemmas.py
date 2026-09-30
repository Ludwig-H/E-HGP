"""Capture the same bounded rational model in normal and optimized Python."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
paths = [root/'order_lemmas.py',root/'record_lemmas.py']
before = {path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
commands = []
for mode,flags in [('normal',[]),('optimized',['-O'])]:
    argv = [sys.executable,'-B',*flags,str(root/'order_lemmas.py')]
    result = subprocess.run(argv,capture_output=True,text=True,check=False,timeout=30)
    for stream in ('stdout','stderr'):
        with (root/('lemmas_'+mode+'.'+stream)).open('x') as file:
            file.write(getattr(result,stream))
    commands.append(dict(argv=argv,mode=mode,exit_code=result.returncode))
after = {path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
passed = before==after and all(call['exit_code']==0 for call in commands)
with (root/'lemmas_execution.json').open('x') as file:
    file.write(json.dumps(dict(status='PASS' if passed else 'FAIL',before=before,after=after,
                              commands=commands,native_executions=0,GCP_used=False),sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(status='PASS' if passed else 'FAIL',commands=len(commands)),sort_keys=True))
raise SystemExit(0 if passed else 1)
