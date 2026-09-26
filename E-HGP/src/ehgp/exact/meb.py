"""Boule englobante minimale exacte, libre en dimension.

Toutes les decisions sont prises en arithmetique rationnelle exacte
(`fractions.Fraction`). Aucun flottant n'intervient dans une comparaison
certifiante : les flottants ne servent qu'aux filtres des modules `soft/`
et `bracket/`, jamais ici.

Convention de niveau (identique a `docs/SPECIFICATION_MORSEHGP3D.md` § 2) :
le niveau d'un ensemble fini non vide $A$ est le RAYON AU CARRE de sa plus
petite boule englobante,

    beta(A) = min_y max_{x in A} ||y - x||^2 .

Le support d'une boule englobante minimale peut toujours etre choisi
affinement independant et de cardinal au plus min(|A|, d + 1) (Welzl).
Le module enumere donc les supports candidats, ce qui reste borne pour le
domaine d'oracle (|A| <= 11 et n <= 14) et ne suppose rien sur d.
"""

from fractions import Fraction
from itertools import combinations


def to_rational_point(point):
    """Convertit un point en tuple de `Fraction`."""
    return tuple(Fraction(coordinate) for coordinate in point)


def to_rational_cloud(cloud):
    """Convertit un nuage en tuple de points rationnels de meme dimension."""
    points = tuple(to_rational_point(point) for point in cloud)
    if not points:
        raise ValueError("nuage vide")
    dimension = len(points[0])
    if dimension == 0:
        raise ValueError("dimension nulle")
    for point in points:
        if len(point) != dimension:
            raise ValueError("dimensions heterogenes")
    return points


def squared_distance(left, right):
    """Distance au carre exacte entre deux points rationnels."""
    total = Fraction(0)
    for a, b in zip(left, right):
        difference = a - b
        total += difference * difference
    return total


def _solve_exact(matrix, vector):
    """Resout `matrix @ x = vector` par elimination de Gauss exacte.

    Renvoie `None` si la matrice est singuliere (support affinement
    dependant : il est alors couvert par un sous-support independant).
    """
    size = len(vector)
    rows = [list(matrix[index]) + [vector[index]] for index in range(size)]
    for column in range(size):
        pivot_row = None
        for row_index in range(column, size):
            if rows[row_index][column] != 0:
                pivot_row = row_index
                break
        if pivot_row is None:
            return None
        rows[column], rows[pivot_row] = rows[pivot_row], rows[column]
        pivot = rows[column][column]
        for row_index in range(column + 1, size):
            factor = rows[row_index][column]
            if factor == 0:
                continue
            ratio = factor / pivot
            for inner in range(column, size + 1):
                rows[row_index][inner] -= ratio * rows[column][inner]
    solution = [Fraction(0)] * size
    for column in range(size - 1, -1, -1):
        accumulator = rows[column][size]
        for inner in range(column + 1, size):
            accumulator -= rows[column][inner] * solution[inner]
        solution[column] = accumulator / rows[column][column]
    return solution


def circumsphere(points):
    """Sphere circonscrite d'un support dans son enveloppe affine.

    Renvoie `(centre, rayon_carre, barycentriques)` ou `None` si les points
    sont affinement dependants. Le centre est l'unique point de
    `aff(points)` equidistant de tous les points ; les barycentriques sont
    ses coordonnees dans le repere affine de `points`.
    """
    base = points[0]
    directions = [tuple(p - b for p, b in zip(point, base)) for point in points[1:]]
    size = len(directions)
    if size == 0:
        return base, Fraction(0), [Fraction(1)]
    gram = [[Fraction(0)] * size for _ in range(size)]
    right = [Fraction(0)] * size
    for i in range(size):
        for j in range(i, size):
            value = sum(a * b for a, b in zip(directions[i], directions[j]))
            gram[i][j] = value
            gram[j][i] = value
        right[i] = gram[i][i] / 2
    weights = _solve_exact(gram, right)
    if weights is None:
        return None
    center = list(base)
    for weight, direction in zip(weights, directions):
        for axis, component in enumerate(direction):
            center[axis] += weight * component
    center = tuple(center)
    radius_squared = squared_distance(center, base)
    barycentric = [Fraction(1) - sum(weights)] + list(weights)
    return center, radius_squared, barycentric


def minimum_enclosing_ball(points):
    """Boule englobante minimale exacte d'un ensemble fini de points.

    Renvoie `(centre, rayon_carre, support)` ou `support` est le tuple des
    indices (dans `points`) d'un support affinement independant dont la
    sphere circonscrite realise la boule, avec centre dans son enveloppe
    convexe.
    """
    cloud = to_rational_cloud(points)
    count = len(cloud)
    best = None
    for size in range(1, count + 1):
        for support in combinations(range(count), size):
            candidate = circumsphere([cloud[index] for index in support])
            if candidate is None:
                continue
            center, radius_squared, barycentric = candidate
            if any(coordinate < 0 for coordinate in barycentric):
                continue
            if best is not None and radius_squared >= best[1]:
                continue
            if any(squared_distance(center, point) > radius_squared for point in cloud):
                continue
            best = (center, radius_squared, support)
        if best is not None and size >= 1 and best[1] == 0:
            break
    if best is None:
        raise RuntimeError("aucun support valide : entree degeneree inattendue")
    return best


def beta(points):
    """Niveau exact `beta(A)` : rayon au carre de la boule englobante."""
    return minimum_enclosing_ball(points)[1]


class SupportCatalog:
    """Catalogue des supports valides d'un nuage, partage par tous les ordres.

    Un support est un sous-ensemble affinement independant dont le centre
    circonscrit appartient a son enveloppe convexe : c'est exactement un
    support possible de boule englobante minimale. Pour un tel support `S`
    de rayon carre `r2`, tout `F` avec `S ⊆ F ⊆ interieur_ferme(S)` verifie
    `beta(F) = r2`. Le catalogue en deduit `beta` de n'importe quel
    sous-ensemble sans reenumerer ses propres supports.

    Le catalogue est borne par `max_support` (au plus `K + 1` pour une tour
    d'ordre `K`), ce qui le garde hors du regime exponentiel en `d`.
    """

    def __init__(self, cloud, max_support):
        self.cloud = to_rational_cloud(cloud)
        self.count = len(self.cloud)
        self.dimension = len(self.cloud[0])
        self.max_support = min(max_support, self.count)
        self.supports = []
        for size in range(1, self.max_support + 1):
            for support in combinations(range(self.count), size):
                candidate = circumsphere([self.cloud[index] for index in support])
                if candidate is None:
                    continue
                center, radius_squared, barycentric = candidate
                if any(coordinate < 0 for coordinate in barycentric):
                    continue
                support_mask = 0
                for index in support:
                    support_mask |= 1 << index
                inside_mask = 0
                for index, point in enumerate(self.cloud):
                    if squared_distance(center, point) <= radius_squared:
                        inside_mask |= 1 << index
                self.supports.append((radius_squared, support_mask, inside_mask, support, center))
        self.supports.sort(key=lambda item: (item[0], item[1]))
        self._cache = {}

    def beta_mask(self, mask):
        """Niveau exact du sous-ensemble decrit par `mask` (bits = indices)."""
        if mask == 0:
            raise ValueError("sous-ensemble vide")
        cached = self._cache.get(mask)
        if cached is not None:
            return cached
        for radius_squared, support_mask, inside_mask, _support, _center in self.supports:
            if support_mask & ~mask:
                continue
            if mask & ~inside_mask:
                continue
            self._cache[mask] = radius_squared
            return radius_squared
        raise RuntimeError("support introuvable : augmenter max_support")

    def ball_mask(self, mask):
        """Renvoie `(centre, rayon_carre, support)` de la boule de `mask`."""
        for radius_squared, support_mask, inside_mask, support, center in self.supports:
            if support_mask & ~mask:
                continue
            if mask & ~inside_mask:
                continue
            return center, radius_squared, support
        raise RuntimeError("support introuvable : augmenter max_support")
