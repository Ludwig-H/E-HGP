"""Concentration, taille et contraste : ambiant ou intrinseque ?

Cette sonde repond a une seule question, celle qui decide si E-HGP a un
avenir sur des donnees reelles :

    l'obstruction de E-HGP en grande dimension est-elle gouvernee par la
    dimension AMBIANTE `d` ou par la dimension INTRINSEQUE `r` du support
    des donnees, et un changement de metrique restaure-t-il le signal de
    densite ?

Trois mesures, pour `k` dans une liste d'ordres :

* (a) OBSTRUCTION DE TAILLE : part des parties `F` de cardinal `k` dont la
  boule englobante minimale est VIDE des autres observations. Une telle
  partie engendre une naissance de composante dans `Gamma_k` : la part
  mesuree est donc, a un facteur `C(n, k)` pres, le nombre de naissances de
  la tour d'ordre `k`.
* (b) OBSTRUCTION DE SIGNAL : moyenne et ecart-type du rayon de boule
  englobante normalise par la distance typique entre paires, a comparer a
  la valeur du simplexe regulier `sqrt((k - 1) / (2 k))`. Quand la mesure
  colle a cette valeur avec un ecart-type qui s'effondre, la filtration en
  rayon degenere en escalier deterministe pilote par `k` seul.
* (c) CONTRASTE DE DENSITE : distribution du nombre d'observations dans
  `B(x_i, r_ref)` quand `x_i` parcourt les observations, avec
  `r_ref` = mediane de la distance au `m`-ieme voisin. Sans dispersion de
  ce comptage, aucune filtration par boules ne peut porter de signal.

Chaque mesure est reprise apres trois changements de representation :
blanchiment (Mahalanobis empirique), projection sur les `r` premieres
composantes principales, projection aleatoire de Johnson-Lindenstrauss.

La boule englobante est recalculee ici, independamment de
`src/ehgp/exact/meb.py` : methode d'ensemble actif (ajout du point le plus
loin, retrait d'un barycentrique negatif) avec CERTIFICAT KKT explicite,
puis repli par enumeration des supports. Elle est validee contre la boule
rationnelle exacte de `ehgp.exact.meb` par la table `selftest`, qui est une
porte : sans validation, aucune autre table ne s'affiche.

Usage :

    python3 bench/concentration.py --table selftest
    python3 bench/concentration.py --table exhaustive --seeds 5
    python3 bench/concentration.py --table free --n-list 200,800
    python3 bench/concentration.py --table dimsweep
    python3 bench/concentration.py --table noise
    python3 bench/concentration.py --table repr
    python3 bench/concentration.py --table contrast
    python3 bench/concentration.py --table tower
    python3 bench/concentration.py --table all

Codes de sortie : 0 si tout a tourne, 3 si un plancher de couverture n'est
pas atteint (validation de la boule, taux de certification, part de
verdicts indecis, nombre de boules calculees).

Aucun octet de SemanticKITTI n'est utilise : la famille `lidar` est un
nuage synthetique fabrique dans ce fichier.
"""

import argparse
import math
import sys
import time
from fractions import Fraction
from itertools import combinations
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.exact.meb import minimum_enclosing_ball
from ehgp.exact.projection import projected_ultrametric
from ehgp.exact.tower import FullTower

TINY = 1e-300


# ---------------------------------------------------------------------------
# Boule englobante minimale certifiee (implementation independante)
# ---------------------------------------------------------------------------


def _circumcentre(points, support):
    """Centre circonscrit d'un support dans son enveloppe affine.

    Renvoie `(centre, rayon_carre, barycentriques)` ou `(None, None, None)`
    si le support est affinement dependant ou mal conditionne. Le systeme
    n'utilise que les produits scalaires des directions : son cout ne
    depend de la dimension ambiante que par le calcul du Gram.
    """
    base = points[support[0]]
    if len(support) == 1:
        return base.copy(), 0.0, np.ones(1)
    directions = points[list(support[1:])] - base
    gram = directions @ directions.T
    right = 0.5 * np.einsum("ij,ij->i", directions, directions)
    try:
        weights = np.linalg.solve(gram, right)
    except np.linalg.LinAlgError:
        return None, None, None
    if not np.all(np.isfinite(weights)):
        return None, None, None
    centre = base + weights @ directions
    radius2 = float(np.dot(centre - base, centre - base))
    deltas = points[list(support)] - centre
    reached = np.einsum("ij,ij->i", deltas, deltas)
    scale = max(radius2, 1.0)
    if float(np.max(np.abs(reached - radius2))) > 1e-7 * scale:
        return None, None, None
    barycentric = np.concatenate(([1.0 - float(weights.sum())], weights))
    return centre, radius2, barycentric


def _meb_enumerate(points):
    """Boule englobante par enumeration des supports (repli, `k` petit)."""
    count = len(points)
    best = None
    for size in range(1, count + 1):
        for support in combinations(range(count), size):
            centre, radius2, barycentric = _circumcentre(points, support)
            if centre is None:
                continue
            if float(np.min(barycentric)) < -1e-9:
                continue
            if best is not None and radius2 >= best[1]:
                continue
            deltas = points - centre
            reached = np.einsum("ij,ij->i", deltas, deltas)
            if float(np.max(reached)) > radius2 * (1.0 + 1e-9) + 1e-12:
                continue
            best = (centre, radius2, tuple(support))
    if best is None:
        return None
    return best


def meb_certified(points, tol=1e-10, max_steps=None):
    """Boule englobante minimale avec certificat KKT.

    Renvoie `(centre, rayon_carre, support, certifie, etapes)`. Le
    certificat est celui de la specification de la boule : les
    barycentriques du centre dans son support sont positifs (le centre est
    dans l'enveloppe convexe du support) ET aucun point n'est strictement
    dehors. Ces deux conditions sont exactement les conditions de Karush,
    Kuhn et Tucker du programme dual `max sum_i w_i ||x_i||^2 - ||sum_i w_i
    x_i||^2` sur le simplexe : le certificat vaut ecart dual nul.
    """
    cloud = np.asarray(points, dtype=float)
    count = cloud.shape[0]
    if count == 1:
        return cloud[0].copy(), 0.0, (0,), True, 0
    norms = np.einsum("ij,ij->i", cloud, cloud)
    squared = norms[:, None] + norms[None, :] - 2.0 * (cloud @ cloud.T)
    flat = int(np.argmax(squared))
    first, second = divmod(flat, count)
    if first == second:
        return cloud[0].copy(), 0.0, (0,), True, 0
    support = [min(first, second), max(first, second)]
    limit = max_steps if max_steps is not None else 6 * count + 30
    centre = None
    radius2 = 0.0
    steps = 0
    while steps < limit:
        steps += 1
        candidate, candidate_radius2, barycentric = _circumcentre(cloud, tuple(support))
        if candidate is None:
            if len(support) <= 2:
                break
            support.pop()
            continue
        centre, radius2 = candidate, candidate_radius2
        scale = max(radius2, 1.0)
        if float(np.min(barycentric)) < -tol:
            support.pop(int(np.argmin(barycentric)))
            continue
        deltas = cloud - centre
        reached = np.einsum("ij,ij->i", deltas, deltas)
        worst = int(np.argmax(reached))
        if float(reached[worst]) <= radius2 + tol * scale:
            return centre, radius2, tuple(sorted(support)), True, steps
        if worst in support:
            break
        support.append(worst)
        support.sort()
    fallback = _meb_enumerate(cloud) if count <= 12 else None
    if fallback is not None:
        return fallback[0], fallback[1], fallback[2], True, steps
    if centre is None:
        centre = cloud.mean(axis=0)
        deltas = cloud - centre
        radius2 = float(np.max(np.einsum("ij,ij->i", deltas, deltas)))
    return centre, radius2, tuple(sorted(support)), False, steps


# ---------------------------------------------------------------------------
# Familles de nuages, toutes parametrees par (n, d, r)
# ---------------------------------------------------------------------------


def _orthonormal(dimension, rank, rng):
    """Base orthonormee de `rank` colonnes dans `R^dimension`."""
    matrix = rng.standard_normal((dimension, rank))
    basis, _ = np.linalg.qr(matrix)
    return basis[:, :rank]


def _median_pair_distance(points, rng, budget=20000):
    """Mediane des distances entre paires (echantillonnee si besoin)."""
    count = len(points)
    total = count * (count - 1) // 2
    if total == 0:
        return 0.0
    if total <= budget:
        rows, cols = np.triu_indices(count, k=1)
    else:
        rows = rng.integers(0, count, size=budget)
        cols = rng.integers(0, count, size=budget)
        keep = rows != cols
        rows, cols = rows[keep], cols[keep]
    deltas = points[rows] - points[cols]
    return float(np.median(np.sqrt(np.einsum("ij,ij->i", deltas, deltas))))


def _swiss_roll(count, rng):
    """Rouleau suisse : variete de dimension 2 plongee NON lineairement."""
    angle = 1.5 * math.pi * (1.0 + 2.0 * rng.random(count))
    height = 2.0 * rng.random(count)
    raw = np.stack([angle * np.cos(angle), height * math.pi, angle * np.sin(angle)], axis=1)
    return raw / (1.5 * math.pi)


def _peano_curve(count, rng):
    """Courbe lisse remplissante : variete de dimension 1 tres repliee."""
    parameter = rng.random(count)
    harmonics = 9.0
    raw = np.stack(
        [
            np.cos(harmonics * math.pi * parameter) * (0.5 + parameter),
            np.sin(harmonics * math.pi * parameter) * (0.5 + parameter),
            parameter,
        ],
        axis=1,
    )
    return raw


def _lidar_cloud(count, attributes, rng):
    """Nuage synthetique de type balayage lidar (aucun octet SemanticKITTI).

    Trois coordonnees metriques (sol plan plus quelques facades verticales,
    echantillonnees par un balayage en azimut et en elevation) suivies
    d'attributs correles a la geometrie : intensite, portee normalisee,
    indice d'echo, hauteur locale. La dimension intrinseque geometrique est
    2 (des surfaces) ; les attributs ajoutent des directions bruitees.
    """
    azimuth = 2.0 * math.pi * rng.random(count)
    rings = 1 + (rng.random(count) * 16.0).astype(int)
    elevation = -0.30 + 0.02 * rings
    ground = elevation < -0.02
    reach = np.where(ground, 1.7 / np.maximum(-elevation, 1e-3), 12.0 + 8.0 * rng.random(count))
    reach = np.minimum(reach, 40.0)
    position = np.stack(
        [reach * np.cos(azimuth), reach * np.sin(azimuth), 1.7 + reach * elevation], axis=1
    )
    facade = ~ground
    position[facade, 2] = 0.2 + 3.0 * rng.random(int(facade.sum()))
    position = position / 10.0
    columns = [position]
    if attributes >= 1:
        intensity = 0.3 + 0.4 * np.cos(3.0 * azimuth) + 0.05 * rng.standard_normal(count)
        columns.append(intensity[:, None])
    if attributes >= 2:
        columns.append((reach / 40.0)[:, None])
    if attributes >= 3:
        columns.append((rings / 16.0)[:, None])
    if attributes >= 4:
        columns.append((position[:, 2] + 0.02 * rng.standard_normal(count))[:, None])
    stacked = np.concatenate(columns, axis=1)
    return stacked[:, : 3 + max(attributes, 0)]


def make_cloud(name, count, dimension, intrinsic, noise, rng, clusters=8):
    """Nuage de la famille `name`, de dimension ambiante `dimension`.

    `intrinsic` est la dimension intrinseque demandee (ignoree par les
    familles de plein rang), `noise` l'ecart-type du bruit gaussien ambiant
    AJOUTE PAR COORDONNEE, exprime en fraction de la distance mediane entre
    paires du support sans bruit. L'energie de bruit vaut donc
    `noise^2 * dimension` : c'est par la que la dimension ambiante entre.
    """
    if name == "uniform":
        return rng.random((count, dimension))
    if name == "gauss_iso":
        return rng.standard_normal((count, dimension))
    if name == "gauss_aniso":
        spectrum = np.power(np.arange(1, dimension + 1, dtype=float), -1.0)
        return rng.standard_normal((count, dimension)) * np.sqrt(spectrum)
    if name == "sphere":
        raw = rng.standard_normal((count, dimension))
        return raw / np.linalg.norm(raw, axis=1, keepdims=True)
    if name == "clusters":
        centres = rng.standard_normal((clusters, dimension)) * 6.0
        labels = rng.integers(0, clusters, size=count)
        return centres[labels] + rng.standard_normal((count, dimension)) * 0.35
    if name in ("flat", "flat_noise"):
        rank = max(1, min(intrinsic, dimension))
        latent = rng.random((count, rank))
        basis = _orthonormal(dimension, rank, rng)
        support = latent @ basis.T
        if name == "flat":
            return support
        return _add_noise(support, noise, rng)
    if name in ("roll", "roll_noise"):
        if dimension < 3:
            raise ValueError("le rouleau suisse demande une dimension ambiante >= 3")
        basis = _orthonormal(dimension, 3, rng)
        support = _swiss_roll(count, rng) @ basis.T
        if name == "roll":
            return support
        return _add_noise(support, noise, rng)
    if name in ("peano", "peano_noise"):
        if dimension < 3:
            raise ValueError("la courbe repliee demande une dimension ambiante >= 3")
        basis = _orthonormal(dimension, 3, rng)
        support = _peano_curve(count, rng) @ basis.T
        if name == "peano":
            return support
        return _add_noise(support, noise, rng)
    if name == "lidar":
        attributes = max(0, dimension - 3)
        return _lidar_cloud(count, attributes, rng)
    raise ValueError("famille inconnue : " + str(name))


def graded_cloud(count, dimension, intrinsic, noise, rng, slope=3.0):
    """Variete plate a densite LATENTE CONNUE, plus bruit ambiant.

    La densite latente est proportionnelle a `exp(slope * u_1)` sur le cube
    `[0, 1]^r` : le rapport de densite entre les deux bords vaut
    `exp(slope)`. Le tirage se fait par transformation inverse, donc la
    densite de chaque point est connue exactement. C'est la seule famille du
    fichier ou le signal cherche est connu, donc la seule ou l'on peut
    mesurer si le comptage de boules le retrouve au lieu de mesurer sa seule
    dispersion.

    Renvoie `(nuage, densite latente)`.
    """
    rank = max(1, min(intrinsic, dimension))
    uniform = rng.random(count)
    first = np.log1p(uniform * (math.exp(slope) - 1.0)) / slope
    if rank > 1:
        rest = rng.random((count, rank - 1))
    else:
        rest = np.zeros((count, 0))
    latent = np.concatenate([first[:, None], rest], axis=1)
    basis = _orthonormal(dimension, rank, rng)
    support = latent @ basis.T
    density = np.exp(slope * first)
    return _add_noise(support, noise, rng), density


def _add_noise(support, noise, rng):
    """Ajoute un bruit gaussien ambiant relatif a l'echelle du support."""
    if noise <= 0.0:
        return support
    scale = _median_pair_distance(support, rng)
    if scale <= 0.0:
        scale = 1.0
    return support + rng.standard_normal(support.shape) * (noise * scale)


FAMILIES = (
    ("uniform", 2, 2),
    ("uniform", 3, 3),
    ("uniform", 20, 20),
    ("uniform", 200, 200),
    ("gauss_iso", 20, 20),
    ("gauss_aniso", 200, 200),
    ("sphere", 200, 199),
    ("clusters", 200, 200),
    ("flat", 200, 2),
    ("flat_noise", 200, 2),
    ("roll", 200, 2),
    ("roll_noise", 200, 2),
    ("peano", 200, 1),
    ("lidar", 7, 2),
)


# ---------------------------------------------------------------------------
# Changements de representation
# ---------------------------------------------------------------------------


def whiten(points, tol=1e-10):
    """Blanchiment : metrique de Mahalanobis empirique, rang effectif garde."""
    centred = points - points.mean(axis=0)
    covariance = centred.T @ centred / max(len(points) - 1, 1)
    values, vectors = np.linalg.eigh(covariance)
    top = float(values.max()) if values.size else 0.0
    keep = values > tol * max(top, TINY)
    if not np.any(keep):
        return centred
    return (centred @ vectors[:, keep]) / np.sqrt(values[keep])


def principal(points, components):
    """Projection sur les `components` premieres composantes principales."""
    centred = points - points.mean(axis=0)
    limit = max(1, min(components, min(centred.shape)))
    _left, _values, right = np.linalg.svd(centred, full_matrices=False)
    return centred @ right[:limit].T


def johnson_lindenstrauss(points, rng, epsilon):
    """Projection aleatoire gaussienne en dimension `O(log n / eps^2)`."""
    count, dimension = points.shape
    target = int(math.ceil(4.0 * math.log(max(count, 2)) / (epsilon * epsilon)))
    target = max(1, min(dimension, target))
    matrix = rng.standard_normal((dimension, target)) / math.sqrt(target)
    return points @ matrix


def pair_distortion(before, after, rng, budget=4000):
    """Distorsion maximale des distances entre paires echantillonnees."""
    count = len(before)
    rows = rng.integers(0, count, size=budget)
    cols = rng.integers(0, count, size=budget)
    keep = rows != cols
    rows, cols = rows[keep], cols[keep]
    left = before[rows] - before[cols]
    right = after[rows] - after[cols]
    source = np.sqrt(np.einsum("ij,ij->i", left, left))
    target = np.sqrt(np.einsum("ij,ij->i", right, right))
    valid = source > 0.0
    ratio = target[valid] / source[valid]
    median = float(np.median(ratio)) if ratio.size else 1.0
    if median <= 0.0:
        median = 1.0
    relative = ratio / median
    return float(np.max(np.abs(relative - 1.0))) if relative.size else 0.0


def represent(points, kind, intrinsic, rng, epsilon):
    """Applique un changement de representation ; renvoie (nuage, etiquette)."""
    if kind == "raw":
        return points, "raw(d=%d)" % points.shape[1]
    if kind == "rot":
        dimension = points.shape[1]
        rotation = _orthonormal(dimension, dimension, rng)
        return points @ rotation, "rot(d=%d)" % dimension
    if kind == "whiten":
        result = whiten(points)
        return result, "whiten(d=%d)" % result.shape[1]
    if kind == "pca":
        result = principal(points, max(1, intrinsic))
        return result, "pca(d=%d)" % result.shape[1]
    if kind == "jl":
        result = johnson_lindenstrauss(points, rng, epsilon)
        return result, "jl(d=%d)" % result.shape[1]
    raise ValueError("representation inconnue : " + str(kind))


# ---------------------------------------------------------------------------
# Les trois mesures
# ---------------------------------------------------------------------------


class Ledger:
    """Compteurs de couverture ; decide le code de sortie."""

    def __init__(self, min_balls, min_selftest, max_undecided):
        self.min_balls = min_balls
        self.min_selftest = min_selftest
        self.max_undecided = max_undecided
        self.balls = 0
        self.certified = 0
        self.decisions = 0
        self.undecided = 0
        self.selftest = 0
        self.selftest_failures = 0
        self.failures = []

    def account(self, balls, certified, decisions, undecided):
        self.balls += balls
        self.certified += certified
        self.decisions += decisions
        self.undecided += undecided

    def verdict(self):
        """Renvoie (code, lignes de diagnostic)."""
        lines = []
        code = 0
        lines.append("boules calculees        : %d (plancher %d)" % (self.balls, self.min_balls))
        rate = self.certified / self.balls if self.balls else 0.0
        lines.append("taux de certification   : %.6f" % rate)
        share = self.undecided / self.decisions if self.decisions else 0.0
        lines.append(
            "verdicts indecis        : %d / %d = %.3e (plafond %.1e)"
            % (self.undecided, self.decisions, share, self.max_undecided)
        )
        lines.append(
            "validations exactes     : %d (plancher %d), echecs %d"
            % (self.selftest, self.min_selftest, self.selftest_failures)
        )
        if self.balls < self.min_balls:
            code = 3
            lines.append("PLANCHER MANQUE : trop peu de boules calculees")
        if self.balls and rate < 1.0:
            code = 3
            lines.append("PLANCHER MANQUE : une boule n'a pas de certificat KKT")
        if share > self.max_undecided:
            code = 3
            lines.append("PLANCHER MANQUE : trop de verdicts indecis")
        if self.selftest < self.min_selftest:
            code = 3
            lines.append("PLANCHER MANQUE : validation exacte insuffisante")
        if self.selftest_failures:
            code = 3
            lines.append("PLANCHER MANQUE : desaccord avec la boule rationnelle exacte")
        for text in self.failures:
            code = 3
            lines.append("PLANCHER MANQUE : " + text)
        return code, lines


def subset_sample(count, order, budget, rng):
    """Parties de cardinal `order` : toutes si peu, sinon un echantillon."""
    total = math.comb(count, order)
    if total <= budget:
        return list(combinations(range(count), order)), True
    seen = set()
    while len(seen) < budget:
        seen.add(tuple(sorted(rng.choice(count, size=order, replace=False).tolist())))
    return sorted(seen), False


def measure_order(points, order, subsets, ledger, tol=1e-9):
    """Mesures (a) et (b) pour un ordre donne sur une liste de parties."""
    count = len(points)
    norms = np.einsum("ij,ij->i", points, points)
    free = 0
    decided = 0
    undecided = 0
    certified = 0
    radii = []
    for subset in subsets:
        block = points[list(subset)]
        centre, radius2, _support, ok, _steps = meb_certified(block)
        if ok:
            certified += 1
        radii.append(math.sqrt(max(radius2, 0.0)))
        distances = norms - 2.0 * (points @ centre) + float(centre @ centre)
        scale = max(radius2, TINY)
        relative = (distances - radius2) / scale
        relative[list(subset)] = 1.0
        inside = int(np.count_nonzero(relative < -tol))
        fuzzy = int(np.count_nonzero(np.abs(relative) <= tol))
        if inside > 0:
            decided += 1
        elif fuzzy > 0:
            undecided += 1
        else:
            decided += 1
            free += 1
    ledger.account(len(subsets), certified, len(subsets), undecided)
    radii = np.asarray(radii)
    return {
        "free": free,
        "decided": decided,
        "undecided": undecided,
        "total": len(subsets),
        "share": free / decided if decided else float("nan"),
        "radius_mean": float(radii.mean()) if radii.size else float("nan"),
        "radius_std": float(radii.std(ddof=0)) if radii.size else float("nan"),
        "certified": certified,
        "count": count,
    }


def density_contrast(points, neighbours, rng):
    """Mesure (c) : dispersion du comptage de boules a rayon fixe."""
    count = len(points)
    squared = _squared_matrix(points)
    order = min(neighbours, count - 1)
    partitioned = np.partition(squared, order, axis=1)[:, order]
    reference2 = float(np.median(partitioned))
    counts = np.count_nonzero(squared <= reference2, axis=1) - 1
    quart = np.percentile(counts, [10.0, 25.0, 50.0, 75.0, 90.0])
    mean = float(counts.mean())
    deviation = float(counts.std(ddof=0))
    low = max(quart[1], 1.0)
    decile = max(quart[0], 1.0)
    values, occurrences = np.unique(counts, return_counts=True)
    probabilities = occurrences / occurrences.sum()
    entropy = float(-np.sum(probabilities * np.log(probabilities)))
    span = math.log(len(values)) if len(values) > 1 else 0.0
    return {
        "radius": math.sqrt(reference2),
        "mean": mean,
        "std": deviation,
        "cv": deviation / mean if mean > 0.0 else float("nan"),
        "q1": float(quart[1]),
        "median": float(quart[2]),
        "q3": float(quart[3]),
        "iqr_ratio": float(quart[3]) / low,
        "decile_ratio": float(quart[4]) / decile,
        "max_ratio": float(counts.max()) / max(quart[2], 1.0),
        "entropy": entropy / span if span > 0.0 else 0.0,
        "distinct": int(len(values)),
    }


# ---------------------------------------------------------------------------
# Impression alignee
# ---------------------------------------------------------------------------


def print_table(title, headers, rows):
    """Imprime un tableau aligne (colonnes de largeur uniforme)."""
    print("")
    print(title)
    columns = len(headers)
    widths = [len(str(headers[index])) for index in range(columns)]
    for row in rows:
        for index in range(columns):
            widths[index] = max(widths[index], len(str(row[index])))
    line = "  ".join(str(headers[index]).ljust(widths[index]) for index in range(columns))
    print(line)
    print("-" * len(line))
    for row in rows:
        print("  ".join(str(row[index]).ljust(widths[index]) for index in range(columns)))


def summarise(values):
    """Moyenne et ecart-type sur les graines, en texte compact."""
    array = np.asarray([value for value in values if not math.isnan(value)])
    if array.size == 0:
        return "n/a"
    if array.size == 1:
        return "%.3f" % array[0]
    return "%.3f+-%.3f" % (float(array.mean()), float(array.std(ddof=0)))


def births_estimate(shares, count, order, budget):
    """Naissances estimees `part * C(n, k)` et resolution de l'echantillon.

    Une part mesuree nulle sur `budget` tirages ne prouve PAS que les
    naissances sont peu nombreuses : elle borne seulement la part par
    environ `1 / budget`, donc les naissances par `C(n, k) / budget`. La
    colonne de resolution donne cette borne, qui reste astronomique des que
    `k` grandit. La mesure echantillonnee certifie une explosion, jamais son
    absence.
    """
    total = math.comb(count, order)
    values = [value for value in shares if not math.isnan(value)]
    if not values:
        return "n/a", "n/a"
    mean = sum(values) / len(values)
    resolution = total / max(min(budget, total), 1)
    return "%.2e" % (mean * total), "%.2e" % resolution


def summarise_pair(means, deviations):
    """Moyenne des moyennes et moyenne des ecarts-types intra-nuage."""
    left = np.asarray([value for value in means if not math.isnan(value)])
    right = np.asarray([value for value in deviations if not math.isnan(value)])
    if left.size == 0:
        return "n/a"
    return "%.3f+-%.3f" % (float(left.mean()), float(right.mean()))


# ---------------------------------------------------------------------------
# Table selftest : validation contre la boule rationnelle exacte
# ---------------------------------------------------------------------------


def _exact_radius2(block):
    """Rayon au carre exact (rationnel) de la boule englobante minimale."""
    rational = [[Fraction(int(value)) for value in point] for point in block]
    return minimum_enclosing_ball(rational)[1]


def table_selftest(options, ledger):
    """Porte : la boule de ce fichier contre la boule rationnelle exacte."""
    rows = []
    rng = np.random.default_rng(options.seed)
    scenarios = (
        ("grille dense", 20),
        ("grille fine", 3),
        ("cospherique", 0),
        ("colineaire", 0),
        ("duplique", 0),
    )
    total = 0
    failures = 0
    worst = 0.0
    for label, span in scenarios:
        local_total = 0
        local_failures = 0
        local_worst = 0.0
        for _trial in range(options.selftest_cases):
            order = int(rng.integers(2, 7))
            dimension = int(rng.integers(1, 6))
            if label == "cospherique":
                angles = rng.integers(0, 24, size=(order, 1))
                block = np.concatenate(
                    [
                        np.round(60.0 * np.cos(2.0 * math.pi * angles / 24.0)),
                        np.round(60.0 * np.sin(2.0 * math.pi * angles / 24.0)),
                    ],
                    axis=1,
                )
            elif label == "colineaire":
                direction = rng.integers(-3, 4, size=(1, dimension))
                steps = rng.integers(-5, 6, size=(order, 1))
                block = (steps * direction).astype(float)
            elif label == "duplique":
                base = rng.integers(0, 9, size=(max(2, order - 1), dimension)).astype(float)
                block = np.concatenate([base, base[:1]], axis=0)
            else:
                block = rng.integers(0, span + 1, size=(order, dimension)).astype(float)
            centre, radius2, _support, ok, _steps = meb_certified(block)
            reference = float(_exact_radius2(block))
            scale = max(reference, 1.0)
            error = abs(radius2 - reference) / scale
            local_worst = max(local_worst, error)
            local_total += 1
            deltas = block - centre
            reached = float(np.max(np.einsum("ij,ij->i", deltas, deltas)))
            if error > 1e-9 or not ok or reached > radius2 * (1.0 + 1e-8) + 1e-9:
                local_failures += 1
        total += local_total
        failures += local_failures
        worst = max(worst, local_worst)
        rows.append([label, local_total, local_failures, "%.2e" % local_worst])
    known = []
    pair = np.array([[0.0, 0.0, 0.0], [3.0, 4.0, 0.0]])
    _centre, radius2, _support, _ok, _steps = meb_certified(pair)
    known.append(["paire (3,4)", "%.12f" % radius2, "%.12f" % 6.25])
    for order in (2, 3, 5, 11):
        simplex = _regular_simplex(order)
        _centre, radius2, _support, _ok, _steps = meb_certified(simplex)
        target = (order - 1) / (2.0 * order)
        known.append(
            ["simplexe k=%d" % order, "%.12f" % radius2, "%.12f" % target]
        )
        if abs(radius2 - target) > 1e-12:
            failures += 1
    ledger.selftest += total
    ledger.selftest_failures += failures
    print_table(
        "selftest (a) boule certifiee contre boule rationnelle exacte",
        ["scenario", "cas", "echecs", "erreur relative max"],
        rows,
    )
    print_table(
        "selftest (b) fixtures gravees : rayon carre",
        ["fixture", "mesure", "attendu"],
        known,
    )
    return total, failures, worst


def _regular_simplex(order):
    """Simplexe regulier de `order` sommets, arete 1, dans `R^{order-1}`."""
    identity = np.eye(order)
    centred = identity - identity.mean(axis=0)
    gram = centred @ centred.T
    values, vectors = np.linalg.eigh(gram)
    keep = values > 1e-12
    coordinates = vectors[:, keep] * np.sqrt(values[keep])
    edge = math.sqrt(2.0)
    return coordinates / edge


# ---------------------------------------------------------------------------
# Tables de mesure
# ---------------------------------------------------------------------------


def _cloud_label(name, dimension, intrinsic):
    return "%s d=%d r=%d" % (name, dimension, intrinsic)


def _orders_for(count, orders):
    return [order for order in orders if order <= count - 1]


def table_exhaustive(options, ledger):
    """Regime exhaustif : toutes les parties, `n` dans 8, 11, 14."""
    rows = []
    for count in options.exhaustive_n:
        for name, dimension, intrinsic in (
            ("uniform", 2, 2),
            ("uniform", 3, 3),
            ("uniform", 20, 20),
            ("uniform", 50, 50),
            ("uniform", 200, 200),
            ("flat", 200, 2),
            ("flat", 200, 3),
            ("roll", 200, 2),
        ):
            for order in _orders_for(count, options.orders):
                shares = []
                for index in range(options.seeds):
                    rng = np.random.default_rng(options.seed + 1000 * index + count)
                    points = make_cloud(name, count, dimension, intrinsic, options.noise, rng)
                    subsets, complete = subset_sample(count, order, 10000, rng)
                    result = measure_order(points, order, subsets, ledger)
                    shares.append(result["share"])
                    if not complete:
                        ledger.failures.append("regime exhaustif incomplet")
                rows.append(
                    [
                        count,
                        _cloud_label(name, dimension, intrinsic),
                        order,
                        math.comb(count, order),
                        summarise(shares),
                    ]
                )
    print_table(
        "(a) part des parties de cardinal k a boule englobante VIDE, regime exhaustif",
        ["n", "famille", "k", "C(n,k)", "part libre (moy+-et sur graines)"],
        rows,
    )
    return rows


def table_free(options, ledger):
    """Regime echantillonne : familles, plusieurs `n`, plusieurs ordres."""
    rows = []
    for count in options.n_list:
        for name, dimension, intrinsic in FAMILIES:
            for order in _orders_for(count, options.orders):
                shares = []
                for index in range(options.seeds):
                    rng = np.random.default_rng(options.seed + 7919 * index + 13 * count)
                    points = make_cloud(name, count, dimension, intrinsic, options.noise, rng)
                    subsets, _complete = subset_sample(count, order, options.subsets, rng)
                    result = measure_order(points, order, subsets, ledger)
                    shares.append(result["share"])
                estimate, resolution = births_estimate(
                    shares, count, order, options.subsets
                )
                rows.append(
                    [
                        count,
                        _cloud_label(name, dimension, intrinsic),
                        order,
                        summarise(shares),
                        estimate,
                        resolution,
                    ]
                )
    print_table(
        "(a) part libre, regime echantillonne (%d parties tirees par cellule)" % options.subsets,
        ["n", "famille", "k", "part libre", "naissances est.", "resolution"],
        rows,
    )
    return rows


def table_radius(options, ledger):
    """Mesure (b) : concentration du rayon normalise."""
    rows = []
    count = options.n_list[0]
    for name, dimension, intrinsic in FAMILIES:
        for order in _orders_for(count, options.orders):
            means = []
            deviations = []
            for index in range(options.seeds):
                rng = np.random.default_rng(options.seed + 104729 * index + count)
                points = make_cloud(name, count, dimension, intrinsic, options.noise, rng)
                scale = _median_pair_distance(points, rng)
                if scale <= 0.0:
                    continue
                subsets, _complete = subset_sample(count, order, options.subsets, rng)
                result = measure_order(points, order, subsets, ledger)
                means.append(result["radius_mean"] / scale)
                deviations.append(result["radius_std"] / scale)
            rows.append(
                [
                    _cloud_label(name, dimension, intrinsic),
                    order,
                    "%.3f" % math.sqrt((order - 1) / (2.0 * order)),
                    summarise_pair(means, deviations),
                    summarise(deviations),
                ]
            )
    print_table(
        "(b) rayon de boule englobante normalise par la distance mediane entre paires (n=%d)"
        % count,
        ["famille", "k", "simplexe", "rayon (moy+-et intra)", "et intra (moy+-et graines)"],
        rows,
    )
    return rows


def table_contrast(options, ledger):
    """Mesure (c) : contraste de densite, brut et apres representation."""
    rows = []
    count = options.n_list[0]
    for name, dimension, intrinsic in FAMILIES:
        for kind in options.representations:
            means = []
            cvs = []
            excess = []
            iqrs = []
            deciles = []
            entropies = []
            label = kind
            for index in range(options.seeds):
                rng = np.random.default_rng(options.seed + 65537 * index + count)
                points = make_cloud(name, count, dimension, intrinsic, options.noise, rng)
                view, label = represent(points, kind, intrinsic, rng, options.jl_epsilon)
                result = density_contrast(view, options.neighbours, rng)
                means.append(result["mean"])
                cvs.append(result["cv"])
                excess.append(result["cv"] * math.sqrt(max(result["mean"], TINY)))
                iqrs.append(result["iqr_ratio"])
                deciles.append(result["decile_ratio"])
                entropies.append(result["entropy"])
            rows.append(
                [
                    _cloud_label(name, dimension, intrinsic),
                    label,
                    summarise(means),
                    summarise(cvs),
                    summarise(excess),
                    summarise(iqrs),
                    summarise(deciles),
                    summarise(entropies),
                ]
            )
    del ledger
    print_table(
        "(c) contraste du comptage dans B(x_i, r_ref), r_ref = mediane du %d-ieme voisin (n=%d) ; "
        "exces = cv * sqrt(moyenne), egal a 1 pour un bruit de Poisson pur"
        % (options.neighbours, count),
        [
            "famille",
            "representation",
            "comptage moyen",
            "cv",
            "exces Poisson",
            "q3/q1",
            "p90/p10",
            "entropie norm.",
        ],
        rows,
    )
    return rows


def table_signal(options, ledger):
    """Le comptage retrouve-t-il une densite CONNUE ? Correlation de Spearman."""
    del ledger
    from scipy import stats

    rows = []
    count = options.n_list[0]
    for dimension in options.signal_dims:
        for level in options.signal_noise:
            for kind in ("raw", "pca", "whiten", "jl"):
                spearmans = []
                cvs = []
                excess = []
                label = kind
                for index in range(options.seeds):
                    rng = np.random.default_rng(options.seed + 7717 * index + 3 * dimension)
                    points, density = graded_cloud(
                        count, dimension, options.signal_rank, level, rng, options.signal_slope
                    )
                    view, label = represent(
                        points, kind, options.signal_rank, rng, options.jl_epsilon
                    )
                    squared = _squared_matrix(view)
                    order = min(options.neighbours, count - 1)
                    partitioned = np.partition(squared, order, axis=1)[:, order]
                    reference2 = float(np.median(partitioned))
                    counts = np.count_nonzero(squared <= reference2, axis=1) - 1
                    spearmans.append(float(stats.spearmanr(counts, density).statistic))
                    mean = float(counts.mean())
                    deviation = float(counts.std(ddof=0))
                    cvs.append(deviation / mean if mean > 0.0 else float("nan"))
                    excess.append(
                        (deviation / mean if mean > 0.0 else float("nan"))
                        * math.sqrt(max(mean, TINY))
                    )
                rows.append(
                    [
                        dimension,
                        options.signal_rank,
                        "%.3f" % level,
                        "%.2f" % (level * math.sqrt(2.0 * dimension)),
                        label,
                        summarise(spearmans),
                        summarise(cvs),
                        summarise(excess),
                    ]
                )
    print_table(
        "(c bis) variete plate de rang r a densite latente connue (rapport exp(%.1f) = %.0f), "
        "n=%d : correlation de Spearman entre comptage et densite vraie"
        % (options.signal_slope, math.exp(options.signal_slope), count),
        [
            "d",
            "r",
            "bruit/coord",
            "bruit*sqrt(2d)",
            "representation",
            "spearman",
            "cv",
            "exces Poisson",
        ],
        rows,
    )
    return rows


def _squared_matrix(points):
    """Matrice des distances au carre, bornee a zero par le bas."""
    norms = np.einsum("ij,ij->i", points, points)
    gram = points @ points.T
    return np.maximum(norms[:, None] + norms[None, :] - 2.0 * gram, 0.0)


def table_dimsweep(options, ledger):
    """Ambiant contre intrinseque : `d` varie, `r` reste fixe."""
    rows = []
    count = options.n_list[0]
    order = options.dimsweep_order
    for name, intrinsic in (
        ("flat", 2),
        ("flat_noise", 2),
        ("roll", 2),
        ("roll_noise", 2),
        ("uniform", 0),
    ):
        for dimension in options.dimsweep_dims:
            if name in ("roll", "roll_noise") and dimension < 3:
                continue
            target = dimension if name == "uniform" else intrinsic
            for kind in options.representations:
                shares = []
                contrasts = []
                label = kind
                for index in range(options.seeds):
                    rng = np.random.default_rng(options.seed + 31337 * index + 17 * dimension)
                    points = make_cloud(name, count, dimension, target, options.noise, rng)
                    view, label = represent(points, kind, target, rng, options.jl_epsilon)
                    subsets, _complete = subset_sample(count, order, options.subsets, rng)
                    result = measure_order(view, order, subsets, ledger)
                    shares.append(result["share"])
                    contrasts.append(density_contrast(view, options.neighbours, rng)["cv"])
                rows.append(
                    [
                        name,
                        dimension,
                        target,
                        label,
                        order,
                        summarise(shares),
                        summarise(contrasts),
                    ]
                )
    print_table(
        "(a)+(c) balayage de la dimension ambiante a dimension intrinseque fixee "
        "(n=%d, bruit=%.3f)" % (count, options.noise),
        ["famille", "d", "r", "representation", "k", "part libre", "cv du comptage"],
        rows,
    )
    return rows


def table_noise(options, ledger):
    """Le bruit ambiant : seuil de bascule et reparation par projection."""
    rows = []
    count = options.n_list[0]
    order = options.dimsweep_order
    for dimension in options.noise_dims:
        for level in options.noise_levels:
            for kind in ("raw", "pca"):
                shares = []
                contrasts = []
                distortions = []
                label = kind
                for index in range(options.seeds):
                    rng = np.random.default_rng(options.seed + 999331 * index + dimension)
                    points = make_cloud("flat_noise", count, dimension, 2, level, rng)
                    view, label = represent(points, kind, 2, rng, options.jl_epsilon)
                    subsets, _complete = subset_sample(count, order, options.subsets, rng)
                    result = measure_order(view, order, subsets, ledger)
                    shares.append(result["share"])
                    contrasts.append(density_contrast(view, options.neighbours, rng)["cv"])
                    distortions.append(pair_distortion(points, view, rng))
                rows.append(
                    [
                        dimension,
                        "%.3f" % level,
                        "%.2f" % (level * math.sqrt(2.0 * dimension)),
                        label,
                        summarise(shares),
                        summarise(contrasts),
                        summarise(distortions),
                    ]
                )
    print_table(
        "(a)+(c) variete plate r=2 plus bruit ambiant, k=%d, n=%d" % (order, count),
        [
            "d",
            "bruit/coord",
            "bruit*sqrt(2d)",
            "representation",
            "part libre",
            "cv du comptage",
            "distorsion paires",
        ],
        rows,
    )
    return rows


def table_repr(options, ledger):
    """Les trois representations sur (a) et (b)."""
    rows = []
    count = options.n_list[0]
    for name, dimension, intrinsic in FAMILIES:
        for order in _orders_for(count, options.orders):
            if order not in (2, options.dimsweep_order):
                continue
            for kind in options.representations:
                shares = []
                means = []
                deviations = []
                label = kind
                for index in range(options.seeds):
                    rng = np.random.default_rng(options.seed + 224737 * index + count)
                    points = make_cloud(name, count, dimension, intrinsic, options.noise, rng)
                    view, label = represent(points, kind, intrinsic, rng, options.jl_epsilon)
                    scale = _median_pair_distance(view, rng)
                    if scale <= 0.0:
                        continue
                    subsets, _complete = subset_sample(count, order, options.subsets, rng)
                    result = measure_order(view, order, subsets, ledger)
                    shares.append(result["share"])
                    means.append(result["radius_mean"] / scale)
                    deviations.append(result["radius_std"] / scale)
                rows.append(
                    [
                        _cloud_label(name, dimension, intrinsic),
                        order,
                        label,
                        summarise(shares),
                        summarise_pair(means, deviations),
                    ]
                )
    print_table(
        "(a)+(b) apres changement de representation (n=%d)" % count,
        ["famille", "k", "representation", "part libre", "rayon normalise"],
        rows,
    )
    return rows


# ---------------------------------------------------------------------------
# Degat de la projection sur la tour exacte
# ---------------------------------------------------------------------------


def quantise(points, bits):
    """Quantifie un nuage sur une grille entiere d'amplitude `2^bits`.

    Le resultat est un tuple de tuples d'entiers PYTHON : un entier numpy
    dans un `Fraction` deborde silencieusement en 64 bits et casse
    l'exactitude, ce qui est exactement ce que la doctrine interdit.
    """
    centred = points - points.mean(axis=0)
    spread = float(np.max(np.abs(centred)))
    if spread <= 0.0:
        spread = 1.0
    scaled = np.rint(centred / spread * float(1 << bits))
    return tuple(tuple(int(value) for value in row) for row in scaled)


def _kendall(left, right):
    """Tau de Kendall entre deux listes de niveaux (paires concordantes)."""
    total = 0
    concordant = 0
    discordant = 0
    size = len(left)
    for first in range(size):
        for second in range(first + 1, size):
            a = left[first] - left[second]
            b = right[first] - right[second]
            if a == 0 or b == 0:
                total += 1
                continue
            total += 1
            if (a > 0) == (b > 0):
                concordant += 1
            else:
                discordant += 1
    if total == 0:
        return float("nan"), 0
    return (concordant - discordant) / total, discordant


def event_sequence(tower, order):
    """Suite des evenements de l'ordre, NIVEAUX ABSOLUS EFFACES.

    Le digest de `FullTower` contient la dimension ambiante et les niveaux
    exacts : il ne peut jamais coincider entre deux representations, meme
    quand celles-ci sont isometriques a une rotation pres. La comparaison
    utile est la partie combinatoire du foncteur : la suite ordonnee des
    niveaux critiques, avec a chaque niveau les sommets qui apparaissent et
    les multifusions qui s'y produisent, les identifiants d'observations
    etant conserves par la projection. Deux tours ont la meme suite si et
    seulement si elles donnent le meme `pi_0(L_k(.))` a reparametrage
    croissant de l'axe des echelles pres.
    """
    state = tower.states[order]
    grouped = {}
    for level, vertex in state.births:
        grouped.setdefault(level, ([], []))[0].append(tuple(vertex))
    for level, arity, _witnesses, representatives, _unions, result in state.merges:
        grouped.setdefault(level, ([], []))[1].append(
            (arity, tuple(sorted(representatives)), tuple(result))
        )
    return tuple(
        (tuple(sorted(grouped[level][0])), tuple(sorted(grouped[level][1])))
        for level in sorted(grouped)
    )


def free_count_strict(points, order):
    """Nombre de parties de cardinal `order` a boule englobante VIDE.

    Convention de la specification : la boule est FERMEE, donc un point
    etranger exactement sur la sphere compte comme interieur (il donne
    `beta(F union {p}) = beta(F)`, donc une coface de meme niveau, donc
    aucune naissance de composante). Un doute numerique est donc tranche du
    cote non libre : ce comptage est un MINORANT du nombre de naissances.
    """
    cloud = np.asarray(points, dtype=float)
    norms = np.einsum("ij,ij->i", cloud, cloud)
    free = 0
    for subset in combinations(range(len(cloud)), order):
        centre, radius2, _support, _ok, _steps = meb_certified(cloud[list(subset)])
        distances = norms - 2.0 * (cloud @ centre) + float(centre @ centre)
        scale = max(radius2, 1.0)
        relative = (distances - radius2) / scale
        relative[list(subset)] = 1.0
        if int(np.count_nonzero(relative < 1e-9)) == 0:
            free += 1
    return free


def event_sequence(tower, order):
    """Suite des evenements de l'ordre, NIVEAUX ABSOLUS EFFACES.

    Le digest de `FullTower` contient la dimension ambiante et les niveaux
    exacts : il ne peut jamais coincider entre deux representations, meme
    quand celles-ci sont isometriques a une rotation pres. La comparaison
    utile est la partie combinatoire du foncteur : la suite ordonnee des
    niveaux critiques, avec a chaque niveau les naissances de composantes et
    les multifusions qui s'y produisent, les identifiants d'observations
    etant conserves par la projection. Deux tours ont la meme suite si et
    seulement si elles donnent le meme `pi_0(L_k(.))` a reparametrage
    croissant de l'axe des echelles pres.
    """
    state = tower.states[order]
    grouped = {}
    for level, component in state.component_births:
        grouped.setdefault(level, ([], []))[0].append(tuple(component))
    for level, arity, _witnesses, representatives, _unions, result in state.merges:
        grouped.setdefault(level, ([], []))[1].append(
            (arity, tuple(sorted(representatives)), tuple(result))
        )
    return tuple(
        (tuple(sorted(grouped[level][0])), tuple(sorted(grouped[level][1])))
        for level in sorted(grouped)
    )


TOWER_FAMILIES = (
    ("flat", 20, 2, 0.0),
    ("flat_noise", 20, 2, 0.05),
    ("uniform", 20, 20, 0.0),
)


def table_tower(options, ledger):
    """La projection preserve-t-elle pi_0(L_k(a)) ? Tour exacte, n <= 9."""
    del ledger
    cross = []
    damage = []
    count = options.tower_n
    for name, dimension, intrinsic, level in TOWER_FAMILIES:
        for index in range(options.tower_seeds):
            rng = np.random.default_rng(options.seed + 555 * index)
            points = make_cloud(name, count, dimension, intrinsic, level, rng)
            grid = quantise(points, options.tower_bits)
            reference = FullTower(grid, options.tower_k)
            reference_digest = reference.digest()
            exact_maps = {}
            reference_sequence = {}
            for order in range(1, options.tower_k + 1):
                exact_maps[order] = projected_ultrametric(reference, order)
                reference_sequence[order] = event_sequence(reference, order)
                state = reference.states[order]
                mine = free_count_strict(np.asarray(grid, dtype=float), order)
                cross.append(
                    [
                        name,
                        index,
                        reference_digest[:12],
                        order,
                        math.comb(count, order),
                        len(state.vertex_appearances),
                        len(state.component_births),
                        mine,
                        len(state.component_births) - mine,
                    ]
                )
            for kind in ("rot", "pca", "jl", "whiten"):
                view, label = represent(points, kind, intrinsic, rng, options.jl_epsilon)
                candidate = FullTower(quantise(view, options.tower_bits), options.tower_k)
                distortion = pair_distortion(points, view, rng, budget=400)
                for order in range(1, options.tower_k + 1):
                    exact = exact_maps[order]
                    other = projected_ultrametric(candidate, order)
                    pairs = sorted(pair for pair in exact if exact[pair] is not None)
                    usable = [pair for pair in pairs if other.get(pair) is not None]
                    left = [exact[pair] for pair in usable]
                    right = [other[pair] for pair in usable]
                    tau, discordant = _kendall(left, right)
                    same = event_sequence(candidate, order) == reference_sequence[order]
                    damage.append(
                        [
                            name,
                            index,
                            label,
                            "%.2f" % distortion,
                            order,
                            "oui" if same else "non",
                            len(reference.states[order].component_births),
                            len(candidate.states[order].component_births),
                            "%.3f" % tau if not math.isnan(tau) else "n/a",
                            discordant,
                            len(usable),
                        ]
                    )
    print_table(
        "(a) contre-verification : ma part libre contre les naissances de composantes "
        "de la tour exacte (n=%d, grille 2^%d)" % (count, options.tower_bits),
        [
            "famille",
            "graine",
            "digest ref",
            "k",
            "C(n,k)",
            "apparitions",
            "naissances",
            "libres (ma mesure)",
            "ecart",
        ],
        cross,
    )
    print_table(
        "degat de la projection sur la tour FULL exacte (n=%d, grille 2^%d, k_max=%d)"
        % (count, options.tower_bits, options.tower_k),
        [
            "famille",
            "graine",
            "representation",
            "distorsion",
            "k",
            "suite egale",
            "naissances ref",
            "naissances proj",
            "tau",
            "discordantes",
            "paires",
        ],
        damage,
    )
    return cross, damage


def table_theoremes(options, ledger):
    """Les quatre enonces demontres, verifies par la mesure.

    Un document qui cite un theoreme sans le passer au meme instrument que
    ses tableaux demande a etre cru. Cette table est donc une PORTE : toute
    violation met le code de sortie a 3.
    """
    rows = []
    rng = np.random.default_rng(options.seed)

    # T1 : beta ne depend que de la matrice des distances.
    worst = 0.0
    cases = 0
    for _trial in range(options.theorem_cases):
        dimension = int(rng.integers(2, 40))
        order = int(rng.integers(2, 12))
        block = rng.standard_normal((order, dimension))
        rotation = _orthonormal(dimension, dimension, rng)
        moved = block @ rotation + rng.standard_normal(dimension) * 5.0
        _c, left, _s, _ok, _st = meb_certified(block)
        _c2, right, _s2, _ok2, _st2 = meb_certified(moved)
        worst = max(worst, abs(right - left) / max(left, TINY))
        cases += 1
    rows.append(["T1 isometrie : beta invariant", cases, "%.3e" % worst, "1e-12"])
    if worst > 1e-12:
        ledger.failures.append("T1 : beta n'est pas invariant par isometrie")

    # T2 : une projection orthogonale contracte beta.
    worst = 0.0
    cases = 0
    for _trial in range(max(options.theorem_cases // 4, 1)):
        dimension = int(rng.integers(3, 60))
        rank = int(rng.integers(1, dimension))
        count = int(rng.integers(4, 12))
        order = int(rng.integers(2, min(count, 7)))
        points = rng.standard_normal((count, dimension))
        basis = _orthonormal(dimension, rank, rng)
        projected = points @ (basis @ basis.T)
        for subset in combinations(range(count), order):
            _c, left, _s, _ok, _st = meb_certified(points[list(subset)])
            _c2, right, _s2, _ok2, _st2 = meb_certified(projected[list(subset)])
            worst = max(worst, (right - left) / max(left, TINY))
            cases += 1
    rows.append(["T2 projection : beta decroit", cases, "%.3e" % worst, "1e-12"])
    if worst > 1e-12:
        ledger.failures.append("T2 : une projection orthogonale a augmente beta")

    # T3 : blanchiment en rang n - 1 : simplexe regulier exact.
    worst = 0.0
    cases = 0
    for count, dimension in ((9, 20), (9, 200), (12, 11), (12, 50), (20, 400)):
        for family in ("uniform", "flat_noise"):
            points = make_cloud(family, count, dimension, 2, 0.05, rng)
            view = whiten(points)
            if view.shape[1] != count - 1:
                ledger.failures.append("T3 : rang effectif inattendu")
                continue
            target_pair = 2.0 * (count - 1)
            for left_index, right_index in combinations(range(count), 2):
                delta = view[left_index] - view[right_index]
                worst = max(worst, abs(float(delta @ delta) / target_pair - 1.0))
            for order in (2, 3, 5):
                target = (count - 1) * (order - 1) / order
                for subset in list(combinations(range(count), order))[:40]:
                    _c, radius2, _s, _ok, _st = meb_certified(view[list(subset)])
                    worst = max(worst, abs(radius2 / target - 1.0))
                    cases += 1
    rows.append(["T3 blanchiment : simplexe regulier", cases, "%.3e" % worst, "1e-9"])
    if worst > 1e-9:
        ledger.failures.append("T3 : le nuage blanchi n'est pas un simplexe regulier")

    print_table(
        "enonces demontres, verifies a l'instrument (ecart relatif maximal)",
        ["enonce", "cas", "ecart max", "plafond"],
        rows,
    )

    # T4 : Johnson-Lindenstrauss, distorsion du rayon contre celle des paires.
    jl_rows = []
    for dimension, target in ((200, 96), (200, 20), (50, 20), (20, 10)):
        count, order = 40, 5
        points = rng.standard_normal((count, dimension))
        matrix = rng.standard_normal((dimension, target)) / math.sqrt(target)
        mapped = points @ matrix
        pair_worst = 0.0
        for left_index, right_index in combinations(range(count), 2):
            left = float(np.linalg.norm(points[left_index] - points[right_index]))
            right = float(np.linalg.norm(mapped[left_index] - mapped[right_index]))
            pair_worst = max(pair_worst, abs(right / left - 1.0))
        radius_worst = 0.0
        for subset in list(combinations(range(count), order))[:300]:
            _c, left, _s, _ok, _st = meb_certified(points[list(subset)])
            _c2, right, _s2, _ok2, _st2 = meb_certified(mapped[list(subset)])
            radius_worst = max(radius_worst, abs(math.sqrt(right / left) - 1.0))
        jl_rows.append(
            [
                dimension,
                target,
                "%.3f" % math.sqrt(4.0 * math.log(count) / target),
                "%.3f" % pair_worst,
                "%.3f" % radius_worst,
            ]
        )
    print_table(
        "T4 Johnson-Lindenstrauss : la distorsion du rayon est majoree par celle des paires",
        ["d", "m", "eps theorique", "distorsion paires max", "distorsion rayon max"],
        jl_rows,
    )
    return rows, jl_rows


# ---------------------------------------------------------------------------
# Interface
# ---------------------------------------------------------------------------


def _int_list(text):
    return [int(item) for item in text.split(",") if item.strip()]


def _float_list(text):
    return [float(item) for item in text.split(",") if item.strip()]


def _str_list(text):
    return [item.strip() for item in text.split(",") if item.strip()]


def build_parser():
    """Analyseur de ligne de commande."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--table",
        default="all",
        choices=(
            "selftest",
            "exhaustive",
            "free",
            "radius",
            "contrast",
            "signal",
            "theoremes",
            "dimsweep",
            "noise",
            "repr",
            "tower",
            "all",
        ),
    )
    parser.add_argument("--seed", type=int, default=31)
    parser.add_argument("--seeds", type=int, default=5)
    parser.add_argument("--orders", type=_int_list, default=[2, 3, 5, 11])
    parser.add_argument("--n-list", type=_int_list, default=[200, 800])
    parser.add_argument("--exhaustive-n", type=_int_list, default=[8, 11, 14])
    parser.add_argument("--subsets", type=int, default=240)
    parser.add_argument("--neighbours", type=int, default=10)
    parser.add_argument("--noise", type=float, default=0.05)
    parser.add_argument(
        "--noise-levels", type=_float_list, default=[0.0, 0.01, 0.03, 0.1, 0.3, 1.0]
    )
    parser.add_argument("--noise-dims", type=_int_list, default=[20, 200])
    parser.add_argument("--dimsweep-dims", type=_int_list, default=[2, 3, 5, 20, 50, 200])
    parser.add_argument("--dimsweep-order", type=int, default=2)
    parser.add_argument(
        "--representations", type=_str_list, default=["raw", "whiten", "pca", "jl"]
    )
    parser.add_argument("--jl-epsilon", type=float, default=0.5)
    parser.add_argument("--signal-dims", type=_int_list, default=[2, 5, 20, 50, 200])
    parser.add_argument("--signal-noise", type=_float_list, default=[0.0, 0.03, 0.1, 0.3])
    parser.add_argument("--signal-rank", type=int, default=2)
    parser.add_argument("--signal-slope", type=float, default=3.0)
    parser.add_argument("--theorem-cases", type=int, default=200)
    parser.add_argument("--selftest-cases", type=int, default=60)
    parser.add_argument("--tower-n", type=int, default=9)
    parser.add_argument("--tower-k", type=int, default=3)
    parser.add_argument("--tower-bits", type=int, default=9)
    parser.add_argument("--tower-seeds", type=int, default=2)
    parser.add_argument("--min-balls", type=int, default=1000)
    parser.add_argument("--min-selftest", type=int, default=200)
    parser.add_argument("--max-undecided", type=float, default=1e-3)
    return parser


def main(argv=None):
    """Point d'entree : imprime les tables demandees, renvoie le code."""
    options = build_parser().parse_args(argv)
    ledger = Ledger(options.min_balls, options.min_selftest, options.max_undecided)
    started = time.time()
    print("phase=exploration_ehgp_hors_registre")
    print("backend=python_reference")
    print("profile=any_dimension_rational_exact")
    print("mode=audit_independant_math_and_architecture")
    print("public_status=not_claimed")
    print(
        "graine de base=%d graines=%d parties tirees=%d"
        % (options.seed, options.seeds, options.subsets)
    )
    wanted = options.table
    if wanted in ("selftest", "all"):
        table_selftest(options, ledger)
    if wanted in ("theoremes", "all"):
        table_theoremes(options, ledger)
    if wanted in ("exhaustive", "all"):
        table_exhaustive(options, ledger)
    if wanted in ("free", "all"):
        table_free(options, ledger)
    if wanted in ("radius", "all"):
        table_radius(options, ledger)
    if wanted in ("contrast", "all"):
        table_contrast(options, ledger)
    if wanted in ("signal", "all"):
        table_signal(options, ledger)
    if wanted in ("dimsweep", "all"):
        table_dimsweep(options, ledger)
    if wanted in ("noise", "all"):
        table_noise(options, ledger)
    if wanted in ("repr", "all"):
        table_repr(options, ledger)
    if wanted in ("tower", "all"):
        table_tower(options, ledger)
    ball_tables = ("exhaustive", "free", "radius", "dimsweep", "noise", "repr", "all")
    if wanted not in ("selftest", "all"):
        ledger.min_selftest = 0
    if wanted not in ball_tables:
        ledger.min_balls = 0
    code, lines = ledger.verdict()
    print("")
    print("planchers de couverture")
    print("-" * 23)
    for text in lines:
        print(text)
    print("duree=%.1f s code=%d" % (time.time() - started, code))
    return code


if __name__ == "__main__":
    sys.exit(main())
