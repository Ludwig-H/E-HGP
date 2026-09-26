"""Juge independant de `pi_0(Gamma_k(a))` : graphe explicite et parcours en largeur.

Ce module recalcule le meme objet que `ehgp.exact.tower`, avec une structure
algorithmique et une arithmetique de decision volontairement autres. Les
differences sont assumees et enumerees ici, car c'est ce qui fait la valeur
d'un juge.

Structure (contre `ehgp.exact.tower`)
------------------------------------
* Aucun balayage incremental des niveaux : pour chaque couple (ordre, niveau)
  le graphe `Gamma_k(a)` est reconstruit de zero.
* Aucun union-find : les composantes sont obtenues par parcours en largeur
  (file FIFO) sur une liste d'adjacence explicite.
* Aucune coface generatrice : l'adjacence est prise a la lettre de
  `docs/SPECIFICATION_MORSEHGP3D.md` section 4, sous forme de PAIRES. Deux sommets
  `F` et `F'` de cardinal `k` sont adjacents lorsque `|F union F'| = k + 1` et
  `beta(F union F') <= a`. Les paires sont enumerees par echange d'un point
  (`F' = (F \\ {i}) union {j}`), donc sans jamais passer par la clique des
  `k + 1` sous-parties d'une coface.

Arithmetique (contre `ehgp.exact.meb`)
-------------------------------------
`meb.py` decide avec les coordonnees : matrice de Gram des vecteurs
differences, resolution, reconstruction explicite du centre, puis
barycentriques deduites de la solution.

Ici, les coordonnees ne sont lues qu'une seule fois, pour former la table
exacte des distances au carre. Ensuite, plus aucune coordonnee n'intervient
et la dimension `d` n'apparait nulle part : tout se decide sur la table.

* Sphere circonscrite d'un support `S = (s_0, ..., s_m)` : systeme BORDE de
  la matrice de distances euclidiennes, d'inconnues les barycentriques
  `w` et un multiplicateur `t`,

      pour tout i : somme_j D[s_i][s_j] w_j + t = 0 ,   somme_j w_j = 1 ,

  dont la solution donne directement `beta = -t / 2`. Ce systeme est borde,
  de taille `m + 2`, et sa matrice est singuliere si et seulement si le
  support est affinement dependant (determinant de Cayley-Menger nul) : le
  test d'independance affine n'est donc pas un rang de Gram mais la
  singularite du systeme lui-meme.
* Distance du centre `c = somme_i w_i p_{s_i}` a une observation `y`, sans
  jamais former `c` :

      ||c - y||^2 = somme_i w_i D[s_i][y] - (1/2) somme_{i,l} w_i w_l D[s_i][s_l] .

* Resolution par Gauss-Jordan a pivot COMPLET (echange de lignes et de
  colonnes), contre le pivot partiel de `meb.py`.

Deux miniballs independantes sont fournies et se jugent l'une l'autre :
`ball_by_support_scan` (balayage des supports avec conditions de
Karush-Kuhn-Tucker) et `ball_by_welzl` (recursion de Welzl exacte). La
premiere est la decision du juge ; la seconde est un temoin supplementaire,
dont les cas indecidables sont rapportes et non masques.

Toutes les comparaisons sont rationnelles exactes. Aucun flottant.
"""

from collections import deque
from fractions import Fraction
from itertools import combinations


# Fautes causales injectables dans le juge. Chacune vise UNE clause de la
# definition de la section 4, et aucune n'en vise deux a la fois :
# * `half_diameter`   : la valeur de `beta` (remplacee par le demi-diametre) ;
# * `union_cardinal`  : la clause `|F union F'| = k + 1` ;
# * `strict_level`    : la comparaison `beta <= a` (rendue stricte) ;
# * `free_swap`       : la clause `beta(F union F') <= a`, purement et
#   simplement ignoree, l'adjacence se reduisant a "partager k - 1 points" ;
# * `no_isolated`     : la forme de la partition (composantes isolees omises).
MUTANT_NONE = "none"
MUTANT_HALF_DIAMETER = "half_diameter"
MUTANT_UNION_CARDINAL = "union_cardinal"
MUTANT_STRICT_LEVEL = "strict_level"
MUTANT_FREE_SWAP = "free_swap"
MUTANT_NO_ISOLATED = "no_isolated"

MUTANTS = (
    MUTANT_HALF_DIAMETER,
    MUTANT_UNION_CARDINAL,
    MUTANT_STRICT_LEVEL,
    MUTANT_FREE_SWAP,
    MUTANT_NO_ISOLATED,
)

KNOWN_MODES = (MUTANT_NONE,) + MUTANTS


class JudgeError(RuntimeError):
    """Incoherence interne du juge : jamais un verdict sur le sujet."""


def squared_distance_table(cloud):
    """Table exacte des distances au carre, seul contact avec les coordonnees.

    La table est l'unique entree geometrique du juge : au-dela de cette
    fonction, la dimension ambiante n'apparait plus.
    """
    points = []
    for point in cloud:
        coordinates = tuple(Fraction(value) for value in point)
        if not coordinates:
            raise ValueError("dimension nulle")
        points.append(coordinates)
    if not points:
        raise ValueError("nuage vide")
    dimension = len(points[0])
    for coordinates in points:
        if len(coordinates) != dimension:
            raise ValueError("dimensions heterogenes")
    count = len(points)
    table = [[Fraction(0)] * count for _ in range(count)]
    for left in range(count):
        for right in range(left + 1, count):
            total = Fraction(0)
            for axis in range(dimension):
                difference = points[left][axis] - points[right][axis]
                total += difference * difference
            table[left][right] = total
            table[right][left] = total
    return tuple(tuple(row) for row in table)


def solve_complete_pivot(matrix, vector):
    """Gauss-Jordan a pivot complet exact ; `None` si la matrice est singuliere.

    Le pivot complet echange lignes ET colonnes : la permutation des
    inconnues est suivie explicitement, puis defaite a la restitution.
    """
    size = len(vector)
    rows = [list(matrix[index]) + [vector[index]] for index in range(size)]
    unknown_of_column = list(range(size))
    for step in range(size):
        best_magnitude = None
        best_row = None
        best_column = None
        for row in range(step, size):
            for column in range(step, size):
                value = rows[row][column]
                if value == 0:
                    continue
                magnitude = value if value > 0 else -value
                if best_magnitude is None or magnitude > best_magnitude:
                    best_magnitude = magnitude
                    best_row = row
                    best_column = column
        if best_magnitude is None:
            return None
        rows[step], rows[best_row] = rows[best_row], rows[step]
        if best_column != step:
            for row in rows:
                row[step], row[best_column] = row[best_column], row[step]
            unknown_of_column[step], unknown_of_column[best_column] = (
                unknown_of_column[best_column],
                unknown_of_column[step],
            )
        pivot = rows[step][step]
        rows[step] = [value / pivot for value in rows[step]]
        for row in range(size):
            if row == step:
                continue
            factor = rows[row][step]
            if factor == 0:
                continue
            rows[row] = [
                value - factor * reference
                for value, reference in zip(rows[row], rows[step])
            ]
    solution = [Fraction(0)] * size
    for step in range(size):
        solution[unknown_of_column[step]] = rows[step][size]
    return solution


def circumsphere_from_table(table, support):
    """Sphere circonscrite d'un support, decidee sur la seule table des distances.

    Renvoie `(rayon_carre, barycentriques)` ou `None` si le support est
    affinement dependant (systeme borde singulier).
    """
    size = len(support)
    if size == 0:
        raise ValueError("support vide")
    matrix = []
    vector = []
    for left in range(size):
        row = [table[support[left]][support[right]] for right in range(size)]
        row.append(Fraction(1))
        matrix.append(row)
        vector.append(Fraction(0))
    matrix.append([Fraction(1)] * size + [Fraction(0)])
    vector.append(Fraction(1))
    solution = solve_complete_pivot(matrix, vector)
    if solution is None:
        return None
    weights = tuple(solution[:size])
    multiplier = solution[size]
    radius_squared = -multiplier / 2
    return radius_squared, weights


def support_inner_term(table, support, weights):
    """Terme `somme_{i,l} w_i w_l D[s_i][s_l]` du developpement du centre."""
    total = Fraction(0)
    for left, left_weight in enumerate(weights):
        if left_weight == 0:
            continue
        for right, right_weight in enumerate(weights):
            if right_weight == 0:
                continue
            total += left_weight * right_weight * table[support[left]][support[right]]
    return total


def squared_distance_to_center(table, support, weights, inner, target):
    """`||c - y||^2` sans former le centre, ou `c` est le barycentre `w` de `S`."""
    total = Fraction(0)
    for index, weight in enumerate(weights):
        if weight == 0:
            continue
        total += weight * table[support[index]][target]
    return total - inner / 2


def ball_by_support_scan(table, subset):
    """Boule englobante minimale par balayage des supports et conditions KKT.

    Un support `S` est admissible lorsque son systeme borde est regulier, ses
    barycentriques sont toutes positives ou nulles (centre dans l'enveloppe
    convexe de `S`) et sa sphere contient tout `subset`. Le minimum du rayon
    au carre sur les supports admissibles est `beta(subset)`.

    Renvoie `(rayon_carre, support)`.
    """
    members = tuple(subset)
    if not members:
        raise ValueError("sous-ensemble vide")
    best_key = None
    best_support = None
    for size in range(1, len(members) + 1):
        for support in combinations(members, size):
            outcome = circumsphere_from_table(table, support)
            if outcome is None:
                continue
            radius_squared, weights = outcome
            if radius_squared < 0:
                continue
            if any(weight < 0 for weight in weights):
                continue
            inner = support_inner_term(table, support, weights)
            witness = squared_distance_to_center(
                table, support, weights, inner, support[0]
            )
            if witness != radius_squared:
                raise JudgeError("systeme borde incoherent avec le developpement")
            outside = False
            for target in members:
                distance = squared_distance_to_center(
                    table, support, weights, inner, target
                )
                if distance > radius_squared:
                    outside = True
                    break
            if outside:
                continue
            key = (radius_squared, size, support)
            if best_key is None or key < best_key:
                best_key = key
                best_support = support
    if best_key is None:
        raise JudgeError("aucun support admissible : table de distances invalide")
    return best_key[0], best_support


def _boundary_ball(table, boundary):
    """Boule dont toute la frontiere declaree est sur la sphere, ou `None`."""
    if not boundary:
        return None
    outcome = circumsphere_from_table(table, boundary)
    if outcome is None:
        return None
    radius_squared, weights = outcome
    if radius_squared < 0:
        return None
    inner = support_inner_term(table, boundary, weights)
    return radius_squared, tuple(boundary), weights, inner


def _welzl(table, pending, boundary):
    """Recursion de Welzl exacte ; `None` quand aucune boule n'est definie.

    Attention a la portee du `None` : un `None` remonte par l'appel interne
    est traite ici comme le cas "la boule ne contient pas le pivot", donc la
    recursion pousse le pivot sur la frontiere au lieu d'abandonner. Les deux
    situations sont ainsi confondues en profondeur, et seule une absence de
    boule au sommet de la recursion est rapportee. C'est pourquoi ce temoin
    ne decide jamais seul : sa valeur est confrontee a celle de
    `ball_by_support_scan`, qui n'a pas cette fragilite.
    """
    if not pending:
        return _boundary_ball(table, boundary)
    pivot = pending[-1]
    rest = pending[:-1]
    ball = _welzl(table, rest, boundary)
    if ball is not None:
        radius_squared, support, weights, inner = ball
        distance = squared_distance_to_center(table, support, weights, inner, pivot)
        if distance <= radius_squared:
            return ball
    return _welzl(table, rest, boundary + (pivot,))


def ball_by_welzl(table, subset):
    """Second temoin : Welzl recursif exact. Renvoie `(rayon_carre, support)` ou `None`.

    La recursion de Welzl n'est pas robuste par construction aux entrees
    degenerees. Quand aucune boule n'est definie au sommet de la recursion,
    ce temoin repond `None` au lieu d'inventer une decision : ce `None` est
    un aveu d'indecision, jamais un verdict. En revanche une sphere
    circonscrite manquante en PROFONDEUR n'est pas rapportee, `_welzl` la
    confondant avec un pivot a mettre sur la frontiere ; ce temoin peut donc
    en principe rendre un rayon faux, et c'est pour cela qu'il est confronte
    a `ball_by_support_scan` au lieu d'etre cru.

    Mesure a ce jour : sur 7158 sous-ensembles (carre cocyclique seul, cube,
    octaedre, simplexe regulier plus origine, colineaires, doublons, et
    nuages a coordonnees resserrees jusqu'a n = 9), aucun `None` et aucun
    rayon faux. C'est une absence de refutation, pas une preuve de robustesse.
    """
    members = tuple(subset)
    if not members:
        raise ValueError("sous-ensemble vide")
    ball = _welzl(table, members, ())
    if ball is None:
        return None
    radius_squared, support, _weights, _inner = ball
    return radius_squared, support


def half_diameter_squared(table, subset):
    """Carre du demi-diametre : majorant grossier, faux des qu'il n'est pas atteint."""
    largest = Fraction(0)
    for left in subset:
        for right in subset:
            value = table[left][right]
            if value > largest:
                largest = value
    return largest / 4


class GammaJudge:
    """Juge d'un nuage : niveaux critiques et partitions de `pi_0(Gamma_k(a))`.

    Le parametre `mutant` injecte une faute causale dans le juge lui-meme :
    une porte doit verifier que chaque mutant produit au moins un desaccord
    avec le sujet, sinon la porte ne mesure rien.
    """

    def __init__(self, cloud, k_max, mutant=MUTANT_NONE):
        if mutant not in KNOWN_MODES:
            raise ValueError("mutant inconnu : " + str(mutant))
        if k_max < 1:
            raise ValueError("k_max doit valoir au moins 1")
        self.table = squared_distance_table(cloud)
        self.count = len(self.table)
        self.k_max = k_max
        self.k_effective = min(k_max, self.count)
        self.max_subset_size = min(self.k_effective + 1, self.count)
        self.mutant = mutant
        self.beta_cache = {}
        self.beta_calls = 0
        self.edge_tests = 0
        self.bfs_visits = 0
        self.partition_calls = 0
        self.levels = self._critical_levels()

    def beta(self, subset):
        """Niveau exact du sous-ensemble, ou sa version mutee."""
        key = tuple(subset)
        cached = self.beta_cache.get(key)
        if cached is not None:
            return cached
        self.beta_calls += 1
        if self.mutant == MUTANT_HALF_DIAMETER:
            value = half_diameter_squared(self.table, key)
        else:
            value, _support = ball_by_support_scan(self.table, key)
        self.beta_cache[key] = value
        return value

    def _critical_levels(self):
        """Niveaux critiques : tous les `beta(S)` de cardinal utile aux ordres."""
        values = set()
        for size in range(1, self.max_subset_size + 1):
            for subset in combinations(range(self.count), size):
                values.add(self.beta(subset))
        return tuple(sorted(values))

    def _is_active(self, subset, level):
        value = self.beta(subset)
        if self.mutant == MUTANT_STRICT_LEVEL:
            return value < level
        return value <= level

    def vertices(self, order, level):
        """Sommets de `Gamma_order(level)`, en ordre lexicographique."""
        if order < 1 or order > self.count:
            raise ValueError("ordre hors de 1..n")
        return [
            subset
            for subset in combinations(range(self.count), order)
            if self._is_active(subset, level)
        ]

    def edges(self, order, level, active):
        """Paires adjacentes, enumerees par echange de points (jamais par coface).

        Les deux clauses de la section 4 sont testees separement : le cardinal
        de l'union par `gap`, son niveau par `_is_active`. Le mutant
        `free_swap` supprime la seconde clause sans toucher a la premiere,
        ce qui est exactement la confusion entre `Gamma_k(a)` et le graphe
        "partager k - 1 points".
        """
        gap = 2 if self.mutant == MUTANT_UNION_CARDINAL else 1
        ignore_union_level = self.mutant == MUTANT_FREE_SWAP
        active_set = set(active)
        found = set()
        for vertex in active:
            outside = [point for point in range(self.count) if point not in vertex]
            if len(outside) < gap or len(vertex) < gap:
                continue
            for entering in combinations(outside, gap):
                union = tuple(sorted(set(vertex) | set(entering)))
                self.edge_tests += 1
                if not ignore_union_level and not self._is_active(union, level):
                    continue
                for leaving in combinations(vertex, gap):
                    other = tuple(sorted((set(vertex) - set(leaving)) | set(entering)))
                    if other == vertex or other not in active_set:
                        continue
                    pair = (vertex, other) if vertex < other else (other, vertex)
                    found.add(pair)
        return sorted(found)

    def partition(self, order, level):
        """Partition de `pi_0(Gamma_order(level))` par parcours en largeur.

        Forme canonique identique a celle de `ehgp.exact.tower` : pour chaque
        composante, le sommet lexicographiquement minimal et l'union triee des
        identifiants d'observations.
        """
        self.partition_calls += 1
        active = self.vertices(order, level)
        adjacency = {vertex: [] for vertex in active}
        for left, right in self.edges(order, level, active):
            adjacency[left].append(right)
            adjacency[right].append(left)
        seen = set()
        components = []
        for start in active:
            if start in seen:
                continue
            seen.add(start)
            queue = deque([start])
            members = [start]
            while queue:
                current = queue.popleft()
                self.bfs_visits += 1
                for neighbour in adjacency[current]:
                    if neighbour in seen:
                        continue
                    seen.add(neighbour)
                    members.append(neighbour)
                    queue.append(neighbour)
            if self.mutant == MUTANT_NO_ISOLATED and len(members) == 1:
                continue
            covered = set()
            for member in members:
                covered.update(member)
            components.append((min(members), tuple(sorted(covered))))
        return tuple(sorted(components))

    def welzl_cross_check(self):
        """Compare les deux miniballs du juge sur tous les sous-ensembles utiles.

        Renvoie `(compares, indecis, desaccords)` : `indecis` compte les
        sous-ensembles ou Welzl refuse de conclure, `desaccords` liste les
        rayons differents.
        """
        compared = 0
        undecided = 0
        disagreements = []
        for size in range(1, self.max_subset_size + 1):
            for subset in combinations(range(self.count), size):
                reference, _support = ball_by_support_scan(self.table, subset)
                witness = ball_by_welzl(self.table, subset)
                if witness is None:
                    undecided += 1
                    continue
                compared += 1
                if witness[0] != reference:
                    disagreements.append((subset, reference, witness[0]))
        return compared, undecided, disagreements
