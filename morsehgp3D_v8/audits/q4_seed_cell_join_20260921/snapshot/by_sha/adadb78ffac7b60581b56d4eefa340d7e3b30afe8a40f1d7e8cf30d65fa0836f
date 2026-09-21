"""Explicit edge fixtures, not an edge generator or aligned LiDAR passes."""
import hashlib
import json
from pathlib import Path
import struct

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def dense(n, permuted=False):
    grid = [(980+i % 41, 1120+(i//41) % 21,
             1000+(40+i//(41*21)//2)*(1 if (i//(41*21)) % 2 == 0 else -1))
            for i in range(41*21*38)]
    if permuted:
        # Frozen platform-independent bijection of grid indices (SHA256 ordering).
        grid = [grid[i] for i in sorted(range(len(grid)), key=lambda j:
                 (hashlib.sha256(b'center-map-dense-v1:'+str(j).encode()).digest(), j))]
    return [(900, 1000, 1000), (1100, 1000, 1000)]+grid[:n-2]


def lidar(scan, n):
    folder = ROOT/'morsehgp3D_v8/audits/lidar08_20260914/prepared'/f'single_{scan:06d}'
    file = folder/f'n{n}.u16le'
    raw = file.read_bytes()
    expected = next(s['sha256'] for s in json.loads((folder/'METADATA.json').read_text())['samples'] if s['n'] == n)
    actual = hashlib.sha256(raw).hexdigest()
    if actual != expected or len(raw) != 6*n:
        raise RuntimeError('LiDAR input changed')
    return list(struct.iter_unpack('<HHH', raw)), {str(file.relative_to(ROOT)): actual}


def edge_first(points, ai, bi):
    return [points[ai], points[bi]]+[p for i, p in enumerate(points) if i not in (ai, bi)]


def save_input(name, points):
    folder = BASE/'.inputs'
    folder.mkdir(exist_ok=True)
    raw = (str(len(points))+'\n'+'\n'.join(' '.join(map(str, p)) for p in points)+'\n').encode()
    path = folder/(name+'.txt')
    if path.exists() and path.read_bytes() != raw:
        raise RuntimeError('Refusing to overwrite a different prepared fixture')
    path.write_bytes(raw)
    return path, hashlib.sha256(raw).hexdigest()


def distance(a, b):
    return sum((x-y)**2 for x, y in zip(a, b))


def cases():
    for permuted in (False, True):
        for n in (8000, 16000, 32000):
            name = f'dense_{"permuted" if permuted else "prefix"}_{n}'
            yield name, dense(n, permuted), {'recipe': 'grid_41x21x38_sha256_v1' if permuted else 'grid_41x21x38_prefix_v1'}, [5, 7]
    for scan in (0, 100, 200):
        small, pins = lidar(scan, 8000)
        large, pins50 = lidar(scan, 50000)
        if large[:8000] != small:
            raise RuntimeError('The paired LiDAR inputs are not nested')
        nearest = sorted(range(1, len(small)), key=lambda i: (distance(small[0], small[i]), i))
        # Same physical edge at 8k and 50k; no transformation or merge of scans.
        for rank in (8, 32, 128):
            b = nearest[rank-1]
            for n, points, pin in ((8000, small, pins), (50000, large, pins50)):
                yield f'lidar_{scan:06d}_{n}_fixed_rank{rank}', edge_first(points, 0, b), {
                    'input_sha256': pin, 'original_edge_ids': [0, b], 'rank_selected_at_n': 8000}, [7]
        # Explicitly broader, still nonrepresentative edges: more than the old 54 faces.
        for anchor in (0, 1000, 3000):
            order = sorted((i for i in range(len(small)) if i != anchor),
                           key=lambda i: (distance(small[anchor], small[i]), i))
            b = order[511]
            yield f'lidar_{scan:06d}_8000_anchor{anchor}_rank512', edge_first(small, anchor, b), {
                'input_sha256': pins, 'original_edge_ids': [anchor, b], 'rank_selected_at_n': 8000}, [7]
