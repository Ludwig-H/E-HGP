"""Capture paired real sklearn runs once, without a native HGP invocation."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parent
out = root/'sklearn_capture'
out.mkdir(exist_ok=False)
pins = {str(root/name):hashlib.sha256((root/name).read_bytes()).hexdigest()
        for name in ('record_sklearn.py','sklearn_probe.py')}
commands = []
outputs = []
for mode,flags in (('normal',[]),('optimized',['-O'])):
    argv = [sys.executable,'-B',*flags,str(root/'sklearn_probe.py')]
    result = subprocess.run(argv,capture_output=True,text=True,timeout=30,check=False,
                            env={**os.environ,'OMP_NUM_THREADS':'1','OPENBLAS_NUM_THREADS':'1',
                                 'MKL_NUM_THREADS':'1'})
    for stream in ('stdout','stderr'):
        with (out/(mode+'.'+stream)).open('x') as file:
            file.write(getattr(result,stream))
    commands.append(dict(mode=mode,argv=argv,exit_code=result.returncode))
    if result.returncode or result.stderr:
        raise ValueError('paired sklearn failed: '+mode)
    report = json.loads(result.stdout)
    if report['status']!='PASS' or report['actual_fit_calls']!=16 or report['before']!=report['after']:
        raise ValueError('sklearn scope/pins failed')
    outputs.append(report)
after = {path:hashlib.sha256(Path(path).read_bytes()).hexdigest() for path in pins}
if after!=pins:
    raise ValueError('capture source changed')
if outputs[0]['rows']!=outputs[1]['rows'] or outputs[0]['before']!=outputs[1]['before']:
    raise ValueError('paired outputs differ')
with (out/'receipt.json').open('x') as file:
    file.write(json.dumps(dict(status='PASS',commands=commands,before=pins,after=after,
                               unique_configurations=16,actual_fit_calls=32,native_HGP_calls=0,
                               GCP_used=False,preflight_fit_calls=16,
                               scope='actual sklearn on equivalent ultrametric; not native HGP/3D'),
                          sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(status='PASS',actual_fit_calls=32,unique_configurations=16,
                     native_HGP_calls=0,GCP_used=False),sort_keys=True))
