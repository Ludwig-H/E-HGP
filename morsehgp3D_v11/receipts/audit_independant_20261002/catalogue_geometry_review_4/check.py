"""Certificats geometriques rationnels ; aucun import/execution du produit."""
from fractions import Fraction as F
from hashlib import sha256
from itertools import combinations, product
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
COUNTS = {}


def require(condition, group, message):
    COUNTS[group] = COUNTS.get(group, 0) + 1
    if not condition:
        raise ValueError(group + ': ' + message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dist2(a, b):
    return dot(sub(a, b), sub(a, b))


def solve(matrix, rhs):
    rows = [list(map(F, row)) + [F(value)] for row, value in zip(matrix, rhs)]
    size = len(rows)
    for column in range(size):
        pivot = next((i for i in range(column, size) if rows[i][column]), None)
        if pivot is None:
            return None
        rows[column], rows[pivot] = rows[pivot], rows[column]
        scale = rows[column][column]
        rows[column] = [value / scale for value in rows[column]]
        for i in range(size):
            if i != column:
                scale = rows[i][column]
                rows[i] = [a - scale * b for a, b in zip(rows[i], rows[column])]
    return tuple(row[-1] for row in rows)


def sphere(points):
    anchor = points[0]
    vectors = [sub(p, anchor) for p in points[1:]]
    weights = solve([[dot(a, b) for b in vectors] for a in vectors],
                    [F(dot(v, v), 2) for v in vectors])
    if weights is None:
        return None
    center = tuple(F(anchor[a]) + sum(w * v[a] for w, v in zip(weights, vectors)) for a in range(3))
    return center, dist2(center, anchor), (1 - sum(weights),) + weights


def critical_balls(points):
    presentations = {}
    for q in (2, 3, 4):
        for support in combinations(range(len(points)), q):
            value = sphere([points[i] for i in support])
            if value is not None and all(w > 0 for w in value[2]):
                presentations.setdefault(value[:2], []).append(support)
    out = []
    for (center, radius2), supports in presentations.items():
        inner = tuple(i for i, p in enumerate(points) if dist2(p, center) < radius2)
        shell = tuple(i for i, p in enumerate(points) if dist2(p, center) == radius2)
        canonical = min(supports, key=lambda support: (len(support), support))
        out.append((center, radius2, inner, shell, canonical, tuple(supports)))
    return out


def corners(box):
    return tuple(product(*[(lo, hi) for lo, hi in box]))


def halfopen(center, box):
    return all(lo <= c < hi for c, (lo, hi) in zip(center, box))


def envelope(points):
    return tuple((min(p[a] for p in points), max(p[a] for p in points) + 1) for a in range(3))


def subboxes(box):
    axes = []
    for lo, hi in box:
        middle = lo + (hi - lo) // 2
        axes.append(((lo, middle), (middle, hi)) if lo < middle < hi else ((lo, hi),))
    return tuple(product(*axes))


def minimum_gap(witness, candidate, box):
    # Definition independent of the native affine expression: all closed vertices.
    return min(dist2(candidate, c) - dist2(witness, c) for c in corners(box))


def affine_gap(witness, candidate, box):
    constant = sum((x - lo) ** 2 - (y - lo) ** 2
                   for x, y, (lo, hi) in zip(candidate, witness, box))
    maximum = sum(max(0, 2 * (hi - lo) * (x - y))
                  for x, y, (lo, hi) in zip(candidate, witness, box))
    return constant - maximum


def filtered(points, parent, witnesses, box, k):
    return tuple(i for i in parent
                 if sum(minimum_gap(points[j], points[i], box) > 0 for j in witnesses) < k)


def nearest_with_ties(points, center, k):
    distances = sorted(dist2(p, center) for p in points)
    cutoff = distances[min(k, len(points)) - 1]
    return {i for i, p in enumerate(points) if dist2(p, center) <= cutoff}


def certificates():
    fixtures = {
        'line': ((0, 0, 0), (2, 0, 0), (3, 0, 0), (7, 0, 0), (9, 0, 0)),
        'cube_with_center': tuple(product((0, 4), repeat=3)) + ((2, 2, 2),),
        'strict_q4_obtuse_prefix': ((5, 2, 1), (10, 5, 5), (9, 8, 5), (1, 5, 8)),
        'q3_q4_shell': ((10, 5, 5), (8, 9, 5), (2, 9, 5), (1, 2, 5),
                        (9, 2, 5), (5, 5, 10), (8, 5, 1)),
    }
    totals = {}
    for name, points in fixtures.items():
        root = envelope(points)
        boxes = subboxes(root)
        balls = critical_balls(points)
        totals[name] = len(balls)
        all_sites = tuple(range(len(points)))
        for box in (root,) + boxes:
            for x in points:
                for y in points:
                    require(affine_gap(y, x, box) == minimum_gap(y, x, box),
                            'closed_dominance', 'affine minimum differs from vertex definition')
        for center, radius2, inner, shell, canonical, supports in balls:
            owners = [box for box in boxes if halfopen(center, box)]
            require(halfopen(center, root) and len(owners) == 1, 'ownership', 'critical center lost or duplicated')
            box = owners[0]
            require(halfopen(center, envelope([points[i] for i in canonical])),
                    'ownership', 'convex support envelope lost center')
            for q in (2, 3, 4):
                for support in combinations(shell, q):
                    value = sphere([points[i] for i in support])
                    if value is not None and all(w > 0 for w in value[2]) and value[:2] == (center, radius2):
                        require(support in supports, 'canonical', 'global presentation missing')
            for support in supports:
                require(not (len(shell) == len(support)) or support == canonical,
                        'canonical', 'm=q shortcut would bypass a smaller/earlier support')
            for k in (1, 2, 3, 4, 5, 12):
                for order in (all_sites, tuple(reversed(all_sites))):
                    parent = filtered(points, all_sites, order[:3 * k], root, k)
                    witnesses = tuple(i for i in order if i in parent)[:3 * k]
                    kept = filtered(points, parent, witnesses, box, k)
                    require(tuple(sorted(kept)) == kept, 'sorted_lists', 'filter did not preserve SiteIdx order')
                    samples = (center,) + corners(box) + (tuple(F(lo + hi, 2) for lo, hi in box),)
                    for sample in samples:
                        require(nearest_with_ties(points, sample, k) <= set(kept),
                                'knn_with_ties', 'global K-nearest or tie eliminated')
                    if len(inner) < k:
                        require(set(inner + shell) <= set(kept), 'global_census', 'closed ball not retained')
                    else:
                        require(len(set(inner) & set(kept)) >= k, 'global_census', 'fewer than K strict interiors retained')
                    if len(inner) + len(canonical) <= k + 1:
                        dominators = set()
                        for r, vertex in enumerate(canonical, start=1):
                            dominators |= {j for j in kept if minimum_gap(points[j], points[vertex], box) > 0}
                            require(dominators <= set(inner) and len(dominators) <= k + 1 - r,
                                    'canonical_prefix', 'canonical prefix can be pruned')
                        require(len(inner) <= k + 1 - len(canonical) <= k - 1,
                                'accepted_census', 'canonical census threshold incorrect')
        if name == 'strict_q4_obtuse_prefix':
            value = sphere(points)
            prefix = sphere(points[:3])
            require(value[:2] == ((F(5),) * 3, F(25)) and all(w > 0 for w in value[2]),
                    'arity_guards', 'q4 witness changed')
            require(prefix is not None and any(w < 0 for w in prefix[2]),
                    'arity_guards', 'q4 requires an obtuse q3 prefix')
        if name == 'q3_q4_shell':
            target = next(ball for ball in balls if ball[:2] == ((F(5),) * 3, F(25)))
            require(len(target[4]) == 3 and len(target[3]) == 7
                    and any(len(s) == 4 for s in target[5]),
                    'arity_guards', 'mixed positive presentations missing')
    # Strict equality at a closed boundary cannot certify removal.
    tied_box = ((1, 2), (0, 1), (0, 1))
    require(minimum_gap((0, 0, 0), (2, 0, 0), tied_box) == -4
            and minimum_gap((2, 0, 0), (0, 0, 0), tied_box) == 0,
            'boundary_tie', 'wrong tie witness')
    require(filtered(((0, 0, 0), (2, 0, 0)), (0, 1), (0, 1), tied_box, 1) == (0, 1),
            'boundary_tie', 'closed-boundary tie removed')
    return totals


def limits():
    for bits in (18, 21, 24):
        require(2 * bits + 5 <= 63 and 5 * bits + 6 <= 127,
                'bit_domains', 'published domain exceeds signed native capacity')
        for width in (1, 2, 3, 5, 7, (1 << bits) - 1, 1 << bits):
            if width > 1:
                parent = (width - 1).bit_length()
                require(max((width // 2 - 1).bit_length(), ((width + 1) // 2 - 1).bit_length()) < parent,
                        'depth_potential', 'integer split does not reduce ceil(log2 width)')
    require(not (2 * 32 + 5 <= 63) and not (5 * 32 + 6 <= 127),
            'bit_domains', 'u32 would incorrectly inherit these expressions')
    none = (1 << 32) - 1
    for n in (0, 1, none - 1):
        require(n < none and n + 1 <= none and n < (1 << 64),
                'virtual_cardinality', 'rank/offset dimension outside domain')
    for size in (1, 64, 65, 256, 1024):
        words = (size + 63) // 64
        require(words <= 16 and size * words <= 1024 * 16,
                'virtual_cardinality', 'leaf dominance storage exceeds declared capacity')


def source_hashes():
    before = json.loads((HERE / 'SOURCE_BEFORE.json').read_text())
    after = json.loads((HERE / 'SOURCE_AFTER.json').read_text())
    require(before['commit'] == after['commit'] == 'f391bf13e1a9a982025bde86fc9219b5b7430afc',
            'source_hashes', 'commit mismatch')
    require(before['files'] == after['files'], 'source_hashes', 'source closure changed')
    for row in before['files']:
        data = (HERE / 'source' / row['path']).read_bytes()
        require(len(data) == row['bytes'] and sha256(data).hexdigest() == row['sha256'],
                'source_hashes', 'captured source mismatch: ' + row['path'])
    return len(before['files'])


def manifest_if_present():
    manifest = HERE / 'SHA256SUMS'
    if not manifest.exists():
        return
    for line in manifest.read_text().splitlines():
        wanted, name = line.split('  ', 1)
        relative = Path(name)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('invalid manifest path')
        if sha256((HERE / relative).read_bytes()).hexdigest() != wanted:
            raise ValueError('manifest mismatch: ' + name)


def main():
    files = source_hashes()
    totals = certificates()
    limits()
    manifest_if_present()
    print(json.dumps({'critical_balls': totals, 'checks': COUNTS, 'total_checks': sum(COUNTS.values()),
                      'source_files': files, 'native_executions': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
