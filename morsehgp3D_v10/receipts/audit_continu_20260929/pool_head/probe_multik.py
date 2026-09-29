import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys

if len(sys.argv) != 4:
    raise SystemExit('usage: probe_multik.py TOWER_BINARY V10_SOURCE NEW_OUTPUT_DIRECTORY')
work = Path(sys.argv[3]).resolve()
work.mkdir(parents=True, exist_ok=False)
base = Path(sys.argv[2]).resolve()
spec = importlib.util.spec_from_file_location('gate', base / 'tests/oracle/test_tower_oracle.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
exe = Path(sys.argv[1]).resolve()
pts = [(x, 0, 0) for x in (0,20,22,50,52)]
(work / 'crossing.u32le').write_bytes(b''.join(struct.pack('<III', *p) for p in pts))
argv = [str(exe), str(work / 'crossing.u32le'), '--k=2', '--threads=1', '--dump='+str(work/'crossing.txt')]
run = subprocess.run(argv, capture_output=True, text=True, timeout=10)
if run.returncode:
    raise RuntimeError(run.stderr)
orders = gate.parse(work / 'crossing.txt')
def parts(k,a):
    groups = {}
    o = orders[k]
    for p,v,e in o['points']:
        if e <= a:
            groups.setdefault(gate.top(o['nodes'],v,a,True),[]).append(p[0])
    return sorted(groups.values())
a,b = parts(1,100),parts(2,225)
if [0,20,22] not in a or [20,22,50,52] not in b:
    raise RuntimeError((a,b))
report = dict(source_commit='6206d1d11', binary_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
              argv=argv, code=run.returncode, points=pts, core_k1_r10=a, core_k2_r15=b,
              intersection=[20,22], exclusive_a=[0], exclusive_b=[50,52],
              conclusion='all_orders_all_scales_core_clusters_are_not_laminar')
(work/'crossing_report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
