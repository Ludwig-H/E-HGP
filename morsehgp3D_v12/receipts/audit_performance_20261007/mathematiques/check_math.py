#!/usr/bin/env python3
"""Small independent Fraction witnesses; no native build, cloud data, GCP or timings."""
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import json
import struct
import sys

PIN = '58d384721678d11ef8ccd86c76cca182f41a716c'
ROOT = Path(__file__).resolve().parents[4]


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sign(x):
    return (x > 0) - (x < 0)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def squared(a, b):
    return dot(subtract(a, b), subtract(a, b))


def solve(matrix, rhs):
    n = len(rhs)
    a = [[Q(x) for x in row] + [Q(v)] for row, v in zip(matrix, rhs)]
    for col in range(n):
        pivot = next((i for i in range(col, n) if a[i][col]), None)
        if pivot is None:
            return None
        a[col], a[pivot] = a[pivot], a[col]
        v = a[col][col]
        a[col] = [x / v for x in a[col]]
        for i in range(n):
            if i != col:
                v = a[i][col]
                a[i] = [x - v * y for x, y in zip(a[i], a[col])]
    return tuple(row[-1] for row in a)


def critical(points):
    if len(points) == 1:
        return tuple(map(Q, points[0])), Q(0)
    vectors = [subtract(p, points[0]) for p in points[1:]]
    weights = solve([[2 * dot(a, b) for b in vectors] for a in vectors],
                    [dot(a, a) for a in vectors])
    if weights is None or min((1 - sum(weights),) + weights) <= 0:
        return None
    center = tuple(Q(points[0][axis]) + sum(w * v[axis] for w, v in zip(weights, vectors))
                   for axis in range(3))
    return center, squared(center, points[0])


def morton(point):
    return sum(((v >> bit) & 1) << (3 * bit + axis)
               for axis, v in enumerate(point) for bit in range(32))


def forms_witness():
    triangle = [(15, 10, 3), (7, 14, 3), (7, 6, 3)]
    pair = [(0, 100, 0), (0, 110, 0)]
    u, v = subtract(triangle[1], triangle[0]), subtract(triangle[2], triangle[0])
    uu, vv, uv = dot(u, u), dot(v, v), dot(u, v)
    pair_form = squared(*pair), 4
    tri_form = uu * vv * (uu + vv - 2 * uv), 4 * (uu * vv - uv * uv)
    need(pair_form == (100, 4) and tri_form == (409600, 16384), 'unreduced forms')
    need(critical(pair)[1] == critical(triangle)[1] == 25, 'equal exact levels')
    need(min(map(morton, triangle)) < min(map(morton, pair)), 'Morton puts triangle first')
    need(min(pair) < min(triangle), 'lexicographic positions put pair first')
    need(critical(pair)[0] != critical(triangle)[0], 'same level is not same ball')
    return dict(points=triangle + pair, pair_form=pair_form, triangle_form=tri_form,
                level='25', first_morton='triangle', first_positions='pair', extended_shell_needed=False)


def payload(level_form=(100, 4), center_scale=1, point_id=0, center=5, vertical=2, parent=2):
    # Actual FULL object of {(0,0,0),(10,0,0)}, K=2. Only encoding forms vary in positive controls.
    def word(x):
        return struct.pack('<Q', x)
    def integer(x):
        return word(int(x < 0)) + word(1) + word(abs(x))
    def words(values):
        return b''.join(map(word, values))
    def level(n, d):
        return integer(n) + integer(d)
    out = bytearray(b'MHGP11FUL1' + words((21, 2, 2, 2)))
    out += words((0, 0, 0, 1, point_id, 10, 0, 0, 1, 1))
    out += words((1, 2, 3, 2, 2))
    for x in (0, 10):
        out += words((parent, 0, 0)) + level(0, 1)
        out += b''.join(integer(t) for t in (x, 0, 0, 1))
    out += words((2**32 - 1, 0, 2)) + level(*level_form) + words((0, 1))
    out += words((2, 1, 1, 0, 0))
    out += words((2**32 - 1, 0, 0)) + level(*level_form)
    out += b''.join(integer(t) for t in (center * center_scale, 0, 0, center_scale))
    out += word(vertical)
    return bytes(out)


def semantic_checks():
    sys.path.insert(0, str(ROOT / 'morsehgp3D_v11' / 'bench'))
    from full_semantic import decode
    original = payload()
    equivalent = payload((409600, 16384), center_scale=4)
    first, second = decode(original, 21, 2, 2), decode(equivalent, 21, 2, 2)
    need(first['raw_sha256'] != second['raw_sha256'], 'raw bytes must differ')
    need(first['sha256'] == second['sha256'], 'rational normalization only')
    changed = []
    for label, data in [('level', payload((104, 4))), ('center', payload(center=6)),
                        ('point_id', payload(point_id=7))]:
        need(decode(data, 21, 2, 2)['sha256'] != first['sha256'], 'semantic mutation detected: ' + label)
        changed.append(label)
    rejected = []
    for label, data in [('vertical', payload(vertical=0)), ('parent', payload(parent=1))]:
        try:
            decode(data, 21, 2, 2)
        except (ValueError, RuntimeError) as error:
            rejected.append(dict(field=label, reason=str(error)))
        else:
            raise RuntimeError('structural mutation accepted: ' + label)
    return dict(semantic_sha256=first['sha256'], raw_sha256=[first['raw_sha256'], second['raw_sha256']],
                changed_semantics=changed, rejected=rejected,
                warning='strict structural reader is not an independent geometric oracle')


def lex_rank_checks():
    points = [(0, 0, 0), (1, 8, 2), (8, 1, 2), (3, 4, 5), (4, 3, 5), (8, 8, 8),
              (2**32 - 1, 0, 0), (0, 2**32 - 1, 0), (0, 0, 2**32 - 1)]
    ranks = {p: i + 1 for i, p in enumerate(sorted(points))}
    supports = [tuple(points[i] for i in part) for q in (2, 3, 4)
                for part in combinations(range(len(points)), q)]
    plain = [tuple(sorted(s)) for s in supports]
    prepared = [tuple(sorted(ranks[p] for p in s)) + (0,) * (4 - len(s)) for s in supports]
    comparisons = 0
    for i in range(len(supports)):
        for j in range(i, len(supports)):
            need(sign((plain[i] > plain[j]) - (plain[i] < plain[j])) ==
                 sign((prepared[i] > prepared[j]) - (prepared[i] < prepared[j])), 'prepared exact lex key')
            comparisons += 1
    return dict(sites=len(points), supports=len(supports), comparisons=comparisons,
                encoding='site lex ranks 1..n; zero padding; never Morton ranks')


def cache_checks():
    clouds = [([(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)]),
              [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0), (1, 1, 0), (7, 1, 0)],
              [(15, 10, 3), (7, 14, 3), (7, 6, 3), (0, 100, 0), (0, 110, 0)]]
    supports_checked = shell_reuses = population_reuses = 0
    for points in clouds:
        by_shell, by_population, supports = {}, {}, []
        for q in range(2, 5):
            for part in combinations(range(len(points)), q):
                sphere = critical([points[i] for i in part])
                if sphere is None:
                    continue
                center, radius = sphere
                shell = tuple(i for i, p in enumerate(points) if squared(p, center) == radius)
                population = tuple(i for i, p in enumerate(points) if squared(p, center) <= radius)
                shell_reuses += shell in by_shell
                population_reuses += population in by_population
                need(by_shell.setdefault(shell, sphere) == sphere, 'shell uniquely identifies critical ball')
                need(by_population.setdefault(population, sphere) == sphere, 'population uniquely identifies ball')
                supports.append((part, sphere))
                supports_checked += 1
        for part, sphere in supports:
            for other, other_sphere in supports:
                if len(part) < len(other) and set(part) < set(other):
                    need(sphere[1] != other_sphere[1], 'minimal supports cannot nest at equal level')
    # Hash-only, cardinal-only and support-only memoization are intentionally falsified.
    line = [(0, 0, 0), (2, 0, 0), (10, 0, 0)]
    need(critical(line[:2]) != critical(line[1:]), 'same cardinal is not same MEB')
    need(sum((0, 3)) == sum((1, 2)), 'additive fingerprints have collisions')
    return dict(clouds=len(clouds), critical_presentations=supports_checked,
                shell_reuses=shell_reuses, population_reuses=population_reuses,
                negative_controls=['cardinality-only cache', 'additive fingerprint without exact equality'])


def main():
    result = dict(schema='ehgp.v12.audit_performance.math.v1', pin=PIN, native_engine_executed=False,
                  gcp_used=False, forms=forms_witness(), full_reader=semantic_checks(),
                  lex_rank=lex_rank_checks(), cache=cache_checks())
    print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
