"""Tiny rational counterexample for a fixed relative ambiguity band and LCA routing."""
from fractions import Fraction
import hashlib
import importlib.util
import json
from math import isqrt
from pathlib import Path
import struct
import subprocess
import sys

if len(sys.argv) != 4:
    raise SystemExit('usage: probe_band.py TOWER_BINARY V10_SOURCE NEW_OUTPUT_DIRECTORY')
exe, base, work = (Path(x).resolve() for x in sys.argv[1:])
work.mkdir(parents=True, exist_ok=False)
spec = importlib.util.spec_from_file_location('gate', base/'tests/oracle/test_tower_oracle.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

def sqrt_fraction(q):
    a, b = isqrt(q.numerator), isqrt(q.denominator)
    if a*a != q.numerator or b*b != q.denominator:
        raise RuntimeError('expected rational radius on the collinear fixture')
    return Fraction(a,b)

def lca(nodes, selected):
    ancestors = []
    v = selected[0]
    while v >= 0:
        ancestors.append(v)
        v = nodes[v][0]
    for v in ancestors:
        valid = True
        for u in selected[1:]:
            while u >= 0 and u != v:
                u = nodes[u][0]
            valid = valid and u == v
        if valid:
            return v
    raise RuntimeError('no common ancestor')

eta, scale = Fraction(1,4), 1000
rows = []
for delta in (-1,1):
    name = 'minus' if delta < 0 else 'plus'
    points = [(x,0,0) for x in (0,8*scale,18*scale+delta)]
    src, dump = work/(name+'.u32le'), work/(name+'.txt')
    src.write_bytes(b''.join(struct.pack('<III',*p) for p in points))
    argv = [str(exe),str(src),'--k=2','--threads=1','--dump='+str(dump)]
    run = subprocess.run(argv,capture_output=True,text=True,timeout=10)
    (work/(name+'.stdout')).write_text(run.stdout)
    (work/(name+'.stderr')).write_text(run.stderr)
    if run.returncode:
        raise RuntimeError('native refusal')
    nodes = gate.parse(dump)[2]['nodes']
    parents = {p for p,_,_ in nodes if p >= 0}
    births = [v for v in range(len(nodes)) if v not in parents]
    if len(births) != 2:
        raise RuntimeError('expected the two adjacent-pair births')
    alpha2 = min(nodes[v][1] for v in births)
    cutoff2 = alpha2*(1+eta)**2
    candidates = [v for v in births if nodes[v][1] <= cutoff2]
    target = lca(nodes,candidates)
    rows.append(dict(points=points,argv=argv,code=run.returncode,
                     birth_radii=[str(sqrt_fraction(nodes[v][1])) for v in births],
                     cutoff_radius=str(sqrt_fraction(cutoff2)),candidates=candidates,
                     target=target,target_radius=str(sqrt_fraction(nodes[target][1])),
                     dump_sha256=hashlib.sha256(dump.read_bytes()).hexdigest()))
jump = Fraction(rows[0]['target_radius'])-Fraction(rows[1]['target_radius'])
if jump != Fraction(9999,2):
    raise RuntimeError('counterexample not reproduced')
report = dict(source_commit='6206d1d11',binary_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
              eta=str(eta),scale=scale,max_point_displacement=2,rows=rows,
              attachment_radius_jump=str(jump),
              conclusion='two_unit_perturbation_causes_9999_over_2_attachment_jump')
(work/'band_report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
