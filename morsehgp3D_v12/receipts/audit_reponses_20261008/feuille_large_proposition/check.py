#!/usr/bin/env python3
"""Oracle rationnel borné de supports croisés ; aucune invocation produit."""
import argparse
from fractions import Fraction as F
from functools import lru_cache
import hashlib
import itertools as it
import json
import math
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def solve(matrix, rhs):
    n = len(rhs); rows = [[F(v) for v in row] + [F(b)] for row, b in zip(matrix, rhs)]
    for col in range(n):
        pivot = next((r for r in range(col, n) if rows[r][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        a = rows[col][col]; rows[col] = [v / a for v in rows[col]]
        for r in range(n):
            if r != col:
                a = rows[r][col]; rows[r] = [x - a * y for x, y in zip(rows[r], rows[col])]
    return tuple(row[-1] for row in rows)


@lru_cache(None)
def ball(points):
    """Centre dans l'enveloppe affine et poids stricts, système de Gram exact."""
    anchor = points[0]; vectors = [sub(p, anchor) for p in points[1:]]
    alpha = solve([[dot(a, b) for b in vectors] for a in vectors], [F(dot(v, v), 2) for v in vectors])
    if alpha is None or any(v <= 0 for v in (1 - sum(alpha),) + alpha):
        return None
    c = tuple(F(anchor[j]) + sum(t * v[j] for t, v in zip(alpha, vectors)) for j in range(3))
    radius = dot(sub(c, anchor), sub(c, anchor))
    return (c, radius) if radius > 0 else None


def owned(c, box):
    return all(F(lo) <= x < F(hi) for x, (lo, hi) in zip(c, box))


def side(point, geometry):
    c, radius = geometry; value = dot(sub(point, c), sub(point, c)) - radius
    return (value > 0) - (value < 0)


def morton(p):
    return sum(((v >> b) & 1) << (3 * b + axis) for axis, v in enumerate(p) for b in range(21))


def ordered(points):
    return tuple(sorted(points, key=morton))


def canonical(points, shell, geometry):
    by_position = sorted(shell, key=lambda i: points[i])
    for q in range(2, 5):
        for ids in it.combinations(by_position, q):
            if ball(tuple(points[i] for i in ids)) == geometry:
                return tuple(sorted(ids))
    raise ValueError('coquille sans support strict')


def census(points, ids, geometry, k, q, early_shell=False):
    """Stockage I borné à theta+1, U à64 ; dépassement U différé."""
    interior = []; shell = []; overflow = False; contacts_before_inside = 0
    for i in ids:
        s = side(points[i], geometry)
        if s < 0:
            interior.append(i)
            if len(interior) > k + 1 - q:
                return 'rejet', (), (), contacts_before_inside
        elif s == 0:
            if not interior:
                contacts_before_inside += 1
            if len(shell) < 64:
                shell.append(i)
            else:
                overflow = True
                if early_shell:
                    return 'shell_capacity', (), (), contacts_before_inside
    if overflow:
        return 'shell_capacity', (), (), contacts_before_inside
    return 'complet', tuple(interior), tuple(shell), contacts_before_inside


def tuples(n, q, block):
    """Bloc de travail du premier rang seulement ; les autres rangs restent globaux."""
    def tail(prefix):
        if len(prefix) == q:
            yield prefix
        else:
            for j in range(prefix[-1] + 1, n):
                yield from tail(prefix + (j,))
    for first in range(0, n, block):
        for i in range(first, min(first + block, n)):
            yield from tail((i,))


def oracle(points, k, box):
    """Toutes les boules géométriques, puis groupement par centre/rayon et census global."""
    geometries = set()
    for q in range(2, 5):
        for ids in it.combinations(range(len(points)), q):
            b = ball(tuple(points[i] for i in ids))
            if b is not None and owned(b[0], box):
                geometries.add(b)
    result = {}
    for b in geometries:
        interior = tuple(i for i, p in enumerate(points) if side(p, b) < 0)
        shell = tuple(i for i, p in enumerate(points) if side(p, b) == 0)
        support = canonical(points, shell, b)
        if len(interior) + len(support) <= k + 1:
            result[b] = (support, interior, shell)
    return result


def streamed(points, k, box, block, candidates=None):
    candidates = list(range(len(points))) if candidates is None else candidates
    emitted = []; result = {}
    for q in range(2, min(4, k + 1) + 1):
        for local in tuples(len(candidates), q, block):
            ids = tuple(candidates[i] for i in local)
            b = ball(tuple(points[i] for i in ids))
            if b is None or not owned(b[0], box):
                continue
            status, interior, shell, _ = census(points, candidates, b, k, q)
            if status == 'rejet':
                continue
            need(status == 'complet', 'petite fixture hors coquille')
            support = canonical(points, shell, b)
            if support == ids:
                emitted.append(b); result[b] = (support, interior, shell)
    need(len(emitted) == len(result), 'doublon géométrique')
    return result


def certified_list(points, k, box):
    """G1 complet par coins : témoin indépendant, pas reproduction du réservoir natif."""
    corners = list(it.product(*(pair for pair in box)))
    result = []
    for i, p in enumerate(points):
        dominates = sum(all(dot(sub(q, c), sub(q, c)) < dot(sub(p, c), sub(p, c)) for c in corners)
                        for j, q in enumerate(points) if j != i)
        if dominates < k:
            result.append(i)
    return result


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--repo', type=Path, required=True); args = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    for name, digest in cap['sources'].items():
        raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', cap['pin'] + ':' + name])
        need(hashlib.sha256(raw).hexdigest() == digest, 'source différente')
    square = [(1, 1, 1), (1, 3, 1), (3, 1, 1), (3, 3, 1)]
    fixtures = [square, square + [(2, 2, 1)], [(1, 2, 2), (2, 1, 2), (2, 2, 1)],
                [(1, 1, 1), (1, 3, 3), (3, 1, 3), (3, 3, 1)],
                [(i, 1, 1) for i in range(1, 6)], list(it.product((1, 3), repeat=3))]
    # Octaèdre cosphérique (trois diamètres), sans accès à un jeu de données.
    fixtures.append([tuple(4 + d if a == axis else 4 for a in range(3)) for axis in range(3) for d in (-3, 3)])
    comparisons = reductions = supports = 0
    boxes = [((0, 9), (0, 9), (0, 9)), ((0, 2), (0, 9), (0, 9)), ((2, 9), (0, 9), (0, 9))]
    for raw in fixtures:
        points = ordered(tuple(map(tuple, raw)))
        for q in range(2, 5):
            wanted = list(it.combinations(range(len(points)), q))
            for block in (1, 2, 3):
                got = list(tuples(len(points), q, block))
                need(got == wanted, 'supports croisés absents ou dupliqués'); supports += len(got)
        for k, box in it.product(range(1, 6), boxes):
            want = oracle(points, k, box)
            for block in (1, 2, 3):
                need(streamed(points, k, box, block) == want, 'catalogue différent'); comparisons += 1
            ids = certified_list(points, k, box)
            need(streamed(points, k, box, 2, ids) == want, 'census K-certifié différent')
            reductions += len(ids) < len(points)
    p = ordered(square); central = ((F(2), F(2), F(1)), F(2)); box = boxes[0]
    full = oracle(p, 1, box)
    blocks = [[i for i, point in enumerate(p) if point[0] == x] for x in (1, 3)]
    need(central in full and all(central not in streamed(p, 1, box, 1, b) for b in blocks), 'témoin croisé')
    need(not owned(central[0], boxes[1]) and owned(central[0], boxes[2]), 'frontière demi-ouverte')
    need(all(boxes[1][j][0] <= c <= boxes[1][j][1] for j, c in enumerate(central[0])), 'mutant frontière fermée')
    sphere = [tuple(16 + x for x in p) for p in it.product(range(-7, 8), repeat=3) if dot(p, p) == 50]
    inner = [(16, 16, 16), (17, 16, 16), (16, 17, 16), (16, 16, 17), (17, 17, 17)]
    b = ((F(16),) * 3, F(50)); points = ordered(sphere + inner)
    need(len(sphere) == 84, 'sphère entière')
    full_census = census(points, range(len(points)), b, 5, 2)
    need(full_census[0] == 'rejet' and full_census[3] == 69, 'priorité du rejet intérieur')
    need(census(points, range(len(points)), b, 5, 2, True)[0] == 'shell_capacity', 'mutant coquille précoce')
    need(census(ordered(sphere), range(84), b, 5, 2)[0] == 'shell_capacity', 'vraie limite coquille')
    # 253 sites extérieurs puis le carré en Morton : le support global finit au rang256.
    translated = [(x + 9998, y + 9998, z + 9999) for x, y, z in square]
    big = ordered([(i, 0, 0) for i in range(253)] + translated)
    geometry = ((F(10000),) * 3, F(2))
    status, interior, shell, _ = census(big, range(257), geometry, 1, 2)
    support = canonical(big, shell, geometry)
    need(status == 'complet' and not interior and len(shell) == 4 and support == (253, 256), 'rang256')
    need(tuple(i & 255 for i in support) != support, 'mutant indice octet')
    print(json.dumps(dict(small_fixtures=len(fixtures), catalogues_compared=comparisons,
                         certified_list_cases=len(fixtures) * 15, strict_list_reductions=reductions,
                         support_tuples_compared=supports, sphere_contacts=84, contacts_before_first_inside=69,
                         shell65_then_five_interiors='rejet', shell84_no_interior='shell_capacity',
                         candidate257_support=list(support),
                         model_mutants=['blocs_geometriques','frontiere_fermee','coquille65_immediate','indice_u8'],
                         support_bound257=sum(math.comb(257, q) for q in range(2, 5)),
                         native_execution=False, gpu_execution=False), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
