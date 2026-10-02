"""Petits temoins exacts : Level differe, minorant commun q4, limites de census."""
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


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dist2(a, b):
    return dot(sub(a, b), sub(a, b))


def solve(matrix, rhs):
    rows = [list(map(F, row)) + [F(value)] for row, value in zip(matrix, rhs)]
    n = len(rows)
    for j in range(n):
        pivot = next((i for i in range(j, n) if rows[i][j]), None)
        if pivot is None:
            return None
        rows[j], rows[pivot] = rows[pivot], rows[j]
        scale = rows[j][j]
        rows[j] = [v / scale for v in rows[j]]
        for i in range(n):
            if i != j:
                scale = rows[i][j]
                rows[i] = [a - scale * b for a, b in zip(rows[i], rows[j])]
    return tuple(row[-1] for row in rows)


def circumsphere(points):
    anchor = points[0]
    edges = tuple(sub(p, anchor) for p in points[1:])
    weights = solve([[dot(a, b) for b in edges] for a in edges], [F(dot(a, a), 2) for a in edges])
    if weights is None:
        return None
    center = tuple(F(anchor[j]) + sum(w * v[j] for w, v in zip(weights, edges)) for j in range(3))
    return center, dist2(anchor, center), (1 - sum(weights),) + weights


def witnesses(points, prefix):
    triangle = [points[i] for i in prefix]
    sphere = circumsphere(triangle)
    if sphere is None:
        return None
    normal = cross(sub(triangle[1], triangle[0]), sub(triangle[2], triangle[0]))
    return tuple(i for i, p in enumerate(points) if i not in prefix
                 and dot(normal, sub(p, triangle[0])) == 0
                 and dist2(p, sphere[0]) < sphere[1])


def all_groups(points, k=None):
    out = {}
    for q in (2, 3, 4):
        for support in combinations(range(len(points)), q):
            if q == 4 and k is not None:
                if k < 3:
                    continue
                common = witnesses(points, support[:3])
                if common is None or len(common) >= k - 2:
                    continue
            sphere = circumsphere([points[i] for i in support])
            if sphere is not None and all(w > 0 for w in sphere[2]):
                out.setdefault(sphere[:2], []).append(support)
    return out


def catalogue(points, groups, k):
    out = []
    for (center, radius2), supports in groups.items():
        canonical = min(supports, key=lambda s: (len(s), s))
        inner = tuple(i for i, p in enumerate(points) if dist2(p, center) < radius2)
        shell = tuple(i for i, p in enumerate(points) if dist2(p, center) == radius2)
        if len(inner) + len(canonical) <= k + 1:
            out.append((center, radius2, canonical, inner, shell))
    return tuple(sorted(out, key=lambda b: (b[1], b[2])))


def geometric_table(points, rows):
    return tuple(sorted((c, r, len(s), tuple(sorted(points[i] for i in inner)),
                         tuple(sorted(points[i] for i in shell))) for c, r, s, inner, shell in rows))


def main():
    before = json.loads((ROOT / 'SOURCE_BEFORE.json').read_text())
    for row in before['files']:
        data = (ROOT / 'source' / row['path']).read_bytes()
        check(len(data) == row['bytes'] and sha256(data).hexdigest() == row['sha256'],
              'source_hashes', 'captured source changed')
    triangle = ((10, 5, 5), (2, 9, 5), (2, 1, 5))
    central = (5, 5, 5)
    fourth = (5, 5, 11)
    base = triangle + (fourth, central)
    tri = circumsphere(triangle)
    tetra = circumsphere(triangle + (fourth,))
    check(tri[:2] == ((F(5),) * 3, F(25)), 'target_geometry', 'triangle sphere changed')
    check(tetra[:2] == ((F(5), F(5), F(71, 12)), F(3721, 144))
          and all(w > 0 for w in tetra[2]), 'target_geometry', 'strict q4 fixture changed')
    check(witnesses(base, (0, 1, 2)) == (4,), 'common_witnesses', 'planar witness missing')
    for t in (F(-3), F(0), F(11, 12), F(7)):
        c = (F(5), F(5), F(5) + t)
        radius2 = F(25) + t * t
        check(dist2(central, c) - radius2 == -25, 'family_invariance', 'planar power varies along center line')
    full_base = all_groups(base)
    rows3, rows4 = catalogue(base, full_base, 3), catalogue(base, full_base, 4)
    check(any(row[:2] == tri[:2] and len(row[3]) == 1 for row in rows3),
          'distinct_thresholds', 'admitted q3 was lost at K3')
    check(not any(row[:2] == tetra[:2] for row in rows3)
          and any(row[:2] == tetra[:2] and len(row[3]) == 1 for row in rows4),
          'distinct_thresholds', 'q4 threshold is not K-3')
    # Ordinary q3 interiors off the plane are NOT common interiors of q4 extensions.
    high_triangle = tuple((x, y, 50) for x, y, z in triangle)
    off_plane = (5, 5, 51)
    lower_fourth = (5, 5, 9)
    off_case = high_triangle + (lower_fourth, off_plane)
    off_tri = circumsphere(high_triangle)
    off_tetra = circumsphere(high_triangle + (lower_fourth,))
    check(dist2(off_plane, off_tri[0]) - off_tri[1] == -24,
          'off_plane_scope', 'off-plane point not initially interior')
    check(all(w > 0 for w in off_tetra[2])
          and dist2(off_plane, off_tetra[0]) - off_tetra[1] == F(672, 41),
          'off_plane_scope', 'off-plane point did not leave the strict q4 ball')
    check(witnesses(off_case, (0, 1, 2)) == (),
          'off_plane_scope', 'off-plane interior was counted as common')
    # An obtuse triple can still support a strict q4; planar midpoint remains a common witness.
    obtuse = ((5, 2, 1), (10, 5, 5), (9, 8, 5))
    obtuse_case = obtuse + ((1, 5, 8), (7, 5, 3))
    check(any(w < 0 for w in circumsphere(obtuse)[2])
          and all(w > 0 for w in circumsphere(obtuse_case[:4])[2]),
          'obtuse_preserved', 'obtuse extension guard missing')
    check(witnesses(obtuse_case, (0, 1, 2)) == (4,),
          'obtuse_preserved', 'midpoint common witness missing for obtuse prefix')
    # Strict q4 center in hull does not determine global I or admission.
    regular = ((0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2))
    regular_ball = circumsphere(regular)
    regular_extra = regular + ((1, 1, 1),)
    check(all(w > 0 for w in regular_ball[2]) and regular_ball[:2] == ((F(1),) * 3, F(3)),
          'hull_not_census', 'regular fixture changed')
    check(any(row[:2] == regular_ball[:2] for row in catalogue(regular, all_groups(regular), 3))
          and not any(row[:2] == regular_ball[:2] for row in catalogue(regular_extra, all_groups(regular_extra), 3)),
          'hull_not_census', 'interior site did not change admission')
    # Materialization requires the shell anchor, not an arbitrary site with the same center.
    anchor = triangle[0]
    denominator = 12
    numerator = tuple(denominator * (c - a) for c, a in zip(tetra[0], anchor))
    check(dot(numerator, numerator) / (denominator * denominator) == tetra[1],
          'level_anchor', 'deferred q4 Level identity failed')
    check(dist2(tetra[0], central) == F(121, 144) != tetra[1],
          'level_anchor', 'interior anchor could reconstruct true Level')
    permutations = 0
    for points in (base, off_case, obtuse_case, regular_extra):
        reference = {k: geometric_table(points, catalogue(points, all_groups(points), k)) for k in range(1, 6)}
        orders = [points[i:] + points[:i] for i in range(len(points))] + [tuple(reversed(points))]
        for order in orders:
            full = all_groups(order)
            for k in range(1, 6):
                expected = catalogue(order, full, k)
                reduced = catalogue(order, all_groups(order, k), k)
                check(reduced == expected, 'filtered_catalogue', 'family rejection lost S* or I/U')
                check(geometric_table(order, expected) == reference[k],
                      'permutation_geometry', 'geometric catalogue changed under permutation')
            permutations += 1
    after = json.loads((ROOT / 'SOURCE_AFTER.json').read_text())
    check(after['captured_files'] == [{'path': row['path'], 'bytes': row['bytes'], 'sha256': row['sha256']}
                                     for row in before['files']], 'source_hashes', 'copy closure mismatch')
    manifest = ROOT / 'SHA256SUMS'
    if manifest.exists():
        for line in manifest.read_text().splitlines():
            wanted, name = line.split('  ', 1)
            path = Path(name)
            if path.is_absolute() or '..' in path.parts or sha256((ROOT / path).read_bytes()).hexdigest() != wanted:
                raise ValueError('manifest mismatch: ' + name)
    print(json.dumps({'checks': COUNTS, 'total_checks': sum(COUNTS.values()),
                      'permuted_cases': permutations, 'native_executions': 0,
                      'target_q3_beta': '25', 'target_q4_beta': '3721/144',
                      'off_plane_power_in_q4': '672/41'}, sort_keys=True))


if __name__ == '__main__':
    main()
