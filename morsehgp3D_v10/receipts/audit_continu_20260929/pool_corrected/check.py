"""Capture short independent construction checks; do not run a full campaign."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time

OUT = Path(__file__).resolve().parent
TMP = Path('/tmp/mhgp10-pool-corrected.pICWsa')
SNAPSHOT = Path('/workspaces/E-HGP/build/v10-audit-independent-20260929/contre_pool_2020/src')
COPY = Path('/workspaces/E-HGP/build/v10-fixes/pool/src/morsehgp3D_v10')
ROOT = OUT.parents[2]
FAULT = Path('/workspaces/E-HGP/build/v10-fixes/pool/build/mhgp10_fault')
keys = [SNAPSHOT/'sched/pool.cpp', SNAPSHOT/'sched/pool.hpp', SNAPSHOT/'core/types.hpp',
        COPY/'src/sched/pool.cpp', COPY/'src/sched/pool.hpp', COPY/'tests/unit/fault_main.cpp',
        COPY/'src/catalogue/generator.cpp', COPY/'src/tower/tower.cpp',
        ROOT/'src/sched/pool.cpp', FAULT, OUT/'thread_create_failure.cpp']
def hashes():
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in keys}
receipt = dict(status='running', scope='corrected_copy_not_integrated', sources_before=hashes(), commands=[])
target = OUT/'receipt.json'
if target.exists():
    raise SystemExit('capture already exists; do not overwrite')
def save():
    target.write_text(json.dumps(receipt,indent=2)+'\n')
save()
def run(name, argv, timeout=10):
    begin = time.monotonic()
    p = subprocess.Popen(argv,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
    timed_out = False
    try:
        stdout,stderr = p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        os.killpg(p.pid,signal.SIGKILL)
        stdout,stderr = p.communicate()
    (OUT/(name+'.stdout')).write_bytes(stdout)
    (OUT/(name+'.stderr')).write_bytes(stderr)
    row=dict(argv=argv,code=p.returncode,timed_out=timed_out,elapsed_s=time.monotonic()-begin,
             stdout_sha256=hashlib.sha256(stdout).hexdigest(),stderr_sha256=hashlib.sha256(stderr).hexdigest())
    receipt['commands'].append(row)
    save()
    if p.returncode or timed_out:
        raise RuntimeError(name+' failed')
try:
    run('allocation_construction',[str(FAULT),'pool_construction'])
    exe = TMP/'thread_create_failure'
    run('compile_thread_create',['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-O1',
        '-I'+str(SNAPSHOT),str(OUT/'thread_create_failure.cpp'),str(SNAPSHOT/'sched/pool.cpp'),
        '-pthread','-ldl','-Wl,--export-dynamic','-o',str(exe)],timeout=30)
    receipt['probe_binary_sha256']=hashlib.sha256(exe.read_bytes()).hexdigest()
    for n in (1,2,3):
        run('thread_create_'+str(n),[str(exe),str(n)])
    receipt['status']='completed'
except BaseException as error:
    receipt['status']='failed'
    receipt['error']=str(error)
    raise
finally:
    receipt['sources_after']=hashes()
    receipt['source_closure']=receipt['sources_before']==receipt['sources_after']
    if not receipt['source_closure']:
        receipt['status']='invalid_source_drift'
    save()
print(json.dumps({k:v for k,v in receipt.items() if k not in ['sources_before','sources_after']},indent=2))
