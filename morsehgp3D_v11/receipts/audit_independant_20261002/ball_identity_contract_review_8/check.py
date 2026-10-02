#!/usr/bin/env python3
"""Oracle rationnel autonome de contrat ; ni imports produit ni natif."""
from fractions import Fraction as F
from functools import reduce
from hashlib import sha256
from itertools import combinations, permutations
from math import gcd, lcm
from pathlib import Path
import json

HERE = Path(__file__).resolve().parent
checks = 0
presentations = 0


def require(condition, message):
    global checks
    if not condition:
        raise RuntimeError(message)
    checks += 1


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def solve(matrix, rhs):
    n = len(rhs)
    rows = [[F(v) for v in row] + [F(rhs[i])] for i, row in enumerate(matrix)]
    for col in range(n):
        pivot = next((i for i in range(col, n) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [v/scale for v in rows[col]]
        for i in range(n):
            if i != col:
                factor = rows[i][col]
                rows[i] = [v-factor*w for v, w in zip(rows[i], rows[col])]
    return tuple(row[-1] for row in rows)


def sphere(points):
    """Centre équidistant dans l'enveloppe affine via Gram, pas Cramer natif."""
    a = points[0]
    vectors = tuple(sub(p, a) for p in points[1:])
    weights = solve([[dot(u, v) for v in vectors] for u in vectors],
                    [F(dot(u, u), 2) for u in vectors])
    if weights is None:
        return None
    center = tuple(F(a[j]) + sum(w*u[j] for w, u in zip(weights, vectors))
                   for j in range(3))
    return center, dot(sub(center, a), sub(center, a)), (1-sum(weights),)+weights


def primitive(a, center, scale=1):
    """Clé dérivée de l'ancre de coquille, sans lire un rayon ou un Level."""
    offset = sub(center, a)
    d = lcm(*(v.denominator for v in offset)) * scale
    n = tuple(int(v*d) for v in offset)
    coefficients = (d,)+tuple(-2*(d*a[j]+n[j]) for j in range(3))+(d*dot(a, a)+2*dot(n, a),)
    divisor = reduce(gcd, (abs(v) for v in coefficients))
    require(d > 0 and divisor > 0, "A et PGCD positifs")
    key = tuple(v//divisor for v in coefficients)
    require(all(v == w*divisor for v, w in zip(coefficients, key)), "division exacte")
    return key


def decode(key):
    a, bx, by, bz, c = key
    center = tuple(F(-b, 2*a) for b in (bx, by, bz))
    return center, dot(center, center)-F(c, a)


def meb(points):
    """MEB petite : tous supports <=4, fallback affine explicite."""
    candidates = []
    for q in range(1, min(4, len(points))+1):
        for support in combinations(points, q):
            value = sphere(support)
            if value is None:
                continue
            center, radius2, weights = value
            if all(w > 0 for w in weights) and all(dot(sub(x, center), sub(x, center)) <= radius2 for x in points):
                candidates.append((radius2, center, q, support))
    require(bool(candidates), "MEB exhaustive existe")
    radius2, center, qmin, support = min(candidates)
    return center, radius2, qmin, support


# Même boule présentée par trois arités, dont q3 rectangle et q4 non critique.
antipodes = ((0,5,5),(10,5,5))
right = antipodes+((5,10,5),)
boundary_tetra = right+((5,5,10),)
expected = ((F(5),F(5),F(5)), F(25))
keys = set()
for support in (antipodes, right, boundary_tetra):
    for ordered in permutations(support):
        value = sphere(ordered)
        require(value is not None, "présentation affine indépendante")
        center, radius2, weights = value
        require((center, radius2) == expected, "même boule q2/q3/q4")
        key = primitive(ordered[0], center)
        require(decode(key) == expected, "clé décode centre et rayon")
        require(primitive(ordered[0], center, 7) == key, "coefficients non réduits inchangés en entrée")
        keys.add(key)
        presentations += 1
require(len(keys) == 1, "clé indépendante de l'arité et de l'ancre")
require(min(sphere(boundary_tetra)[2]) == 0, "q4 valide mais non strictement intérieur")

# qmin dépend de la coquille du nuage, jamais de l'identité de la boule.
acute = ((10,5,5),(2,9,5),(2,1,5))
qmin4_cloud = ((10,5,5),(9,8,5),(5,2,1),(1,5,8),(9,2,5))
qmins = []
for cloud, target_qmin in ((antipodes,2),(acute,3),(qmin4_cloud,4)):
    center, radius2, qmin, support = meb(cloud)
    require((center, radius2) == expected and qmin == target_qmin, "même boule, qmin distinct")
    require(primitive(support[0], center) in keys, "clé ne contient pas qmin")
    qmins.append(qmin)

small = sphere(((1,5,5),(9,5,5)))
require(small[0] == expected[0] and small[1] != expected[1], "même centre, autre rayon")
require(primitive((1,5,5), small[0]) not in keys, "rayons distincts, clés distinctes")

negative = ((2,0,0),(0,2,0),(0,0,2))
center, radius2, _ = sphere(negative)
negative_key = primitive(negative[0], center)
require(negative_key == (3,-4,-4,-4,-4), "C strictement négatif")
require(dot(center, center) < radius2, "origine intérieure")
require(decode(negative_key) == (center, radius2), "C signé nécessaire")

fallbacks = (((0,0,1),(6,0,1),(6,8,1),(0,8,1)), ((0,0,0),(2,0,0),(5,0,0)))
for cloud in fallbacks:
    require(sphere(cloud) is None, "sphère du support complet dépendant absente")
    center, radius2, qmin, support = meb(cloud)
    require(qmin == 2, "MEB utilise un sous-support exact")
    require(decode(primitive(support[0], center)) == (center, radius2), "clé après fallback affine")

obtuse = ((0,0,0),(4,0,0),(1,1,0))
circum = sphere(obtuse)
minimum = meb(obtuse)
require(min(circum[2]) < 0 and circum[1] > minimum[1], "circonsphère non MEB")
require(primitive(obtuse[0], circum[0]) != primitive(minimum[3][0], minimum[0]), "clés distinguent circonsphère et MEB")

for bits in (18,21,24):
    require(5*bits+7 <= 127, "B global tient en i128")
    require(72*(2**bits)**5 < 2**127, "C q4 natif même non critique")
    require(6*bits+8 in (116,134,152), "C q3 suit SideInt")

before = json.loads((HERE/'SOURCE_BEFORE.json').read_text())
after = json.loads((HERE/'SOURCE_AFTER.json').read_text())
require(before['commit'] == after['commit'], "commit figé")
for old, new in zip(before['files'], after['files']):
    require(old['path'] == new['path'], "mêmes sources")
    require(sha256((HERE/'source'/old['path']).read_bytes()).hexdigest()
            == old['sha256'] == new['copy_sha256'] == new['git_sha256'] == new['working_sha256'], "source intacte")
manifest=HERE/'SHA256SUMS'
if manifest.exists():
    for line in manifest.read_text().splitlines():
        digest, name = line.split('  ',1)
        require(sha256((HERE/name).read_bytes()).hexdigest()==digest, "fermeture "+name)
print(json.dumps({'status':'PASS','sources':len(before['files']),
                  'permuted_presentations':presentations,'same_ball_qmins':qmins,
                  'negative_C_key':negative_key,'affine_fallbacks':len(fallbacks)},sort_keys=True))
