"""Bounded exact audit of decimal grids and radius perturbation, no engine."""
from fractions import Fraction as F
from itertools import combinations
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


ROOT = Path(__file__).resolve().parent


def hashes():
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted((ROOT / 'source').glob('*.py'))}


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


before = hashes()
prep = load('exact_grid_audit', ROOT / 'source/prepare_lidar_precision.py')
ref = load('exact_meb_audit', ROOT / 'source/hgp10_ref.py')
clouds = {
    'tetra': [(0.0017, 0.0024, 0.0031), (0.0041, 0.0027, 0.0039),
              (0.0019, 0.0056, 0.0036), (0.0025, 0.0033, 0.0078)],
    'near_sensor_plane': [(-0.00004, 0, 0), (0.00004, 0, 0),
                          (0, 0.00131, 0.00013), (0.0015, -0.00127, 0.0002),
                          (0.00101, 0.00075, 0.0012)],
    'wide_origin': [(123.125, -456.5, 2.25), (123.2, -456.25, 2.0),
                    (123.15, -456.3, 2.5), (123.17, -456.17, 2.33)],
}
rows = []
counts = dict(coordinate_roundings=0, raw_mapping_entries=0, radius_pairs=0,
              fused_profiles=0, shifted_cut_profiles=0, origin_outside_u32_profiles=0)
for name, coordinates in clouds.items():
    raw = b''.join(struct.pack('<ffff', *point, 0.75) for point in coordinates)
    points = [tuple(F(v) for v in point[:3]) for point in struct.iter_unpack('<ffff', raw)]
    require(ref.rank([ref.sub(p, points[0]) for p in points[1:]]) == 3, 'cloud is not 3D')
    raw_beta = {ids: ref.meb(points, ids)[0]
                for size in range(1, len(points) + 1) for ids in combinations(range(len(points)), size)}
    for mm in ('1', '0.1', '0.01'):
        step = F(mm) / 1000
        meta, payloads = prep.reconstruct(raw, 'grid', mm)
        sites = list(struct.iter_unpack('<III', payloads['full.u32le']))
        mapping = [x[0] for x in struct.iter_unpack('<I', payloads['raw_to_full.u32le'])]
        translation = meta['translation']['vector']
        quantized = [tuple(step * (sites[s][axis] - translation[axis]) for axis in range(3))
                     for s in mapping]
        require(len(mapping) == len(points) and set(mapping) == set(range(len(sites))), 'mapping completeness')
        require(meta['quantization']['step_metres'] ==
                {'numerator': step.numerator, 'denominator': step.denominator}, 'physical unit changed')
        counts['raw_mapping_entries'] += len(mapping)
        for p, q in zip(points, quantized):
            for x, y in zip(p, q):
                index = (2*x.numerator*step.denominator + x.denominator*step.numerator) // (
                    2*x.denominator*step.numerator)
                require(y == index * step and 2*abs(y-x) <= step, 'exact decimal rounding mismatch')
                counts['coordinate_roundings'] += 1
        epsilon2 = max(ref.dot(ref.sub(p, q), ref.sub(p, q)) for p, q in zip(points, quantized))
        require(4*epsilon2 <= 3*step*step, 'Euclidean grid error exceeds declared bound')
        max_beta_change = F(0)
        for ids, a in raw_beta.items():
            b = ref.meb(quantized, ids)[0]
            # |sqrt(a)-sqrt(b)| <= sqrt(epsilon2), tested without approximate roots.
            difference = a + b - epsilon2
            require(difference <= 0 or difference*difference <= 4*a*b, 'MEB radius perturbation bound')
            counts['radius_pairs'] += 1
            max_beta_change = max(max_beta_change, abs(a-b))
        fused = len(sites) != len(points)
        shifted = meta['boundaries']['raw_to_represented_quadrant_changes'] > 0
        outside = any(not 0 <= t < 2**32 for t in translation)
        counts['fused_profiles'] += fused
        counts['shifted_cut_profiles'] += shifted
        counts['origin_outside_u32_profiles'] += outside
        rows.append(dict(cloud=name, precision_mm=mm, raw_returns=len(points), sites=len(sites),
                         epsilon_squared_m2=str(epsilon2), bound_squared_m2=str(3*step*step/4),
                         max_squared_radius_change_m2=str(max_beta_change), fusions=fused,
                         shifted_raw_cut=shifted, origin_outside_u32=outside,
                         fixed_raw_ID_universe_only=True,
                         deduplicated_site_KNN_equivalence_claimed=False))
require(counts['radius_pairs'] == 183 and counts['coordinate_roundings'] == 117, 'panel changed')
require(counts['fused_profiles'] >= 2 and counts['shifted_cut_profiles'] >= 2 and
        counts['origin_outside_u32_profiles'] >= 3, 'boundary cases vacuous')
require(before == hashes(), 'source changed')
print(json.dumps(dict(status='PASS', scope='bounded metric audit only; no native FULL/G4',
                      optimize_flag=sys.flags.optimize, sources=before, counts=counts,
                      observations=rows, GCP_used=False), sort_keys=True, indent=2))
