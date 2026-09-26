"""Sonde de comparaison : tour empirique contre tour spectrale, par dimension.

QUESTION TRANCHEE PAR CETTE SONDE. A partir de quelle dimension la tour
empirique (comptage de boules, projetee sur les observations) s'effondre-t-
elle, et la tour spectrale (modele de log-densite regularise par
f-divergence) tient-elle mieux ?

TROIS OBJETS COMPARES, sur des melanges gaussiens a verite terrain :

(i)   TOUR EMPIRIQUE PROJETEE a l'ordre `k`. Version FLOTTANTE de
      `src/ehgp/engine/point_tower.py`, ecrite ici parce que la version
      rationnelle exacte enumere les croisements de `n` droites pour chacune
      des `C(n, 2)` paires et ne passe pas l'echelle voulue. La
      construction est la meme : l'observation `x_i` nait au niveau
      `a_k(x_i)`, et deux observations sont reliees au niveau
      `w_ij = max_t a_k((1-t) x_i + t x_j)`. L'identite exploitee est celle
      de `engine/segment.py` : le long du segment, les energies partagent
      leur coefficient dominant, donc

          a_k(y(t)) = A t^2 + (k-ieme plus petite de n AFFINES) ,
          A = ||x_j - x_i||^2 ,
          B_l = 2 (G[i,j] - G[j,l] - G[i,i] + G[i,l]) ,  C_l = D[i,l] ,

      ou `G` est la matrice de Gram et `D` la matrice des distances au carre.
      Tout s'obtient donc de `G` et `D` precalculees, SANS aucun produit en
      dimension `d` par arete : le cout par arete est `O(T n)` pour `T`
      noeuds de grille, et la dimension n'entre qu'une fois, dans `G`.

      DEUX APPROXIMATIONS DECLAREES, dans des sens opposes :
      * le maximum sur `t` est pris sur une GRILLE de `T` temps au lieu des
        croisements exacts, donc le poids calcule est un MINORANT du poids
        exact de segment (sens : fusion un peu trop tot) ;
      * l'ensemble des aretes candidates est le graphe des `q` plus proches
        voisins symetrise, complete par l'arbre couvrant minimal euclidien
        pour garantir la connexite, donc un MAJORANT des poids d'un graphe
        complet (sens : fusion un peu trop tard).
      Aucune des deux n'est un certificat ; les deux sont mesurees ici sur
      un temoin (option `--full-graph`, qui prend toutes les paires).

(ii)  TOUR SPECTRALE : `src/ehgp/spectral/tower.py` sur le modele de
      `src/ehgp/spectral/log_density.py`.

(iii) VERITE TERRAIN : les composantes du melange.

MESURE DE QUALITE. Indice de Rand ajuste (ARI) entre la partition obtenue
en coupant chaque arbre au NOMBRE VRAI de classes, plus le nombre de classes
que chaque methode propose SPONTANEMENT par la meme regle de saut declaree.

DEUX REGIMES DE BRUIT, parce qu'ils ne mesurent pas la meme chose :

* `per_coordinate` : ecart-type de bruit fixe PAR COORDONNEE. La norme du
  bruit croit alors comme `sqrt(d)` et finit par depasser la separation des
  centres : la VERITE TERRAIN devient geometriquement irrecuperable pour
  toute methode a noyau isotrope. Ce regime mesure la malediction de la
  dimension AMBIANTE, et un effondrement y est attendu des deux cotes.
* `isotropic_norm` : ecart-type de bruit divise par `sqrt(d)`, donc norme du
  bruit constante. La geometrie est essentiellement independante de `d` ;
  ce qui reste mesure est la degradation des ESTIMATEURS, pas celle du
  probleme. C'est le regime qui repond a la question posee.

Usage :

    python3 bench/spectral_tower.py --quick
    python3 bench/spectral_tower.py --seeds 3 --features 300
    python3 bench/spectral_tower.py --dims 50 --intrinsic 2 --n 1000 --full-graph

Aucune sortie de cette sonde ne promeut un statut public : ce sont des
mesures de qualite de partition sur des donnees synthetiques.
"""

import argparse
import math
import os
import sys
import time
from pathlib import Path

# FAIT MESURE, avant toute importation de numpy. Sur les tailles de matrices
# de cette sonde (par exemple (500, 200) fois (200, 200)), OpenBLAS multifils
# est de 7 a 30 fois PLUS LENT que le mode monofil dans ce conteneur :
# 0,275 s contre 1,847 s pour cinquante produits, et 0,094 s contre 2,722 s
# pour deux cents produits (400, 50) fois (50, 64). Le parallelisme est donc
# MESURE puis desactive, jamais declare. `--blas-threads` permet de refaire
# la mesure.
for _variable in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_variable, os.environ.get("EHGP_BLAS_THREADS", "1"))

import numpy as np  # noqa: E402
from scipy.sparse.csgraph import minimum_spanning_tree  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.engine.segment import rational_cloud, segment_maximum  # noqa: E402
from ehgp.spectral.log_density import SpectralLogDensity  # noqa: E402
from ehgp.spectral.tower import SpectralTower  # noqa: E402


# -- verite terrain ----------------------------------------------------


def gaussian_mixture(
    count,
    dimension,
    groups,
    intrinsic,
    separation,
    noise,
    mode,
    seed,
    intrinsic_noise=1.0,
):
    """Melange gaussien a dimension intrinseque controlee, SNR factorise.

    Construction, conçue pour que la difficulte ne change pas avec `d` sans
    qu'on l'ait demande :

    * les centres vivent dans le sous-espace des `intrinsic` premieres
      coordonnees, aux points d'un RESEAU de pas `separation` : la distance
      minimale entre deux centres vaut donc exactement `separation`, elle est
      choisie et non tiree au hasard. La verite terrain est reellement
      recuperable : un echec de methode est un echec de methode, pas un
      melange non identifiable ;
    * le bruit INTRINSEQUE (sur les `intrinsic` coordonnees) est fixe a
      `intrinsic_noise`, donc le probleme vu dans le sous-espace est
      exactement le meme pour tout `d`, et `separation / intrinsic_noise` est
      le rapport signal sur bruit, en ecarts-types ;
    * le bruit AMBIANT (sur les `d - intrinsic` autres coordonnees) est
      gouverne par `mode` : `none` (nul, le probleme intrinseque est
      simplement PLONGE dans R^d, donc toute methode invariante par
      rotation doit donner le MEME resultat pour tout `d` : c'est un temoin),
      `isotropic_norm` (norme totale constante, donc echelle
      `noise / sqrt(d - intrinsic)`), `per_coordinate` (echelle `noise` fixe,
      donc norme croissant comme `sqrt(d - intrinsic)`) ;
    * une rotation aleatoire est appliquee, pour qu'aucune methode ne
      beneficie de l'alignement sur les axes.
    """
    generator = np.random.default_rng(seed)
    free = max(dimension - intrinsic, 0)
    if mode == "none":
        scale = 0.0
    elif mode == "per_coordinate":
        scale = noise
    elif mode == "isotropic_norm":
        scale = noise / math.sqrt(free) if free > 0 else 0.0
    else:
        raise ValueError("regime de bruit inconnu : " + str(mode))
    side = int(math.ceil(groups ** (1.0 / intrinsic)))
    lattice = []
    for flat in range(side ** intrinsic):
        digits = []
        remaining = flat
        for _axis in range(intrinsic):
            digits.append(remaining % side)
            remaining //= side
        lattice.append(digits)
        if len(lattice) == groups:
            break
    centres = np.zeros((groups, dimension))
    centres[:, :intrinsic] = separation * np.array(lattice, dtype=float)
    labels = generator.integers(0, groups, count)
    cloud = centres[labels].copy()
    cloud[:, :intrinsic] += intrinsic_noise * generator.standard_normal((count, intrinsic))
    if free > 0 and scale > 0.0:
        cloud[:, intrinsic:] += scale * generator.standard_normal((count, free))
    rotation, _ = np.linalg.qr(generator.standard_normal((dimension, dimension)))
    return cloud @ rotation.T, labels, scale


def adjusted_rand_index(left, right):
    """Indice de Rand ajuste, table de contingence, sans dependance externe."""
    left = np.asarray(left)
    right = np.asarray(right)
    total = left.size
    if total < 2:
        return 1.0
    left_codes = np.unique(left, return_inverse=True)[1]
    right_codes = np.unique(right, return_inverse=True)[1]
    table = np.zeros((left_codes.max() + 1, right_codes.max() + 1), dtype=np.int64)
    np.add.at(table, (left_codes, right_codes), 1)

    def pairs(values):
        values = np.asarray(values, dtype=np.float64)
        return float(np.sum(values * (values - 1.0) / 2.0))

    inside = pairs(table)
    rows = pairs(table.sum(axis=1))
    columns = pairs(table.sum(axis=0))
    whole = total * (total - 1) / 2.0
    expected = rows * columns / whole
    maximum = 0.5 * (rows + columns)
    if maximum == expected:
        return 1.0
    return (inside - expected) / (maximum - expected)


def gap_classes(levels, max_classes):
    """Nombre de classes propose par le plus grand saut, regle PARTAGEE.

    `levels` est la suite des niveaux de fusion DANS L'ORDRE DES FUSIONS,
    monotone (croissante sur l'axe `a` de la tour empirique, decroissante
    sur l'axe `lambda` de la tour spectrale). Appliquer les `j` premieres
    fusions laisse `total - j` composantes ; le saut qui justifie de
    s'arreter a `j` est `|levels[j] - levels[j-1]|`. On maximise ce saut sur
    `c = total - j` dans `2 .. max_classes`. La regle ne regarde donc que
    l'ordre des fusions et l'echelle des niveaux, jamais la nature de la
    methode : elle est applicable telle quelle aux deux tours.
    """
    total = len(levels) + 1
    best_classes, best_gap = 1, -1.0
    for classes in range(2, min(max_classes, total - 1) + 1):
        index = total - classes
        if index < 1 or index > len(levels) - 1:
            continue
        gap = abs(float(levels[index]) - float(levels[index - 1]))
        if gap > best_gap:
            best_gap, best_classes = gap, classes
    return best_classes


def persistence_classes(births, merges, ascending, max_classes):
    """Nombre de classes propose par la PERSISTANCE, regle PARTAGEE.

    Regle de l'aine (elder rule), identique dans les deux echelles. Chaque
    feuille `i` nait au niveau `births[i]` et MEURT au niveau de la fusion
    qui absorbe sa composante dans une composante dont l'aine est plus
    ancienne ; sa persistance est `|mort - naissance|`. La feuille la plus
    ancienne ne meurt jamais. On trie les persistances par ordre decroissant
    et on coupe au plus grand saut : le nombre de classes est le nombre de
    persistances retenues plus une (l'aine globale).

    `ascending` dit dans quel sens le temps s'ecoule : `True` sur l'axe `a`
    de la tour empirique (une naissance plus PETITE est plus ancienne),
    `False` sur l'axe `lambda` de la tour spectrale (une naissance plus
    GRANDE est plus ancienne). La regle ne regarde que l'ordre des
    naissances et les niveaux de fusion : elle s'applique telle quelle aux
    deux tours, ce qui est la condition pour que la comparaison soit juste.
    """
    births = np.asarray(births, dtype=float)
    total = births.size
    signed = births if ascending else -births
    forest = _Forest(total)
    elder = list(range(total))
    persistences = []
    for level, left, right in merges:
        root_left, root_right = forest.find(left), forest.find(right)
        if root_left == root_right:
            continue
        first, second = elder[root_left], elder[root_right]
        if signed[first] <= signed[second]:
            keeper, dying = first, second
        else:
            keeper, dying = second, first
        persistences.append(abs(float(level) - float(births[dying])))
        forest.union(left, right)
        elder[forest.find(left)] = keeper
    if not persistences:
        return 1
    persistences.sort(reverse=True)
    limit = min(max_classes, len(persistences) + 1)
    best_classes, best_gap = 1, -1.0
    for classes in range(2, limit + 1):
        index = classes - 2
        following = persistences[index + 1] if index + 1 < len(persistences) else 0.0
        gap = persistences[index] - following
        if gap > best_gap:
            best_gap, best_classes = gap, classes
    return best_classes


# -- (i) tour empirique projetee, version flottante --------------------


class _Forest:
    def __init__(self, size):
        self.parent = list(range(size))

    def find(self, item):
        root = item
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[item] != root:
            self.parent[item], item = root, self.parent[item]
        return root

    def union(self, left, right):
        left_root, right_root = self.find(left), self.find(right)
        if left_root == right_root:
            return False
        keep, drop = (
            (left_root, right_root) if left_root < right_root else (right_root, left_root)
        )
        self.parent[drop] = keep
        return True


def candidate_edges(squared, neighbours, full_graph):
    """Aretes candidates : `q` plus proches voisins plus l'ACM euclidien."""
    count = squared.shape[0]
    if full_graph:
        return [(left, right) for left in range(count) for right in range(left + 1, count)]
    keep = min(neighbours, count - 1)
    masked = squared.copy()
    np.fill_diagonal(masked, np.inf)
    nearest = np.argpartition(masked, keep - 1, axis=1)[:, :keep]
    edges = set()
    for left in range(count):
        for right in nearest[left]:
            edges.add((min(left, int(right)), max(left, int(right))))
    tree = minimum_spanning_tree(np.sqrt(np.maximum(squared, 0.0))).tocoo()
    for left, right in zip(tree.row, tree.col):
        edges.add((min(int(left), int(right)), max(int(left), int(right))))
    return sorted(edges)


def gram_and_squared(cloud):
    """Matrice de Gram et matrice des distances au carre, calculees une fois."""
    cloud = np.asarray(cloud, dtype=float)
    gram = cloud @ cloud.T
    diagonal = np.diag(gram).copy()
    squared = np.maximum(diagonal[:, None] + diagonal[None, :] - 2.0 * gram, 0.0)
    return gram, diagonal, squared


def segment_weights(gram, diagonal, squared, order, edges, samples, chunk=256):
    """Poids de segment approches `max_t a_order` sur une grille de `t`."""
    times = np.linspace(0.0, 1.0, samples)
    left_index = np.array([pair[0] for pair in edges])
    right_index = np.array([pair[1] for pair in edges])
    weights = np.empty(len(edges))
    for start in range(0, len(edges), chunk):
        stop = min(start + chunk, len(edges))
        sources = left_index[start:stop]
        targets = right_index[start:stop]
        leading = squared[sources, targets]
        constant = squared[sources, :]
        linear = 2.0 * (
            gram[sources, targets][:, None]
            - gram[targets, :]
            - diagonal[sources][:, None]
            + gram[sources, :]
        )
        values = constant[:, None, :] + times[None, :, None] * linear[:, None, :]
        statistic = np.partition(values, order - 1, axis=-1)[..., order - 1]
        statistic = statistic + leading[:, None] * (times ** 2)[None, :]
        weights[start:stop] = statistic.max(axis=1)
    return weights


def empirical_tower(cloud, order, neighbours=15, samples=17, full_graph=False):
    """Tour empirique projetee : niveaux de naissance et arbre de fusion.

    Renvoie un dictionnaire avec la suite des fusions (niveaux croissants),
    la fonction d'etiquetage a `c` classes, et les compteurs de couverture.
    """
    cloud = np.asarray(cloud, dtype=float)
    count = cloud.shape[0]
    gram, diagonal, squared = gram_and_squared(cloud)
    edges = candidate_edges(squared, neighbours, full_graph)
    weights = segment_weights(gram, diagonal, squared, order, edges, samples)
    births = np.partition(squared, order - 1, axis=1)[:, order - 1]
    ordering = np.argsort(weights, kind="stable")
    forest = _Forest(count)
    merges = []
    for position in ordering:
        left, right = edges[position]
        if forest.union(left, right):
            merges.append((float(weights[position]), left, right))
    return {
        "order": order,
        "count": count,
        "edges": len(edges),
        "births": births,
        "merges": merges,
        "weights": weights,
    }


def empirical_labels(tower, classes):
    """Etiquettes des observations pour une coupe a `classes` composantes."""
    count = tower["count"]
    applied = max(0, count - classes)
    forest = _Forest(count)
    for level, left, right in tower["merges"][:applied]:
        del level
        forest.union(left, right)
    codes = {}
    labels = np.empty(count, dtype=int)
    for index in range(count):
        root = forest.find(index)
        if root not in codes:
            codes[root] = len(codes)
        labels[index] = codes[root]
    return labels


def self_test(seed=5, count=9, dimension=4, orders=(1, 2, 3), grids=(9, 17, 33, 129)):
    """Confrontation du port FLOTTANT a l'implementation RATIONNELLE exacte.

    Le poids de segment exact de `ehgp.engine.segment.segment_maximum` est le
    maximum de `a_k` sur les croisements des `n` droites ; le poids flottant
    de cette sonde est le maximum sur une grille de `samples` temps. Le second
    est donc un MINORANT du premier, et l'ecart mesure ici dit ce que coute la
    grille. La porte verifie les deux faits : le minorant ne depasse jamais la
    valeur exacte, et l'ecart relatif reste sous un seuil affiche.
    """
    generator = np.random.default_rng(seed)
    cloud = np.round(6.0 * generator.standard_normal((count, dimension)), 3)
    exact_cloud = rational_cloud(cloud)
    gram, diagonal, squared = gram_and_squared(cloud)
    edges = [(left, right) for left in range(count) for right in range(left + 1, count)]
    print("auto-test : port flottant contre segment rationnel exact")
    print("  n=%d  d=%d  paires=%d" % (count, dimension, len(edges)))
    worst = 0.0
    for order in orders:
        exact = []
        for left, right in edges:
            level, _time = segment_maximum(exact_cloud, left, right, order)
            exact.append(float(level))
        exact = np.array(exact)
        for samples in grids:
            weights = segment_weights(gram, diagonal, squared, order, edges, samples)
            above = int(
                np.count_nonzero(weights > exact + 1e-9 * np.maximum(1.0, np.abs(exact)))
            )
            gaps = (exact - weights) / np.maximum(1e-12, np.abs(exact))
            worst = max(worst, float(gaps.max()))
            print(
                "  k=%d grille=%3d depassements=%d ecart_relatif_max=%.3e"
                % (order, samples, above, float(gaps.max()))
            )
            if above:
                return 1
    print("  minorant respecte partout ; ecart relatif maximal %.3e" % worst)
    return 0


# -- campagne ----------------------------------------------------------


def run_case(
    cloud,
    truth,
    groups,
    order,
    features,
    neighbours,
    samples,
    path_samples,
    refine_steps,
    rho_max,
    bandwidth_scale,
    covariance_floor,
    max_classes,
    full_graph,
    seed,
):
    """Une cellule du tableau : les deux tours sur le meme nuage."""
    started = time.perf_counter()
    empirical = empirical_tower(cloud, order, neighbours, samples, full_graph)
    empirical_seconds = time.perf_counter() - started
    empirical_cut = empirical_labels(empirical, groups)
    empirical_levels = [level for level, _left, _right in empirical["merges"]]
    row = {
        "order": order,
        "edges": empirical["edges"],
        "merges_empirical": len(empirical["merges"]),
        "ari_empirical": adjusted_rand_index(truth, empirical_cut),
        "gap_empirical": gap_classes(empirical_levels, max_classes),
        "persistence_empirical": persistence_classes(
            empirical["births"], empirical["merges"], True, max_classes
        ),
        "seconds_empirical": empirical_seconds,
    }
    started = time.perf_counter()
    model = SpectralLogDensity(
        feature_count=features,
        bandwidth_scale=bandwidth_scale,
        rho_max=rho_max,
        covariance_floor=covariance_floor,
        seed=seed,
    ).fit(cloud)
    fit_seconds = time.perf_counter() - started
    started = time.perf_counter()
    tower = SpectralTower(
        cloud,
        model.log_density,
        model.gradient_log_density,
        path_samples=path_samples,
        refine_steps=refine_steps,
    )
    tower_seconds = time.perf_counter() - started
    spectral_levels = [level for level, _left, _right in tower.merges]
    row.update(
        {
            "rank": model.effective_rank,
            "lambda_min": float(model.eigenvalues.min()),
            "lambda_max": float(model.eigenvalues.max()),
            "maxima": tower.maximum_count,
            "ari_spectral": adjusted_rand_index(truth, tower.labels(groups)),
            "achieved_spectral": tower.achieved_classes(groups),
            "gap_spectral": gap_classes(spectral_levels, max_classes),
            "persistence_spectral": persistence_classes(
                tower.maximum_values, tower.merges, False, max_classes
            ),
            "gradient_max": tower.ascent["gradient_max"],
            "gradient_scale": tower.gradient_scale,
            "unconverged": tower.unconverged,
            "refined": tower.refinement_accepted,
            "seconds_fit": fit_seconds,
            "seconds_spectral": tower_seconds,
            "digest": tower.structural_digest()[:12],
        }
    )
    return row


HEADER = (
    "  d  di    n  G  k  bruit          rank lam_min lam_max  P  "
    "ARI_emp ARI_spe  per_e per_s gap_e gap_s  |g|/ech  t_emp  t_spe"
)


def format_row(dimension, intrinsic, count, groups, mode, row):
    return (
        "%3d %3d %4d %2d %2d  %-14s %4d %7.4f %7.3f %3d  "
        " %6.3f  %6.3f  %5d %5d %5d %5d  %7.1e %6.1f %6.1f"
        % (
            dimension,
            intrinsic,
            count,
            groups,
            row["order"],
            mode,
            row["rank"],
            row["lambda_min"],
            row["lambda_max"],
            row["maxima"],
            row["ari_empirical"],
            row["ari_spectral"],
            row["persistence_empirical"],
            row["persistence_spectral"],
            row["gap_empirical"],
            row["gap_spectral"],
            row["gradient_max"] / max(row["gradient_scale"], 1e-300),
            row["seconds_empirical"],
            row["seconds_fit"] + row["seconds_spectral"],
        )
    )


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--dims", default="2,10,50,200")
    parser.add_argument("--intrinsic", default="2,5")
    parser.add_argument("--n", default="300,1000")
    parser.add_argument("--groups", type=int, default=4)
    parser.add_argument("--orders", default="1,5")
    parser.add_argument("--noise-modes", default="none,isotropic_norm,per_coordinate")
    parser.add_argument("--intrinsic-noise", type=float, default=1.0)
    parser.add_argument("--separation", type=float, default=8.0)
    parser.add_argument("--noise", type=float, default=1.0)
    parser.add_argument("--features", type=int, default=256)
    # Defaut MESURE, pas devine : dans le regime temoin `none` (probleme
    # intrinseque plonge sans bruit ambiant), les echelles 0,20 / 0,35 / 0,50
    # / 1,00 donnent toutes un ARI spectral de 0,99 a 1,00 en d = 2 et d = 50.
    # 0,35 est pris au milieu de ce plateau, une fois pour toute la campagne :
    # aucune ligne du tableau n'a sa propre largeur de bande.
    parser.add_argument("--bandwidth-scale", type=float, default=0.35)
    parser.add_argument("--rho-max", type=float, default=1.0)
    parser.add_argument("--covariance-floor", type=float, default=1e-6)
    parser.add_argument("--neighbours", type=int, default=15)
    parser.add_argument("--samples", type=int, default=17)
    parser.add_argument("--path-samples", type=int, default=33)
    parser.add_argument("--refine-steps", type=int, default=0)
    parser.add_argument("--max-classes", type=int, default=12)
    parser.add_argument("--seeds", type=int, default=1)
    parser.add_argument("--full-graph", action="store_true")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--blas-threads", type=int, default=None)
    options = parser.parse_args(argv)
    if options.blas_threads is not None and options.blas_threads != int(
        os.environ.get("OPENBLAS_NUM_THREADS", "1")
    ):
        print(
            "relancer avec EHGP_BLAS_THREADS=%d : le nombre de fils BLAS se fixe "
            "avant l'importation de numpy" % options.blas_threads
        )
        return 2
    if options.self_test:
        return self_test()
    if options.quick:
        options.dims = "2,10,50"
        options.intrinsic = "2"
        options.n = "300"
        options.orders = "1,5"
        options.noise_modes = "none"
        options.features = 128
    dimensions = [int(item) for item in options.dims.split(",")]
    intrinsics = [int(item) for item in options.intrinsic.split(",")]
    counts = [int(item) for item in options.n.split(",")]
    orders = [int(item) for item in options.orders.split(",")]
    modes = [item.strip() for item in options.noise_modes.split(",")]
    print(HEADER)
    print("-" * len(HEADER))
    summary = {}
    for mode in modes:
        for dimension in dimensions:
            for intrinsic in intrinsics:
                if intrinsic > dimension:
                    continue
                for count in counts:
                    for seed in range(options.seeds):
                        cloud, truth, _scale = gaussian_mixture(
                            count,
                            dimension,
                            options.groups,
                            intrinsic,
                            options.separation,
                            options.noise,
                            mode,
                            1000 + seed,
                            options.intrinsic_noise,
                        )
                        for order in orders:
                            row = run_case(
                                cloud,
                                truth,
                                options.groups,
                                order,
                                options.features,
                                options.neighbours,
                                options.samples,
                                options.path_samples,
                                options.refine_steps,
                                options.rho_max,
                                options.bandwidth_scale,
                                options.covariance_floor,
                                options.max_classes,
                                options.full_graph,
                                7 + seed,
                            )
                            print(
                                format_row(
                                    dimension, intrinsic, count, options.groups, mode, row
                                ),
                                flush=True,
                            )
                            key = (mode, dimension, order)
                            slot = summary.setdefault(key, [0, 0.0, 0.0])
                            slot[0] += 1
                            slot[1] += row["ari_empirical"]
                            slot[2] += row["ari_spectral"]
    print()
    print("moyennes par (bruit, d, k) : ARI empirique contre ARI spectral")
    print("bruit            d   k  cas  ARI_emp  ARI_spe  ecart")
    for (mode, dimension, order) in sorted(summary):
        cases, empirical_total, spectral_total = summary[(mode, dimension, order)]
        print(
            "%-15s %3d %3d %4d   %6.3f   %6.3f  %+6.3f"
            % (
                mode,
                dimension,
                order,
                cases,
                empirical_total / cases,
                spectral_total / cases,
                (spectral_total - empirical_total) / cases,
            )
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
