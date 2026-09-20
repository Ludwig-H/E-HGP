"""Closure helpers for this independent audit and its explicit source reuse."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pins():
    own=('local_sweeps.cpp','local_gate.py','inputs.py','verify_local.py',
         'evidence.py','qualify.py','run.py','read.py')
    old=('center_blocks.cpp','fixtures.py','oracle_gate.py')
    paths=[BASE/p for p in own]+[BASE.parent/'q4_center_blocks_20260920'/p for p in old]
    return {str(p.relative_to(ROOT)):sha(p) for p in paths}


def write(path,value):
    path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')


def execute(command,environment=None,timeout=240):
    start=time.monotonic()
    result=dict(command=command)
    try:
        child=subprocess.run(command,capture_output=True,text=True,env=environment,timeout=timeout)
        result.update(returncode=child.returncode,stdout=child.stdout,stderr=child.stderr)
    except BaseException as error:
        result.update(returncode=None,error=repr(error),stdout=str(getattr(error,'stdout','')),stderr=str(getattr(error,'stderr','')))
    result['process_wall_seconds']=time.monotonic()-start
    return result
