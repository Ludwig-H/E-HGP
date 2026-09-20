#!/usr/bin/env python3
"""Finite exact output-safety gate; does not test a product q4 implementation."""
from fractions import Fraction as F
import atexit
import gzip
import hashlib
from itertools import combinations
import json
from pathlib import Path
import random
import subprocess
import sys

from fixtures import BASE, distance, save_input


def require(value, message):
    if not value:
        raise RuntimeError(message)


def solve(matrix, rhs):
    rows = [[F(x) for x in row]+[F(y)] for row, y in zip(matrix, rhs)]
    for i in range(len(rows)):
        pivot = next((j for j in range(i, len(rows)) if rows[j][i]), None)
        if pivot is None:
            return None
        rows[i], rows[pivot] = rows[pivot], rows[i]
        denominator = rows[i][i]
        rows[i] = [x/denominator for x in rows[i]]
        for j in range(len(rows)):
            if j != i:
                multiple = rows[j][i]
                rows[j] = [x-multiple*y for x, y in zip(rows[j], rows[i])]
    return [row[-1] for row in rows]


def ball(tetra):
    # Solve perpendicular bisectors in Cartesian coordinates; independent of plane cells.
    a = tetra[0]
    rows = [[2*(p[i]-a[i]) for i in range(3)] for p in tetra[1:]]
    center = solve(rows, [sum(x*x for x in p)-sum(x*x for x in a) for p in tetra[1:]])
    if center is None:
        return None
    bary = solve([[tetra[j][i]-a[i] for j in (1, 2, 3)] for i in range(3)],
                 [center[i]-a[i] for i in range(3)])
    require(bary is not None, 'Cartesian and barycentric ranks disagree')
    if min([1-sum(bary)]+bary) <= 0:
        return None
    return center, distance(a, center)


def seeds(points):
    a, b = points[:2]
    d = distance(a, b)
    return {i for i, x in enumerate(points[2:], 2)
            if max(distance(a, x), distance(b, x)) <= d
            and d+distance(a, x) > distance(b, x)
            and d+distance(b, x) > distance(a, x)
            and distance(a, x)+distance(b, x) > d}


def accepted_seeds(points, k, counts):
    d = distance(*points[:2])
    good = seeds(points)
    accepted = set()
    for x, y in combinations(range(2, len(points)), 2):
        counts['tetrahedra_enumerated'] += 1
        ids = (0, 1, x, y)
        if any(distance(points[i], points[j]) > d for i, j in combinations(ids, 2)):
            continue
        value = ball([points[i] for i in ids])
        if value is None:
            continue
        c, r = value
        counts['positive_owned_tetrahedra'] += 1
        depth = sum(distance(p, c) < r for p in points)
        counts['global_strict_point_tests'] += len(points)
        if depth < k-2:
            accepted.update(good.intersection((x, y)))
            counts['accepted_tetrahedra'] += 1
    return accepted


def fixtures():
    # Positive tetrahedron with one obtuse incident face.
    yield 'obtuse', [(10,20,20),(30,20,20),(20,29,27),(20,11,22)]
    # All eight vertices lie on one sphere: strict count zero, shell cannot be credited.
    cube = [(x, y, z) for x in (0, 8) for y in (0, 8) for z in (0, 8)]
    yield 'cosphere', [cube[0], cube[-1]]+cube[1:-1]
    yield 'flat', [(i, j, 0) for i in range(3) for j in range(3)]
    rng = random.Random(20260920)
    for trial in range(24):
        points = []
        while len(points) < 12:
            p = tuple(rng.randrange(21) for _ in range(3))
            if p not in points:
                points.append(p)
        a, b = max(combinations(range(12), 2), key=lambda ij: distance(points[ij[0]], points[ij[1]]))
        points = [points[a], points[b]]+[p for i, p in enumerate(points) if i not in (a, b)]
        if trial % 3 == 1:
            points = [tuple(3000*x+500 for x in p) for p in points]
        if trial % 3 == 2:
            # Integer similarity from quaternion(1,2,3,4), neither axis-aligned nor a permutation.
            matrix = ((-20,4,22),(20,-10,20),(10,28,4))
            points = [tuple(1000+sum(row[j]*p[j] for j in range(3)) for row in matrix) for p in points]
        yield f'random_{trial}', points


def main():
    exe = Path(sys.argv[1]).resolve()
    target = Path(sys.argv[2]).resolve()
    require(not target.exists(), 'Refusing to overwrite a gate capture')
    counts = dict(calls=0, tetrahedra_enumerated=0, positive_owned_tetrahedra=0,
                  accepted_tetrahedra=0, global_strict_point_tests=0,
                  rejected_families=0, preserved_accepted_families=0)
    records = []
    atexit.register(lambda: target.write_bytes(gzip.compress(json.dumps(records).encode(), mtime=0)))
    # Small budget, dense refinement, and the degenerate root are exercised.
    settings = [(3,0,1,0),(3,5,341,1),(5,5,341,0),(5,7,4096,1),
                (10,7,1,1),(10,7,4096,0),(10,7,4096,1)]
    for name, points in fixtures():
        file, digest = save_input('gate_'+name, points)
        expected = {k: accepted_seeds(points, k, counts) for k in (3,5,10)}
        for k, depth, budget, domain in settings:
            command = [str(exe), str(file), str(k), str(depth), str(budget), str(domain)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=60)
            record = dict(case=name, input_sha256=digest, command=command,
                          returncode=result.returncode, stdout=result.stdout, stderr=result.stderr)
            records.append(record)
            # Preserve the first failed execution, including sanitizer stderr.
            if result.returncode:
                target.write_bytes(gzip.compress(json.dumps(records).encode(), mtime=0))
                raise RuntimeError('Prototype gate call failed')
            data = json.loads(result.stdout)
            rejected = set(data['rejected_seed_ids'])
            require(len(rejected) == data['rejected'], 'Duplicate or missing rejected IDs')
            require(rejected <= seeds(points), 'A nonseed was classified')
            require(data['seeds'] == len(seeds(points)), 'Seed discovery disagrees')
            require(data['survivors']+data['rejected'] == data['seeds'], 'Partition failed')
            require(not rejected.intersection(expected[k]), 'A positive accepted tetrahedron lost its seed')
            counts['calls'] += 1
            counts['rejected_families'] += len(rejected)
            counts['preserved_accepted_families'] += len(expected[k])
    target.write_bytes(gzip.compress(json.dumps(records).encode(), mtime=0))
    require(counts['rejected_families'] > 0 and counts['preserved_accepted_families'] > 0,
            'Gate did not exercise both outcomes')
    print(json.dumps({'status':'passed', 'scope':'independent finite rational oracle; no product qualification',
                      'binary_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),
                      'capture_sha256':hashlib.sha256(target.read_bytes()).hexdigest(), 'counts':counts}, sort_keys=True))


if __name__ == '__main__':
    main()
