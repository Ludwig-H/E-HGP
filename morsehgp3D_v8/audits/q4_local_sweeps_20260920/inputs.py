"""Explicit reuse of the immutable 2920b8b5 audit fixtures, not product code."""
import hashlib
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
REFERENCE = BASE.parent/'q4_center_blocks_20260920'
sys.path.insert(0,str(REFERENCE))
from fixtures import cases, dense, edge_first, lidar  # noqa: E402
from oracle_gate import ball, distance, fixtures as old_fixtures, seeds  # noqa: E402


def save(name, points):
    folder = BASE/'.inputs'
    folder.mkdir(exist_ok=True)
    raw = (str(len(points))+'\n'+'\n'.join(' '.join(map(str,p)) for p in points)+'\n').encode()
    file = folder/(name+'.txt')
    if file.exists() and file.read_bytes()!=raw:
        raise RuntimeError('Refusing to overwrite a different fixture')
    file.write_bytes(raw)
    return file,hashlib.sha256(raw).hexdigest()


def gate_fixtures():
    yield from old_fixtures()
    yield 'lower_left_tangent',[(10,10,10),(12,12,10),(10,12,8),(12,10,8)]
    sphere=[(x,y,z) for x in range(-5,6) for y in range(-5,6) for z in range(-5,6) if x*x+y*y+z*z==25]
    a,b=(5,0,0),(-3,4,0)
    sphere=[a,b]+[p for p in sphere if p not in (a,b)]
    sphere=[tuple(v+20 for v in p) for p in sphere]
    if len(sphere)!=30:
        raise RuntimeError('Shell30 fixture changed')
    yield 'shell30',sphere
    yield 'shell30_inside',sphere+[(20,20,20)]
