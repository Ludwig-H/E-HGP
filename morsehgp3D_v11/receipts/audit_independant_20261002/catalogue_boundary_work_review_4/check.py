"""Coquilles entieres et travail local : geometrie autonome, aucun import produit."""
from itertools import combinations
from math import comb, isqrt
import json


def require(condition, label):
    if not condition:
        raise RuntimeError(label)


def sphere_sites(radius):
    sites = []
    for x in range(-radius, radius + 1):
        for y in range(-radius, radius + 1):
            square = radius * radius - x * x - y * y
            if square < 0:
                continue
            z = isqrt(square)
            if z * z == square:
                for signed in ((0,) if z == 0 else (-z, z)):
                    sites.append((x, y, signed))
    return tuple(sorted(sites))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def orient(a, b, c, d):
    return dot(cross(sub(b, a), sub(c, a)), sub(d, a))


def acute(a, b, c):
    return all(dot(sub(q, p), sub(r, p)) > 0
               for p, q, r in ((a, b, c), (b, a, c), (c, a, b)))


def inside_tetrahedron(vertices):
    for omitted in range(4):
        face = [vertices[i] for i in range(4) if i != omitted]
        if orient(*face, (0, 0, 0)) * orient(*face, vertices[omitted]) <= 0:
            return False
    return True


def owner_path(radius, number, leaf_size=32):
    lo, hi = [0, 0, 0], [2 * radius + 1] * 3
    history = []
    while number > leaf_size:
        axis = max(range(3), key=lambda i: hi[i] - lo[i])
        width = hi[axis] - lo[axis]
        if width == 1:
            break
        middle = lo[axis] + width // 2
        if radius < middle:
            hi[axis] = middle
        else:
            lo[axis] = middle
        require(all(lo[i] <= radius < hi[i] for i in range(3)), 'centre proprietaire')
        history.append({'lo': lo[:], 'hi': hi[:], 'axis': axis})
    return history, lo, hi


def no_dominance(points, radius, lo, hi):
    # Minimum de ||candidate-c||^2-||witness-c||^2 sur la boite FERMEE.
    translated = [tuple(x + radius for x in p) for p in points]
    for candidate in translated:
        for witness in translated:
            minimum = sum(candidate[i] ** 2 - witness[i] ** 2
                          - 2 * (candidate[i] - witness[i])
                          * (hi[i] if candidate[i] > witness[i] else lo[i])
                          for i in range(3))
            require(minimum <= 0, 'aucun dominateur strict sur une boite contenant le centre')


rows = []
for radius, number in ((5, 30), (15, 150), (35, 270)):
    points = sphere_sites(radius)
    require(len(points) == number and len(set(points)) == number, 'sites exacts distincts')
    require(all(dot(p, p) == radius * radius for p in points), 'coquille exacte')
    require(2 * radius < 2 ** 18, 'entree u18')
    history, lo, hi = owner_path(radius, number)
    no_dominance(points, radius, [0, 0, 0], [2 * radius + 1] * 3)
    for box in history:
        no_dominance(points, radius, box['lo'], box['hi'])
    row = {'radius': radius, 'sites': number, 'owner_box': {'lo': lo, 'hi': hi},
           'owner_depth': len(history), 'default_wide_leaf': number > 256,
           'prefixes_if_enumerated_one_pass': sum(comb(number, q) for q in range(1, 5)),
           'q4_prefixes_if_enumerated_one_pass': comb(number, 4)}
    if radius == 5:
        counts = {2: sum(a == tuple(-x for x in b) for a, b in combinations(points, 2)),
                  3: sum(orient(a, b, c, (0, 0, 0)) == 0 and acute(a, b, c)
                         for a, b, c in combinations(points, 3)),
                  4: sum(inside_tetrahedron(vertices) for vertices in combinations(points, 4))}
        require(counts == {2: 15, 3: 120, 4: 1560}, 'presentations centrales')
        row.update(central_presentations=counts, central_interior=0, central_shell=number,
                   central_census_tests_one_pass=number * sum(counts.values()),
                   central_noncanonical_presentations_one_pass=sum(counts.values()) - 1)
        for k in (5, 10):
            require(all(k + 1 - q >= 0 for q in counts), 'seuils census compatibles')
        require(row['prefixes_if_enumerated_one_pass'] == 31930, 'prefixes 30 sites')
    if radius == 15:
        require(set(tuple(3 * x for x in p) for p in sphere_sites(5)) <= set(points),
                'sous-coquille de trente sites')
        require(row['prefixes_if_enumerated_one_pass'] == 20822900, 'prefixes 150 sites')
    rows.append(row)

print(json.dumps({'status': 'PASS', 'scope': 'exact local mathematical work, no native execution or timing',
                  'families': rows}, sort_keys=True, indent=2))
