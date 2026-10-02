"""Petits controles autonomes des certificats de familles ; aucun produit importe."""
from fractions import Fraction as F
from hashlib import sha256
from itertools import combinations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COUNTS = {}


def check(ok, group, message):
    COUNTS[group] = COUNTS.get(group, 0) + 1
    if not ok:
        raise ValueError(group + ': ' + message)


def difference(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def distance2(a, b):
    delta = difference(a, b)
    return dot(delta, delta)


def linear_solve(matrix, rhs):
    rows = [list(map(F, row)) + [F(value)] for row, value in zip(matrix, rhs)]
    n = len(rows)
    for j in range(n):
        pivot = next((i for i in range(j, n) if rows[i][j]), None)
        if pivot is None:
            return None
        rows[j], rows[pivot] = rows[pivot], rows[j]
        scale = rows[j][j]
        rows[j] = [value / scale for value in rows[j]]
        for i in range(n):
            if i != j:
                scale = rows[i][j]
                rows[i] = [a - scale * b for a, b in zip(rows[i], rows[j])]
    return tuple(row[-1] for row in rows)


def circumsphere(points):
    a = points[0]
    edges = tuple(difference(p, a) for p in points[1:])
    weights = linear_solve([[dot(x, y) for y in edges] for x in edges],
                           [F(dot(x, x), 2) for x in edges])
    if weights is None:
        return None
    c = tuple(F(a[j]) + sum(w * v[j] for w, v in zip(weights, edges)) for j in range(3))
    return c, distance2(a, c), (1 - sum(weights),) + weights


def presentations(points, arities):
    out = {}
    for q in arities:
        for support in combinations(range(len(points)), q):
            sphere = circumsphere([points[i] for i in support])
            if sphere is not None and all(w > 0 for w in sphere[2]):
                out.setdefault(sphere[:2], []).append(support)
    return out


def catalogue(points, groups, k):
    out = []
    for (center, radius2), supports in groups.items():
        support = min(supports, key=lambda s: (len(s), s))
        inner = tuple(i for i, p in enumerate(points) if distance2(p, center) < radius2)
        shell = tuple(i for i, p in enumerate(points) if distance2(p, center) == radius2)
        if len(inner) + len(support) <= k + 1:
            out.append((center, radius2, support, inner, shell))
    return tuple(sorted(out, key=lambda b: (b[1], b[2])))


def family_route(points, common):
    # Keep every q2/q3 ball. At most one additional ball can come from q4.
    groups = presentations(points, (2, 3))
    if common not in groups:
        for triple in combinations(range(1, len(points)), 3):
            support = (0,) + triple
            candidate = circumsphere([points[i] for i in support])
            if candidate is not None and all(w > 0 for w in candidate[2]):
                check(candidate[:2] == common, 'common_family', 'independent quadruple has another sphere')
                groups[common] = [support]
                break
    return groups


def main():
    before = json.loads((ROOT / 'SOURCE_BEFORE.json').read_text())
    for row in before['files']:
        data = (ROOT / 'source' / row['path']).read_bytes()
        check(len(data) == row['bytes'] and sha256(data).hexdigest() == row['sha256'],
              'source_hashes', 'captured source changed')
    points4 = ((4, 5, 5), (4, 1, 1), (2, 5, 1), (2, 1, 5), (5, 4, 5))
    common4 = ((F(3),) * 3, F(9))
    q4_presentations = []
    for shift in range(5):
        points = points4[shift:] + points4[:shift]
        full = presentations(points, (2, 3, 4))
        candidates = full[common4]
        check(all(len(s) == 4 for s in candidates), 'qmin4_shell5', 'lower support unexpectedly exists')
        check(len({s[0] for s in candidates}) >= 1, 'qmin4_shell5', 'no positive tetrahedron')
        for vertex in range(5):
            check(any(vertex in s for s in candidates), 'anchor_lemma', 'shell vertex cannot complete to strict tetrahedron')
        check(min(candidates)[0] == 0, 'anchor_lemma', 'canonical first site is not minimal shell site')
        reduced = family_route(points, common4)
        for k in (1, 2, 3, 4, 5, 6):
            check(catalogue(points, reduced, k) == catalogue(points, full, k),
                  'catalogue_equivalence', 'one-family q4 route lost catalogue or I/U')
        q4_presentations.append(len(candidates))
    # Noncoplanar shell with qmin3: its first site lies outside every triangle support of b.
    points3 = ((5, 5, 10), (10, 5, 5), (2, 9, 5), (2, 1, 5))
    common3 = ((F(5),) * 3, F(25))
    supports3 = presentations(points3, (2, 3, 4))[common3]
    check(supports3 == [(1, 2, 3)], 'anchor_scope', 'qmin3 scope witness changed')
    check(any(circumsphere([points3[i] for i in s]) is not None
              for s in combinations(range(4), 4)), 'anchor_scope', 'shell is not full-dimensional')
    # Entire shell in an open hemisphere: b is not critical, but other q2/q3 balls remain.
    north = ((4, 5, 5), (2, 5, 5), (5, 4, 5), (5, 2, 5), (3, 3, 6))
    north_full = presentations(north, (2, 3, 4))
    check(common4 not in north_full, 'noncritical_family', 'hemisphere center became critical')
    north_reduced = family_route(north, common4)
    for k in (1, 3, 6):
        check(catalogue(north, north_reduced, k) == catalogue(north, north_full, k),
              'noncritical_family', 'other centers were lost')
    # Positive fourth weight is necessary, including for an obtuse triangle prefix.
    triangle = ((0, 0, 0), (4, 0, 0), (2, 3, 0))
    extension_cases = [(triangle, p) for p in ((2, 0, 2), (2, 1, 1), (2, 1, 3))]
    obtuse = ((5, 2, 1), (10, 5, 5), (9, 8, 5))
    extension_cases.append((obtuse, (1, 5, 8)))
    signs = []
    for prefix, s in extension_cases:
        tri = circumsphere(prefix)
        tetra = circumsphere(prefix + (s,))
        normal = cross(difference(prefix[1], prefix[0]), difference(prefix[2], prefix[0]))
        height2 = F(dot(normal, difference(s, prefix[0])) ** 2, dot(normal, normal))
        power = distance2(s, tri[0]) - tri[1]
        check(height2 > 0 and tetra is not None, 'extension_identity', 'extension not independent')
        check(tetra[2][3] == power / (2 * height2), 'extension_identity', 'fourth weight/power identity wrong')
        if power <= 0:
            check(not all(w > 0 for w in tetra[2]), 'extension_filter', 'inside or shell extension is strict')
        signs.append((power > 0) - (power < 0))
    check(signs == [0, -1, 1, 1], 'extension_filter', 'power guards changed')
    check(any(w < 0 for w in circumsphere(obtuse)[2])
          and all(w > 0 for w in circumsphere(obtuse + ((1, 5, 8),))[2]),
          'obtuse_preserved', 'obtuse prefix strict extension lost')
    collinear = ((0, 0, 0), (2, 0, 0), (3, 0, 0))
    for p in ((0, 1, 1), (9, 2, 1)):
        check(circumsphere(collinear + (p,)) is None, 'rank_pruning', 'collinear prefix has independent q4')
    # Ownership is necessary to reuse the co-spherical list as the global shell.
    center = (10, 10, 10)
    listed = ((5, 10, 10), (15, 10, 10))
    global_points = listed + (center,)
    check(all(distance2(p, center) == 25 for p in listed), 'owner_scope', 'listed shell witness changed')
    check(distance2(global_points[2], center) < 25, 'owner_scope', 'global interior missing from counterexample')
    # On Q=[14,16]x[10,11]x[10,11], site15 strictly dominates site10;
    # listed is therefore a valid conservative K1 list although c is outside Q.
    check(min(distance2(center, (x, y, z)) - distance2(listed[1], (x, y, z))
              for x in (14, 16) for y in (10, 11) for z in (10, 11)) > 0,
          'owner_scope', 'K1 certificate outside center failed')
    after = json.loads((ROOT / 'SOURCE_AFTER.json').read_text())
    for row in after['captured_files']:
        check(sha256((ROOT / 'source' / row['path']).read_bytes()).hexdigest() == row['sha256'],
              'source_hashes', 'copy changed before closure')
    manifest = ROOT / 'SHA256SUMS'
    if manifest.exists():
        for line in manifest.read_text().splitlines():
            wanted, name = line.split('  ', 1)
            path = Path(name)
            if path.is_absolute() or '..' in path.parts or sha256((ROOT / path).read_bytes()).hexdigest() != wanted:
                raise ValueError('manifest mismatch: ' + name)
    print(json.dumps({'checks': COUNTS, 'total_checks': sum(COUNTS.values()),
                      'qmin4_shell5_positive_presentations_by_rotation': q4_presentations,
                      'extension_power_signs': signs, 'native_executions': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
