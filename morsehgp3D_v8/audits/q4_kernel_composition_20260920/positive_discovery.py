#!/usr/bin/env python3
"""Bounded exact selection of productive LiDAR fixtures, not an edge generator.

Inputs and the earlier Cartesian Fraction oracle are read only. All output
must be a fresh direct child of this audit directory. No product import.
"""
import argparse
from collections import Counter
from fractions import Fraction
from functools import cmp_to_key
import hashlib
from itertools import combinations
import json
from math import gcd, lcm
from pathlib import Path
import sys

sys.dont_write_bytecode = True
BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
REFERENCE = BASE.parent / 'q4_center_blocks_20260920'
sys.path.insert(0, str(REFERENCE))
from fixtures import distance, lidar
from oracle_gate import ball, solve

SCANS = (0, 100, 200)
ANCHORS = (0, 1000, 3000, *range(1, 30))
NEIGHBORS = 12
TARGET = 3
N = 8000


def require(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def rational(value):
    return [value.numerator, value.denominator]


def key(center, radius2):
    raw = [Fraction(1), *(-2*x for x in center), sum(x*x for x in center)-radius2]
    denominator = lcm(*(x.denominator for x in raw))
    coefficients = [int(x*denominator) for x in raw]
    divisor = gcd(*coefficients)
    coefficients = tuple(x//divisor for x in coefficients)
    require(coefficients[0] > 0, 'sphere coefficient must be positive')
    return coefficients


def source_pins():
    paths = [Path(__file__).resolve(), REFERENCE/'fixtures.py', REFERENCE/'oracle_gate.py']
    return {str(p.relative_to(ROOT)): sha(p) for p in paths}


def input_pins():
    pins = {}
    for scan in SCANS:
        folder = BASE.parent/'lidar08_20260914/prepared'/f'single_{scan:06d}'
        for name in ('n8000.u16le', 'METADATA.json'):
            path = folder/name
            pins[str(path.relative_to(ROOT))] = sha(path)
    return pins


def discover_scan(scan, manifest):
    points, pins = lidar(scan, N)
    require(len(set(points)) == N, 'LiDAR owner contains duplicate coordinates')
    norms = [sum(v*v for v in p) for p in points]
    counts = Counter(norm_points=N, unique_coordinate_checks=N)
    log = dict(scan=scan, n=N, counts=counts, anchors=[], proposals=[],
               input_sha256=pins, selected=0, stop_reason='anchor_budget_exhausted')
    manifest['searches'].append(log)
    seen_tetra, seen_balls, chosen_edges = set(), set(), set()
    for anchor in ANCHORS:
        ranked = [(distance(points[anchor], p), i) for i, p in enumerate(points) if i != anchor]
        counts['nearest_distance_tests'] += N-1
        def compare(left, right):
            counts['nearest_sort_comparisons'] += 1
            return (left > right)-(left < right)
        ranked.sort(key=cmp_to_key(compare))
        nearest = [i for _, i in ranked[:NEIGHBORS]]
        log['anchors'].append(dict(anchor_id=anchor, nearest_ids=nearest))
        counts['anchors'] += 1
        for companions in combinations(nearest, 3):
            ids = tuple(sorted((anchor, *companions)))
            proposal = dict(anchor_id=anchor, support_ids=list(ids))
            log['proposals'].append(proposal)
            counts['proposals'] += 1
            if ids in seen_tetra:
                proposal['status'] = 'duplicate_tetrahedron'
                counts['duplicate_tetrahedra'] += 1
                continue
            seen_tetra.add(ids)
            tetra = [points[i] for i in ids]
            counts['cartesian_positive_ball_calls'] += 1
            value = ball(tetra)
            if value is None:
                proposal['status'] = 'rank_deficient_or_not_strictly_positive'
                counts['nonpositive_or_rank_deficient'] += 1
                continue
            center, radius2 = value
            coefficients = key(center, radius2)
            counts['positive_tetrahedra'] += 1
            proposal['coefficients'] = list(coefficients)
            if coefficients in seen_balls:
                proposal['status'] = 'duplicate_ball'
                counts['duplicate_balls'] += 1
                continue
            seen_balls.add(coefficients)
            # Canonical owner: longest edge, ties by ORIGINAL input IDs.
            pairs = [(distance(points[a], points[b]), (a, b)) for a, b in combinations(ids, 2)]
            counts['owner_edge_distance_tests'] += 6
            longest = max(d for d, _ in pairs)
            edge = min(pair for d, pair in pairs if d == longest)
            proposal['edge_ids'] = list(edge)
            q, x, y, z, constant = coefficients
            interior, shell = [], []
            for i, (p, norm) in enumerate(zip(points, norms)):
                power = q*norm+x*p[0]+y*p[1]+z*p[2]+constant
                if power < 0:
                    interior.append(i)
                elif power == 0:
                    shell.append(i)
            counts['global_integer_census_calls'] += 1
            counts['global_integer_point_tests'] += N
            proposal['depth'] = len(interior)
            require(set(ids) <= set(shell), 'support missing from integer shell')
            if len(interior) >= 3:
                proposal['status'] = 'depth_at_least_3'
                counts['depth_rejections'] += 1
                continue
            if edge in chosen_edges:
                proposal['status'] = 'duplicate_selected_edge'
                counts['duplicate_selected_edges'] += 1
                continue
            # On selected fixtures, also check EVERY site using direct Fraction
            # squared distances, independently of the integer power evaluation.
            fraction_inside, fraction_shell = [], []
            for i, p in enumerate(points):
                delta = distance(p, center)-radius2
                if delta < 0:
                    fraction_inside.append(i)
                elif delta == 0:
                    fraction_shell.append(i)
            counts['selected_fraction_point_tests'] += N
            require((interior, shell) == (fraction_inside, fraction_shell), 'Fraction/global integer census differs')
            bary = solve([[tetra[j][axis]-tetra[0][axis] for j in (1, 2, 3)] for axis in range(3)],
                         [center[axis]-tetra[0][axis] for axis in range(3)])
            weights = [1-sum(bary), *bary]
            require(min(weights) > 0, 'selected center is not strictly inside its tetrahedron')
            a, b = edge
            acute_ids = []
            for i in ids:
                if i in edge:
                    continue
                da, db = distance(points[a], points[i]), distance(points[b], points[i])
                counts['selected_face_distance_tests'] += 2
                if da+db > longest and da+longest > db and db+longest > da:
                    acute_ids.append(i)
            require(acute_ids, 'positive owned tetrahedron has no acute incident seed')
            chosen_edges.add(edge)
            proposal['status'] = 'selected'
            fixture = dict(scan=scan, n=N, edge_ids=list(edge), support_ids=list(ids),
                           support_coordinates=[list(p) for p in tetra],
                           edge_coordinates=[list(points[i]) for i in edge],
                           coefficients=list(coefficients), center=[rational(c) for c in center],
                           radius_squared=rational(radius2), barycentric_weights=[rational(w) for w in weights],
                           depth=len(interior), interior_ids=interior, shell_ids=shell,
                           input_sha256=pins, acute_seed_ids=acute_ids,
                           owner_distance_squared=longest, proposal_index=len(log['proposals'])-1,
                           selection_anchor_id=anchor)
            manifest['fixtures'].append(fixture)
            log['selected'] += 1
            counts['selected'] += 1
            print(json.dumps(dict(scan=scan, edge_ids=list(edge), depth=len(interior), shell_size=len(shell))), flush=True)
            if log['selected'] == TARGET:
                log['stop_reason'] = 'three_distinct_productive_edges_selected'
                return


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', default='POSITIVE_FIXTURES.json')
    parser.add_argument('--compare')
    args = parser.parse_args()
    target = (BASE/args.output).resolve()
    require(target.parent == BASE and not target.exists(), 'fresh direct-child output required')
    before, inputs = source_pins(), input_pins()
    manifest = dict(schema='mhgp8_audit_positive_lidar_fixtures_v1', status='started',
                    scope='bounded fixture selection only; no exhaustive edge generator or product qualification',
                    recipe=dict(scans=list(SCANS), n=N, kmax=5, strict_depth_limit=3,
                                target_per_scan=TARGET, anchors=list(ANCHORS), nearest_count=NEIGHBORS,
                                neighbor_order='exact squared distance, then original input ID',
                                proposals='anchor plus each lexicographic 3-combination of its 12 nearest IDs; canonical sorted support IDs',
                                max_proposals_per_scan=len(ANCHORS)*220,
                                stop='first three distinct ball keys and owner edges per scan, otherwise exhaust the bounded anchor list',
                                coordinate_policy='immutable single scans, independently prepared; no alignment, transforms or merging',
                                arithmetic='Fraction Cartesian perpendicular-bisector solve and strict barycentric positivity; integer primitive sphere key and full global census',
                                key_order=['quadratic','linear_x','linear_y','linear_z','constant']),
                    sources=before, input_files=inputs, fixtures=[], searches=[])
    try:
        for scan in SCANS:
            discover_scan(scan, manifest)
        require(all(search['selected'] >= 1 for search in manifest['searches']), 'bounded search found no productive edge for a scan')
        require(source_pins() == before and input_pins() == inputs, 'source or input changed during search')
        manifest['all_targets_met'] = all(search['selected'] == TARGET for search in manifest['searches'])
        manifest['status'] = 'completed'
        result = {k: manifest[k] for k in ('recipe','sources','input_files','fixtures','searches','all_targets_met')}
        manifest['result_sha256'] = hashlib.sha256(encoded(result)).hexdigest()
        if args.compare:
            previous = (BASE/args.compare).resolve()
            require(previous.parent == BASE, 'comparison must belong to this audit directory')
            other = json.loads(previous.read_text())
            require(other['status'] == 'completed' and all(other[k] == result[k] for k in result),
                    'normal/optimized discovery differs')
            manifest['comparison'] = dict(path=previous.name, sha256=sha(previous), identical_result=True)
    except BaseException as error:
        manifest['status'] = 'failed'
        manifest['error'] = repr(error)
        raise
    finally:
        manifest['execution'] = dict(python=sys.version, optimize=sys.flags.optimize, command=sys.argv)
        target.write_text(json.dumps(manifest, sort_keys=True, indent=2)+'\n')
    print(json.dumps(dict(status=manifest['status'], fixtures=len(manifest['fixtures']),
                          result_sha256=manifest['result_sha256'], output_sha256=sha(target)), sort_keys=True))


if __name__ == '__main__':
    main()
