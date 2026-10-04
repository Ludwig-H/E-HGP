"""Catalogue borne independant : Gram/Gauss Fraction, aucun import R2, intgeom ou module produit.

Les supports de taille 2..4 sont enumeres exhaustivement. Un centre dans l'interieur relatif a des
coordonnees barycentriques toutes strictement positives. Leur enumeration dans la coquille globale
determine q_min et S*, puis p+q_min <= K+1 decide l'admission. Ce cout exponentiel borne est un juge,
jamais une architecture proposee pour le moteur.
"""
from dataclasses import dataclass
from fractions import Fraction as F
from functools import lru_cache
from itertools import combinations


def require(condition, message):
    if not condition:
        raise ValueError(message)


def morton(point):
    """Cle de Morton sans limite a 64 bits : calcul bit par bit propre au juge."""
    return sum(((coordinate >> bit) & 1) << (3 * bit + axis)
               for axis, coordinate in enumerate(point) for bit in range(coordinate.bit_length()))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def distance2(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def solve(matrix, rhs):
    """Elimination exacte a pivot, distincte des formules Cramer/double produit vectoriel du produit."""
    size = len(rhs)
    rows = [[F(x) for x in row] + [F(value)] for row, value in zip(matrix, rhs)]
    for col in range(size):
        pivot = next((i for i in range(col, size) if rows[i][col] != 0), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        divisor = rows[col][col]
        rows[col] = [x / divisor for x in rows[col]]
        for i in range(size):
            if i != col:
                multiplier = rows[i][col]
                rows[i] = [x - multiplier * y for x, y in zip(rows[i], rows[col])]
    return tuple(rows[i][-1] for i in range(size))


def circumsphere(points):
    """Centre affine, rayon carre et poids barycentriques ; None pour une dependance affine."""
    anchor = points[0]
    edges = [tuple(x - y for x, y in zip(point, anchor)) for point in points[1:]]
    weights = solve([[dot(u, v) for v in edges] for u in edges], [F(dot(u, u), 2) for u in edges])
    if weights is None:
        return None
    center = tuple(F(anchor[j]) + sum(weight * edge[j] for weight, edge in zip(weights, edges)) for j in range(3))
    return center, distance2(center, anchor), (1 - sum(weights),) + weights


@dataclass(frozen=True)
class Ball:
    center: tuple
    level: F
    support: tuple
    inner: tuple
    shell: tuple
    presentations: tuple

    @property
    def qmin(self):
        return len(self.support)

    @property
    def p(self):
        return len(self.inner)


@lru_cache(maxsize=256)
def all_balls(points):
    """points : sites distincts en ordre de Morton ; completude par enumeration, independante des boites."""
    require(0 < len(points) <= 14, 'modele : 1..14 sites distincts seulement')
    require(len(set(points)) == len(points), 'modele : sites distincts requis')
    require(tuple(sorted(points, key=morton)) == points, 'modele : ordre Morton requis')
    candidates = {}
    for size in range(2, min(4, len(points)) + 1):
        for support in combinations(range(len(points)), size):
            sphere = circumsphere([points[i] for i in support])
            if sphere is None or not all(weight > 0 for weight in sphere[2]):
                continue
            center, level, _weights = sphere
            candidates.setdefault((center, level), []).append(support)
    out = []
    for (center, level), presentations in candidates.items():
        distances = [distance2(point, center) for point in points]
        inner = tuple(i for i, value in enumerate(distances) if value < level)
        shell = tuple(i for i, value in enumerate(distances) if value == level)
        support = min(presentations, key=lambda value: (len(value), value))
        out.append(Ball(center, level, support, inner, shell, tuple(presentations)))
    # Un vrai prefixe propre ne peut partager le rayon d'un support strict plus grand ; le remplissage sentinelle
    # fixe neanmoins explicitement l'ordre v10/v11, sans dependre de cette propriete pour le test.
    return tuple(sorted(out, key=lambda ball: (ball.level, ball.support + (2**32 - 1,) * (4 - ball.qmin))))


def catalogue(points, kmax):
    require(1 <= kmax <= 12, 'modele : K hors de 1..12')
    return tuple(ball for ball in all_balls(points) if ball.p + ball.qmin <= kmax + 1)


def prepared(records):
    """records=(x,y,z,PointId) ; l'identite du site reste separee de tous ses PointId."""
    by_position = {}
    for x, y, z, identifier in records:
        by_position.setdefault((x, y, z), []).append(identifier)
    points = tuple(sorted(by_position, key=morton))
    return points, tuple(tuple(sorted(by_position[point])) for point in points)


def expected(records, kmax):
    points, identifiers = prepared(records)
    require(all(len(ids) == 1 for ids in identifiers), 'modele : les multiplicites ne sont pas ce contrat')
    balls = catalogue(points, kmax)
    levels = sorted({F(0)} | {ball.level for ball in balls})
    ranks = {level: i for i, level in enumerate(levels)}
    encode = lambda value: [str(value.numerator), str(value.denominator)]
    return {'sites': [list(p) for p in points], 'site_ids': [list(ids) for ids in identifiers],
            'levels': [encode(level) for level in levels],
            'balls': [{'support': list(ball.support), 'qmin': ball.qmin, 'p': ball.p, 'm': len(ball.shell),
                       'inner': list(ball.inner), 'shell': list(ball.shell), 'level': encode(ball.level),
                       'rank': ranks[ball.level]} for ball in balls]}
