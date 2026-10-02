#!/usr/bin/env python3
"""Contrôles géométriques autonomes ; aucun import ni lancement du produit."""
from fractions import Fraction as Q
from itertools import combinations
import hashlib
import json
from pathlib import Path

CIRCUM_CALLS = 0


def require(value, message):
    if not value:
        raise ValueError(message)


def dot(a, b):
    return sum((x*y for x, y in zip(a, b)), Q(0))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def solve(rows, rhs):
    n = len(rhs)
    a = [[Q(v) for v in row] + [Q(v)] for row, v in zip(rows, rhs)]
    for col in range(n):
        pivot = next((i for i in range(col, n) if a[i][col]), None)
        if pivot is None:
            return None
        a[col], a[pivot] = a[pivot], a[col]
        scale = a[col][col]
        a[col] = [x/scale for x in a[col]]
        for i in range(n):
            if i != col:
                scale = a[i][col]
                a[i] = [x-scale*y for x, y in zip(a[i], a[col])]
    return tuple(a[i][-1] for i in range(n))


def circum(points):
    global CIRCUM_CALLS
    CIRCUM_CALLS += 1
    # Centre = somme w_j p_j : égalités de distances absolues et somme w_j=1.
    # Ce système barycentrique n'utilise pas les formules natives de Sphere.
    p0 = points[0]
    rows = [[2*dot(p, sub(v, p0)) for p in points] for v in points[1:]]
    rhs = [dot(v, v)-dot(p0, p0) for v in points[1:]]
    rows.append([1]*len(points))
    rhs.append(1)
    weights = solve(rows, rhs)
    if weights is None:
        return None
    center = tuple(sum((w*p[i] for w, p in zip(weights, points)), Q(0)) for i in range(3))
    radius = dot(sub(center, p0), sub(center, p0))
    require(all(dot(sub(center, p), sub(center, p)) == radius for p in points), 'circonsphère')
    return center, radius, weights


def power(ball, p):
    return dot(sub(p, ball[0]), sub(p, ball[0]))-ball[1]


def morton(p):
    bits = max(p).bit_length()
    return sum(((p[axis] >> bit) & 1) << (3*bit+axis) for bit in range(bits) for axis in range(3))


def order(points):
    return tuple(sorted(points, key=morton))


def exhaustive(points):
    points = tuple(points)
    require(1 <= len(points) <= 12 and len(set(points)) == len(points), 'partie certifiée')
    enclosures, strict = [], []
    tested = 0
    for arity in range(1, min(4, len(points))+1):
        for ids in combinations(range(len(points)), arity):
            tested += 1
            ball = circum(tuple(points[i] for i in ids))
            if ball is None or any(power(ball, p) > 0 for p in points):
                continue
            enclosures.append((ids, ball))
            if all(w > 0 for w in ball[2]):
                strict.append((ids, ball))
    require(enclosures and strict, 'existence du support strict')
    best = min(ball[1] for _, ball in enclosures)
    winners = [(ids, ball) for ids, ball in enclosures if ball[1] == best]
    require(len({ball[0] for _, ball in winners}) == 1, 'unicité de la MEB')
    require(all(ball[:2] == winners[0][1][:2] for _, ball in strict), 'strict + enclosure certifie la MEB')
    canonical = min(strict, key=lambda item: (len(item[0]), item[0]))
    require(strict[0] == canonical, 'premier accepté canonique')
    return canonical, tested, len(winners), len(strict)


def encoded(value):
    if isinstance(value, Q):
        return str(value)
    if isinstance(value, (list, tuple)):
        return [encoded(v) for v in value]
    if isinstance(value, dict):
        return {k: encoded(v) for k, v in value.items()}
    return value


def main():
    base = Path(__file__).resolve().parent
    before = json.loads((base/'SOURCE_BEFORE.json').read_text())
    for row in before['files']:
        blob = (base/'source'/row['path']).read_bytes()
        require(hashlib.sha256(blob).hexdigest() == row['sha256'], 'source figée : '+row['path'])
    cases = (
        ('singleton', ((0, 0, 0),), (0, 0, 0), Q(0), 1),
        ('demi_entier', ((0, 0, 0), (1, 1, 1)), (Q(1, 2),)*3, Q(3, 4), 2),
        ('affine_ligne', tuple((i, 0, 0) for i in range(12)), (Q(11, 2), 0, 0), Q(121, 4), 2),
        ('triangle_droit', ((0, 0, 0), (4, 0, 0), (0, 4, 0)), (2, 2, 0), Q(8), 2),
        ('triangle_obtus', ((0, 0, 0), (4, 0, 0), (1, 1, 0)), (2, 0, 0), Q(4), 2),
        ('triangle_aigu', ((0, 0, 0), (4, 0, 0), (2, 3, 0)), (2, Q(5, 6), 0), Q(169, 36), 3),
        ('prefixe_obtus_q4', ((10, 5, 5), (9, 8, 5), (5, 2, 1), (1, 5, 8)), (5, 5, 5), Q(25), 4),
        ('poids_nul_q4', ((0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 0, 2)), (2, Q(5, 6), 0), Q(169, 36), 3),
        ('support_negatif', ((1, 2, 0), (0, 5, 0), (8, 1, 0), (8, 9, 0)), (5, 5, 0), Q(25), 3),
    )
    results, presentations, orders = [], 0, 0
    for name, points, center, radius, arity in cases:
        sites = order(points)
        answer, count, minima, strict = exhaustive(sites)
        ids, ball = answer
        require(ball[:2] == (center, radius) and len(ids) == arity, 'attendu indépendant '+name)
        variants = (sites, tuple(reversed(sites)), sites[1:]+sites[:1])
        for variant in variants:
            # Le contrat trie les SiteIdx, indépendamment de l'ordre de la partie.
            same_sites = tuple(sorted(variant, key=morton))
            repeated, tested, _, _ = exhaustive(same_sites)
            require(repeated == answer, 'ordre de la partie '+name)
            presentations += tested
            orders += 1
        results.append(dict(name=name, sites=sites, support=ids, center=ball[0], beta=ball[1],
                            weights=ball[2], all_enclosure_minima=minima, strict_enclosures=strict))

    tetra = cases[6][1]
    face = circum(tetra[:3])
    full = circum(tetra)
    require(face is not None and any(w < 0 for w in face[2]), 'préfixe obtus utile')
    require(full is not None and all(w > 0 for w in full[2]), 'q4 strict après préfixe obtus')
    null = circum(cases[7][1])
    require(null is not None and null[2][-1] == 0 and all(w >= 0 for w in null[2]), 'poids q4 nul')
    require(null[:2] == (cases[7][2], cases[7][3]), 'repli q3 de la présentation q4 non stricte')

    # Un census complet p<k n'est pas un certificat d'admission dans Cat_k.
    triangle = ((0, 0, 0), (6, 0, 0), (3, 4, 0))
    cloud = triangle + ((2, 1, 0), (4, 1, 0))
    ball = exhaustive(triangle)[0][1]
    inner = tuple(p for p in cloud if power(ball, p) < 0)
    shell = tuple(p for p in cloud if power(ball, p) == 0)
    require(ball[:2] == ((3, Q(7, 8), 0), Q(625, 64)), 'MEB triangle FULL')
    qmin = len(exhaustive(shell)[0][0])
    require(len(inner) == 2 < 3 and qmin == 3 and len(inner)+qmin == 5 > 4, 'complet mais hors Cat3')
    child = exhaustive(inner + (triangle[0],))[0][1]
    require(child[:2] == ((2, Q(1, 2), 0), Q(17, 4)) and child[1] < ball[1], 'prochaine descente stricte')

    print(json.dumps(encoded(dict(status='PASS', source_commit=before['frozen_commit'], source_files=len(before['files']),
                                  fixture_sets=len(cases), reordered_parts=orders, reordered_presentation_evaluations=presentations,
                                  total_circumsphere_evaluations=CIRCUM_CALLS,
                                  native_runs=0, product_imports=0, cases=results,
                                  prefix=dict(face_weights=face[2], tetra_weights=full[2]),
                                  null_q4_weights=null[2],
                                  complete_not_catalogue=dict(center=ball[0], beta=ball[1], p=len(inner), qmin=qmin,
                                                             K=3, inner=inner, shell=shell, child_center=child[0], child_beta=child[1]))),
                     sort_keys=True, ensure_ascii=False))


if __name__ == '__main__':
    main()
