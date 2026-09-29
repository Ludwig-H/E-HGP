"""Scenes synthetiques 3D du banc v10.

PROVENANCE : porte de morsehgp3D_v9/experiments/synthetic_bench_20260928/bench_datasets.py (commit ce8a649dd,
meme depot, MIT), familles et generateurs inchanges ; seule la quantification change (`quantize18` : pas
isotrope sur 18 bits, commun a toutes les methodes). Les niveaux calibres de la v9 sont gardes comme
PARAMETRES geometriques ; l'audit a montre qu'ils ne mesurent pas une difficulte stable (L09-F4) : aucune
conclusion ne s'appuie sur leur nom.
"""

import hashlib
import math

import numpy as np

FAMILIES = ('spherical', 'anisotropic', 'heteroscedastic', 'unbalanced', 'shells', 'bridge', 'hierarchical',
            'filaments')
LEVELS = ('easy', 'medium', 'hard', 'extreme')
MAX_GROUPS = 64
SPEC_KEYS = frozenset({'family', 'n', 'groups', 'level', 'noise_fraction', 'seed'})

# Separation (distance minimale entre centres, en ecarts-types de reference)
# par famille et par niveau. Calibree le 28 septembre 2026 pour viser, sur
# HDBSCAN(min_cluster_size=20) a n=800, un ARI d'environ 0,95 / 0,70 / 0,40
# et 0,15. Les familles sans centres gaussiens (shells, bridge, filaments)
# reglent leur propre parametre de difficulte avec la meme echelle.
SEPARATION = {
    'spherical': dict(easy=6.8, medium=5.2, hard=4.6, extreme=4.0),
    'anisotropic': dict(easy=8.2, medium=5.6, hard=4.6, extreme=3.8),
    'heteroscedastic': dict(easy=15.0, medium=5.2, hard=3.2, extreme=1.5),
    'unbalanced': dict(easy=4.6, medium=3.4, hard=2.5, extreme=2.0),
    'shells': dict(easy=4.3, medium=4.05, hard=3.9, extreme=3.7),
    'bridge': dict(easy=7.0, medium=5.3, hard=5.0, extreme=3.9),
    'hierarchical': dict(easy=4.0, medium=6.3, hard=6.8, extreme=9.0),
    'filaments': dict(easy=2.9, medium=2.15, hard=1.6, extreme=1.25),
}

# ARI mesure de HDBSCAN(min_cluster_size=20) au point central du plan
# (n = 2000, 8 groupes, sans bruit, moyenne de cinq graines), le 28 septembre
# 2026. C'est la definition operatoire des quatre niveaux : une famille est
# `hard` parce que la reference y tombe a 0,4, pas parce qu'un parametre est
# grand. La porte `test_levels_match_the_published_calibration` refuse une
# derive de plus de 0,15 et toute inversion entre deux niveaux.
#
# Deux familles ne descendent pas jusqu'a 0,15 et c'est leur propos :
# `hierarchical` bute sur 0,46, l'ARI d'un surdecoupage systematique en trois
# sous-amas. Franchir ce plancher demande de choisir le bon NIVEAU de la
# hierarchie, pas un meilleur seuil.
CALIBRATION = {
    'spherical': dict(easy=0.96, medium=0.73, hard=0.32, extreme=0.16),
    'anisotropic': dict(easy=0.95, medium=0.66, hard=0.43, extreme=0.23),
    'heteroscedastic': dict(easy=0.97, medium=0.72, hard=0.42, extreme=0.18),
    'unbalanced': dict(easy=0.96, medium=0.70, hard=0.40, extreme=0.16),
    'shells': dict(easy=1.00, medium=0.84, hard=0.34, extreme=0.18),
    'bridge': dict(easy=0.96, medium=0.74, hard=0.46, extreme=0.15),
    'hierarchical': dict(easy=1.00, medium=0.70, hard=0.51, extreme=0.46),
    'filaments': dict(easy=0.95, medium=0.75, hard=0.34, extreme=0.14),
}
CALIBRATION_POINT = dict(n=2000, groups=8, noise_fraction=0.0, min_cluster_size=20, seeds=(11, 12, 13, 14, 15))

# Dimension intrinseque du support de chaque famille. C'est l'exposant `z` du
# poids psi(t) = 1/t^z de la tour : une famille filamentaire vit en dimension
# un, une famille surfacique en dimension deux, une gaussienne en dimension
# trois. Le banc n'impose rien, il publie la valeur attendue pour que le choix
# de `z` d'une campagne se lise a cote de son score.
INTRINSIC_DIMENSION = {
    'spherical': 3, 'anisotropic': 3, 'heteroscedastic': 3, 'unbalanced': 3,
    'bridge': 3, 'hierarchical': 3, 'shells': 2, 'filaments': 1,
}


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def validate_spec(spec):
    """Specification complete et close ; aucune valeur par defaut implicite."""
    need(isinstance(spec, dict) and set(spec) == SPEC_KEYS, 'spec keys: ' + ', '.join(sorted(SPEC_KEYS)))
    family, level = spec['family'], spec['level']
    need(family in FAMILIES, 'unknown family ' + str(family))
    need(level in LEVELS, 'unknown level ' + str(level))
    n, groups, seed = spec['n'], spec['groups'], spec['seed']
    need(isinstance(n, int) and 8 <= n <= 2_000_000, 'n in 8..2e6')
    need(isinstance(groups, int) and 2 <= groups <= MAX_GROUPS, 'groups in 2..64')
    need(isinstance(seed, int) and 0 <= seed < 2 ** 63, 'seed is a non-negative integer')
    noise = spec['noise_fraction']
    need(isinstance(noise, (int, float)) and 0.0 <= float(noise) < 0.9, 'noise_fraction in [0, 0.9)')
    need(n >= 16 * groups, 'at least sixteen points per group after the noise and bridge budgets')
    return dict(spec, noise_fraction=float(noise))


def centres(groups):
    """Centres sur une grille entiere, distance minimale ramenee a un.

    Les points sont pris gloutonnement sur la grille 4x4x4 en maximisant la
    distance minimale aux deja choisis : meme geometrie que le banc du
    27 septembre, donc les deux campagnes restent confrontables.
    """
    grid = [(x, y, z) for x in (-3, -1, 1, 3) for y in (-3, -1, 1, 3) for z in (-3, -1, 1, 3)]
    selected = [grid[0]]
    while len(selected) < groups:
        rest = [p for p in grid if p not in selected]
        need(rest, 'the 4x4x4 grid holds at most 64 centres')
        selected.append(max(rest, key=lambda p: min(sum((a - b) ** 2 for a, b in zip(p, q)) for q in selected)))
    squared = min((sum((a - b) ** 2 for a, b in zip(x, y)) for i, x in enumerate(selected) for y in selected[:i]),
                  default=1)
    out = np.asarray(selected, dtype=np.float64)
    out -= out.mean(axis=0)
    return out / math.sqrt(squared)


def _rotation(rng):
    """Rotation uniforme (QR d'une gaussienne), determinant ramene a +1."""
    q, r = np.linalg.qr(rng.standard_normal((3, 3)))
    q = q * np.sign(np.diag(r))
    if np.linalg.det(q) < 0:
        q[:, 0] = -q[:, 0]
    return q


def _sizes(n, groups, family, rng):
    """Tailles des groupes : egales, ou tres desequilibrees (rapport 32)."""
    if family == 'unbalanced':
        weights = np.array([32.0 / (2 ** min(j, 5)) for j in range(groups)])
    else:
        weights = np.ones(groups)
    weights /= weights.sum()
    sizes = np.maximum(4, np.floor(weights * n).astype(int))
    while sizes.sum() > n:
        sizes[int(np.argmax(sizes))] -= 1
    while sizes.sum() < n:
        sizes[int(np.argmin(sizes))] += 1
    return sizes


def _gaussian_family(family, groups, sizes, delta, rng):
    """Familles a composantes gaussiennes : spherique, anisotrope, densites."""
    middles = delta * centres(groups)
    points, labels = [], []
    for j in range(groups):
        if family == 'anisotropic':
            scale = np.diag([2.0, 1.0, 0.35])
        elif family == 'heteroscedastic':
            scale = np.diag([(0.4, 1.0, 2.5)[j % 3]] * 3)
        else:
            scale = np.eye(3)
        transform = _rotation(rng) @ scale
        points.append(middles[j] + rng.standard_normal((sizes[j], 3)) @ transform.T)
        labels.append(np.full(sizes[j], j, dtype=np.int64))
    return np.vstack(points), np.concatenate(labels)


SHELL_THICKNESS = 0.15
FILAMENT_LENGTH = 3.0
HIERARCHICAL_GROUP_GAP = 8.0


def shell_radius(count):
    """Rayon d'une sphere creuse portant `count` points a densite surfacique un.

    L'unite de longueur du banc est l'espacement moyen entre voisins ; en la
    fixant, la difficulte d'une famille surfacique se lit directement comme
    l'ecart entre surfaces, et non comme un rayon qui change la densite.
    """
    return math.sqrt(max(int(count), 1) / (4.0 * math.pi))


def _shells(groups, sizes, delta, rng):
    """Spheres creuses separees : support de dimension intrinseque deux.

    Les coquilles concentriques du premier jet etaient degenerees : l'aire
    croissant en r^2, la distance entre voisins d'une meme coquille depassait
    l'ecart entre coquilles des le deuxieme rayon, et toute methode de densite
    echouait a tous les niveaux (mesure du 28 septembre : ARI nul jusqu'en
    `easy`). Ici chaque groupe est une sphere creuse a densite surfacique un,
    et `delta` est l'ecart entre surfaces en nombre d'espacements.

    C'est la famille ou l'exposant du poids doit valoir deux : le support est
    une surface, pas un volume.
    """
    radii = np.array([shell_radius(size) for size in sizes], dtype=np.float64)
    middles = (2.0 * float(radii.max()) + delta) * centres(groups)
    points, labels = [], []
    for j in range(groups):
        direction = rng.standard_normal((sizes[j], 3))
        direction /= np.linalg.norm(direction, axis=1, keepdims=True)
        thickness = SHELL_THICKNESS * rng.standard_normal((sizes[j], 1))
        points.append(middles[j] + direction * (radii[j] + thickness))
        labels.append(np.full(sizes[j], j, dtype=np.int64))
    return np.vstack(points), np.concatenate(labels)


def bridge_budget(count, groups):
    """Points reserves aux ponts : deux pour cent du budget, pris dessus."""
    pairs = max(1, groups // 2)
    return pairs * max(2, int(0.02 * count) // pairs)


def _bridge(groups, sizes, delta, rng, bridge_total):
    """Groupes spheriques relies deux a deux par un pont mince et peu dense.

    Le pont est le piege du chainage : une methode de liaison simple fusionne
    par le pont, une methode de densite doit le couper. Ses points sont du
    bruit (-1) et sont pris sur le budget, jamais ajoutes.
    """
    middles = delta * centres(groups)
    points, labels = [], []
    for j in range(groups):
        points.append(middles[j] + rng.standard_normal((sizes[j], 3)))
        labels.append(np.full(sizes[j], j, dtype=np.int64))
    pairs = max(1, groups // 2)
    per_pair = bridge_total // pairs
    for index, j in enumerate(range(0, groups - 1, 2)):
        share = per_pair + (bridge_total - per_pair * pairs if index == 0 else 0)
        t = rng.random((share, 1))
        line = middles[j] + t * (middles[j + 1] - middles[j])
        points.append(line + 0.25 * rng.standard_normal((share, 3)))
        labels.append(np.full(share, -1, dtype=np.int64))
    return np.vstack(points), np.concatenate(labels)


def _hierarchical(groups, sizes, delta, rng):
    """Amas d'amas : la difficulte est le choix du NIVEAU, pas l'ecart.

    Les groupes sont toujours largement separes (`HIERARCHICAL_GROUP_GAP`) et
    gardent un ecart-type de un : seule leur structure interne change. `delta`
    est le rapport entre l'ecart des trois sous-amas d'un meme groupe et leur
    dispersion, et l'echelle interne compense pour que l'ecart-type du groupe
    reste un. A rapport nul le groupe est une gaussienne ordinaire, que toute
    methode resout ; a rapport eleve la sous-structure est criante et une
    methode a coupe unique la prend pour la verite, donc surdecoupe.

    La mesure du 28 septembre a montre que l'ecart entre groupes ne pilotait
    rien ici : l'ARI de HDBSCAN plafonnait a 0,46 de 3 a 9, par surdecoupage.
    C'est ce plafond que la famille doit exposer, pas masquer.
    """
    middles = HIERARCHICAL_GROUP_GAP * centres(groups)
    inner = 1.0 / math.sqrt(1.0 + (delta ** 2) / 9.0)
    points, labels = [], []
    for j in range(groups):
        share = _sizes(int(sizes[j]), 3, 'equal', rng)
        local = _rotation(rng) @ ((delta * inner) * centres(3)).T
        for c in range(3):
            points.append(middles[j] + local[:, c] + inner * rng.standard_normal((share[c], 3)))
            labels.append(np.full(share[c], j, dtype=np.int64))
    return np.vstack(points), np.concatenate(labels)


def _filaments(groups, sizes, delta, rng):
    """Filaments : segments fins d'orientations libres, proches du LiDAR."""
    middles = delta * centres(groups)
    points, labels = [], []
    for j in range(groups):
        direction = _rotation(rng)[:, 0]
        t = (rng.random((sizes[j], 1)) - 0.5) * FILAMENT_LENGTH
        points.append(middles[j] + t * direction + 0.18 * rng.standard_normal((sizes[j], 3)))
        labels.append(np.full(sizes[j], j, dtype=np.int64))
    return np.vstack(points), np.concatenate(labels)


def generate(spec):
    """Rend (points float64, etiquettes int64, metadonnees).

    Le bruit uniforme est tire dans la boite englobante des groupes elargie
    de 10 % et porte l'etiquette -1.
    """
    spec = validate_spec(spec)
    family, groups, n = spec['family'], spec['groups'], spec['n']
    delta = SEPARATION[family][spec['level']]
    rng = np.random.default_rng(spec['seed'])
    noise_count = int(round(spec['noise_fraction'] * n))
    bridge_total = bridge_budget(n - noise_count, groups) if family == 'bridge' else 0
    sizes = _sizes(n - noise_count - bridge_total, groups, family, rng)
    if family in ('spherical', 'anisotropic', 'heteroscedastic', 'unbalanced'):
        points, labels = _gaussian_family(family, groups, sizes, delta, rng)
    elif family == 'shells':
        points, labels = _shells(groups, sizes, delta, rng)
    elif family == 'bridge':
        points, labels = _bridge(groups, sizes, delta, rng, bridge_total)
    elif family == 'hierarchical':
        points, labels = _hierarchical(groups, sizes, delta, rng)
    else:
        points, labels = _filaments(groups, sizes, delta, rng)
    if noise_count:
        low, high = points.min(axis=0), points.max(axis=0)
        margin = 0.1 * (high - low)
        points = np.vstack([points, rng.uniform(low - margin, high + margin, size=(noise_count, 3))])
        labels = np.concatenate([labels, np.full(noise_count, -1, dtype=np.int64)])
    order = rng.permutation(len(points))
    points, labels = points[order], labels[order]
    digest = hashlib.sha256(np.ascontiguousarray(points, dtype='<f8').tobytes()).hexdigest()
    meta = dict(spec, separation=delta, points=len(points), noise_points=int((labels < 0).sum()),
                groups_present=int(len(set(labels.tolist()) - {-1})), digest=digest)
    return points, labels, meta


def quantize18(points, labels, bits=18):
    """Pas isotrope h = etendue / (2^bits - 1) ; q = floor(x / h + 1/2). Les doublons de position sont retires
    (premiere occurrence gardee) pour TOUTES les methodes ; leur nombre est rendu."""
    low = points.min(axis=0)
    span = float((points.max(axis=0) - low).max())
    h = span / float((1 << bits) - 1) if span > 0 else 1.0
    grid = np.floor((points - low) / h + 0.5).astype(np.int64)
    grid = np.clip(grid, 0, (1 << bits) - 1)
    _, first = np.unique(grid, axis=0, return_index=True)
    keep = np.sort(first)
    return grid[keep].astype(np.uint32), labels[keep], len(grid) - len(keep), h


def quantize(points, millimetre=0.001, bits=18):
    """Grille entiere du moteur : pas fixe, origine au minimum, sans doublon.

    Rend (entiers uint32, echelle) ou leve si deux points tombent sur la meme
    case (le moteur refuse les positions dupliquees) ou si la scene deborde
    du domaine de `bits` bits.
    """
    need(points.ndim == 2 and points.shape[1] == 3, 'points is an n x 3 array')
    grid = np.floor((points - points.min(axis=0)) / millimetre + 0.5).astype(np.int64)
    span = int(grid.max()) if grid.size else 0
    need(span < (1 << bits), 'the scene exceeds the %d-bit domain (%d cells)' % (bits, span))
    unique = np.unique(grid, axis=0)
    need(len(unique) == len(grid), 'duplicate positions after quantisation')
    return grid.astype(np.uint32), millimetre
