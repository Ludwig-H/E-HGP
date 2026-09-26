"""Banc de comparaison : la tour E-HGP projetee vaut-elle comme clusterer ?

QUESTION TRANCHEE ICI. L'axe d'ORDRE `k` de la tour apporte-t-il quelque
chose de mesurable par rapport a `k = 1` (qui est exactement la liaison
simple euclidienne, cf. fait 1 du chantier) en grande dimension ? Et la tour
E-HGP se distingue-t-elle de la reachability mutuelle facon HDBSCAN au meme
ordre ? Si `k > 1` n'apporte rien, c'est un resultat et il doit etre ecrit.

CE QUI EST COMPARE. Trois objets partagent exactement la meme machinerie
d'extraction (`LinkageTower`), donc leurs differences ne viennent QUE de la
regle de liaison :

* TOUR E-HGP a l'ordre `k` : poids `w_ij = max_t a_k(y(t))` sur le SEGMENT
  `[x_i, x_j]`, majorant certifie calcule par
  `ehgp.engine.fast_point_tower` ; naissance de `x_i` au niveau `a_k(x_i)`.
* REACHABILITY MUTUELLE a l'ordre `k` : poids
  `max(a_k(x_i), a_k(x_j), D_ij / 4)`, memes naissances. Ce n'est PAS le
  meme objet : elle ne regarde que les extremites, jamais ce que le segment
  traverse.
* LIAISON SIMPLE euclidienne : poids `D_ij / 4`, naissances nulles. C'est le
  cas `k = 1` des deux precedents.

Temoins non densitaires : liaison de Ward et k-moyennes. Temoins de la
bibliotheque : DBSCAN et HDBSCAN de scikit-learn quand il est installe.
DBSCAN est regle par ORACLE (on lui donne le meilleur `eps` d'une grille au
vu de la verite terrain) : c'est donc un MAJORANT de ce qu'il ferait en
usage reel, et il est annonce comme tel.

DEUX COUPES, pour ne pas confondre qualite d'arbre et qualite de selection :

* coupe au VRAI nombre de classes : tous les points etiquetes, aucun rejet
  de bruit ; elle juge l'arbre seul ;
* coupe SPONTANEE : meme regle pour toutes les methodes dendrogrammes (plus
  grand saut de niveau de fusion, entre 2 et 25 groupes), plus le rejet de
  bruit propre a la tour (une observation non encore nee, `a_k(x_i) > a`,
  n'est dans aucune composante) et l'elimination des groupes de moins de
  `min_size` observations.

MESURES. Indice de Rand ajuste et information mutuelle normalisee (calcules
ici, sans dependance a scikit-learn, et confrontes a scikit-learn dans la
porte), part du bruit correctement rejetee, part des vrais points de classe
conserves, nombre de groupes rendus, temps mur. Cinq graines au moins,
moyennes et ecarts-types.

DIAGNOSTIC DE L'ENCADREMENT, publie avec chaque cellule : part des couples
dont le maximum de segment est PROUVE exact (`upper == lower`), ecart relatif
moyen, et accord entre la partition obtenue avec le majorant et celle
obtenue avec le minorant. Un banc dont l'encadrement est lache et dont les
deux partitions divergent ne mesure pas la tour : il mesure son majorant.

Usage :

    python3 bench/clustering_compare.py gate
    python3 bench/clustering_compare.py run --profile quick
    python3 bench/clustering_compare.py run --profile full --out receipts/x.jsonl
    python3 bench/clustering_compare.py table --input receipts/x.jsonl

Aucune porte ne repose sur le mot-cle `assert` : seules les methodes
`self.assert*` de `unittest` decident, donc tout tient sous `python3 -O`.
"""

import argparse
import json
import sys
import time
import unittest
from pathlib import Path

import numpy as np

SOURCE_ROOT = str(Path(__file__).resolve().parent.parent / "src")
if SOURCE_ROOT not in sys.path:
    sys.path.insert(0, SOURCE_ROOT)

from ehgp.engine.fast_point_tower import (  # noqa: E402
    FastPointTower,
    LinkageTower,
    certify_against_exact,
    mutual_reachability,
    segment_brackets,
    squared_distances,
    upper_pairs,
)

try:
    import sklearn  # noqa: F401
    from sklearn.cluster import DBSCAN, HDBSCAN, KMeans
    from sklearn.metrics import adjusted_rand_score, normalized_mutual_info_score

    HAS_SKLEARN = True
except ImportError:  # pragma: no cover - dependance optionnelle
    HAS_SKLEARN = False

try:
    from scipy.cluster.hierarchy import fcluster, linkage
    from scipy.spatial.distance import pdist

    HAS_SCIPY = True
except ImportError:  # pragma: no cover - dependance optionnelle
    HAS_SCIPY = False


# ----------------------------------------------------------------------------
# Mesures d'accord, ecrites ici pour ne dependre d'aucune bibliotheque.
# ----------------------------------------------------------------------------


def contingency(truth, labels):
    """Table de contingence entiere des deux etiquetages, bruit compris."""
    truth = np.asarray(truth)
    labels = np.asarray(labels)
    left_values, left_index = np.unique(truth, return_inverse=True)
    right_values, right_index = np.unique(labels, return_inverse=True)
    table = np.zeros((left_values.size, right_values.size), dtype=np.int64)
    np.add.at(table, (left_index, right_index), 1)
    return table


def adjusted_rand_index(truth, labels):
    """Indice de Rand ajuste, en arithmetique entiere puis un seul quotient.

    Le bruit est une etiquette comme une autre (convention usuelle de la
    litterature densitaire) : une methode qui rejette a tort est punie, une
    methode qui ne rejette jamais ne peut pas gagner sur le bruit.
    """
    table = contingency(truth, labels)
    total = int(table.sum())
    if total < 2:
        return 1.0
    pairs = int((table * (table - 1) // 2).sum())
    rows = table.sum(axis=1)
    columns = table.sum(axis=0)
    row_pairs = int((rows * (rows - 1) // 2).sum())
    column_pairs = int((columns * (columns - 1) // 2).sum())
    all_pairs = total * (total - 1) // 2
    expected = row_pairs * column_pairs / all_pairs
    maximum = 0.5 * (row_pairs + column_pairs)
    if maximum == expected:
        return 1.0
    return float((pairs - expected) / (maximum - expected))


def normalized_mutual_information(truth, labels):
    """Information mutuelle normalisee par la moyenne arithmetique des entropies."""
    table = contingency(truth, labels).astype(np.float64)
    total = table.sum()
    if total <= 0:
        return 0.0
    joint = table / total
    rows = joint.sum(axis=1)
    columns = joint.sum(axis=0)
    positive = joint > 0.0
    outer = rows[:, None] * columns[None, :]
    mutual = float(np.sum(joint[positive] * np.log(joint[positive] / outer[positive])))
    entropy_rows = -float(np.sum(rows[rows > 0.0] * np.log(rows[rows > 0.0])))
    entropy_columns = -float(
        np.sum(columns[columns > 0.0] * np.log(columns[columns > 0.0]))
    )
    if entropy_rows <= 0.0 and entropy_columns <= 0.0:
        return 1.0
    return float(2.0 * mutual / (entropy_rows + entropy_columns))


def score_labels(truth, labels):
    """Toutes les mesures d'une cellule pour un etiquetage donne."""
    truth = np.asarray(truth)
    labels = np.asarray(labels)
    noise_truth = truth < 0
    found_noise = labels < 0
    groups = int(np.unique(labels[labels >= 0]).size)
    noise_recall = (
        float(np.mean(found_noise[noise_truth])) if int(noise_truth.sum()) else 1.0
    )
    kept = (
        float(np.mean(~found_noise[~noise_truth])) if int((~noise_truth).sum()) else 1.0
    )
    return {
        "ari": adjusted_rand_index(truth, labels),
        "nmi": normalized_mutual_information(truth, labels),
        "groups": groups,
        "noise_recall": noise_recall,
        "cluster_kept": kept,
    }


# ----------------------------------------------------------------------------
# Jeux synthetiques a verite terrain.
# ----------------------------------------------------------------------------


def random_isometry(rng, dim, rank):
    """Application lineaire orthonormee de `R^rank` dans `R^dim`."""
    reach = min(rank, dim)
    matrix = rng.normal(size=(dim, reach))
    basis, _ = np.linalg.qr(matrix)
    return basis[:, :reach]


def place_centres(rng, clusters, dim, gap):
    """Centres dont la plus petite distance mutuelle vaut EXACTEMENT `gap`.

    Les centres sont tires sur la sphere unite puis dilates jusqu'a ce que
    leur ecart minimal atteigne `gap`. Sans cette normalisation, deux centres
    tires au hasard peuvent se toucher et la cellule mesure alors une tache
    impossible au lieu de mesurer une methode.
    """
    if clusters == 1:
        return np.zeros((1, dim))
    centres = rng.normal(size=(clusters, dim))
    centres = centres / np.linalg.norm(centres, axis=1, keepdims=True)
    separation = squared_distances(centres)
    np.fill_diagonal(separation, np.inf)
    smallest = float(np.sqrt(separation.min()))
    return centres * (gap / max(smallest, 1e-12))


def split_sizes(rng, total, parts, ratio=1.0):
    """Partage `total` en `parts` tailles de rapport maximal `ratio`."""
    weights = np.exp(rng.uniform(0.0, np.log(max(ratio, 1.0)), size=parts))
    weights = weights / weights.sum()
    sizes = np.maximum(3, np.floor(weights * total).astype(int))
    while int(sizes.sum()) > total:
        sizes[int(np.argmax(sizes))] -= 1
    while int(sizes.sum()) < total:
        sizes[int(np.argmin(sizes))] += 1
    return sizes


def make_dataset(family, rng, count, dim, noise_share, clusters=None):
    """Nuage `(points, verite)` avec `-1` pour le bruit de fond.

    Les quatre familles, et ce que chacune met a l'epreuve :

    * `gauss_iso` : melange gaussien isotrope, separation des centres
      proportionnelle a `sqrt(d)`, donc difficulte a peu pres constante en
      dimension. C'est la reference.
    * `gauss_aniso` : tailles dans un rapport 6, echelles dans un rapport 3,
      une direction allongee d'un facteur 6. Teste les amas de DENSITES
      DIFFERENTES, la ou une coupe a seuil unique echoue.
    * `manifold` : amas portes par des varietes de dimension intrinseque 2
      ou 5, plongees par une isometrie aleatoire, PLUS un bruit ambiant
      isotrope. Le plongement seul ne change rien (une isometrie preserve
      toutes les distances) : c'est le bruit ambiant, de norme croissante en
      `sqrt(d)`, qui fait mordre la dimension. Sans lui la colonne `d` serait
      une illusion, et c'est dit ici parce que beaucoup de bancs l'oublient.
    * `filament` : amas FILIFORMES, longs et minces, directions aleatoires.
      C'est le regime ou une hierarchie de densite retarde la naissance des
      objets fins, donc le plus defavorable a l'axe d'ordre.
    """
    if clusters is None:
        clusters = int(rng.integers(3, 11))
    noise_count = int(round(count * noise_share))
    signal = count - noise_count
    sizes = split_sizes(rng, signal, clusters, 6.0 if family == "gauss_aniso" else 1.0)
    blocks = []
    truth = []
    if family in ("gauss_iso", "gauss_aniso"):
        # Un ecart-type par coordonnee : le rayon d'un amas croit en sqrt(d),
        # donc l'ecart des centres doit en tenir compte, sinon la colonne d
        # mesure seulement un recouvrement total.
        centres = place_centres(rng, clusters, dim, max(8.0, 1.4 * np.sqrt(dim)))
        for index in range(clusters):
            size = int(sizes[index])
            cloud = rng.normal(size=(size, dim))
            if family == "gauss_aniso":
                scale = float(np.exp(rng.uniform(0.0, np.log(3.0))))
                axis = random_isometry(rng, dim, 1)[:, 0]
                cloud = cloud * scale
                cloud = cloud + 3.0 * np.outer(rng.normal(size=size), axis)
            blocks.append(centres[index] + cloud)
            truth.extend([index] * size)
    elif family == "manifold":
        intrinsic = 2 if dim < 10 else 5
        ambient = 0.2
        centres = place_centres(rng, clusters, dim, 16.0)
        for index in range(clusters):
            size = int(sizes[index])
            basis = random_isometry(rng, dim, intrinsic)
            local = rng.uniform(-1.0, 1.0, size=(size, basis.shape[1]))
            local = local * np.array([6.0] + [2.0] * (basis.shape[1] - 1))
            curved = np.sin(local[:, :1]) * 2.0
            local = np.concatenate([local[:, :1], local[:, 1:] + curved], axis=1)
            # Le plongement est une isometrie : il ne change RIEN aux
            # distances. Seul le bruit ambiant, de norme croissante en
            # sqrt(d), fait mordre la dimension.
            blocks.append(
                centres[index] + local @ basis.T + ambient * rng.normal(size=(size, dim))
            )
            truth.extend([index] * size)
    elif family == "filament":
        centres = place_centres(rng, clusters, dim, 24.0)
        for index in range(clusters):
            size = int(sizes[index])
            axis = random_isometry(rng, dim, 1)[:, 0]
            along = rng.uniform(-1.0, 1.0, size=size) * 10.0
            # Epaisseur de norme constante en dimension : un filament doit
            # rester filiforme, sinon la famille se dissout en boules.
            thickness = 0.5 * rng.normal(size=(size, dim)) / np.sqrt(dim)
            blocks.append(centres[index] + np.outer(along, axis) + thickness)
            truth.extend([index] * size)
    else:
        raise ValueError("famille inconnue : " + str(family))
    points = np.concatenate(blocks, axis=0)
    if noise_count:
        low = points.min(axis=0)
        high = points.max(axis=0)
        background = rng.uniform(low, high, size=(noise_count, dim))
        points = np.concatenate([points, background], axis=0)
        truth.extend([-1] * noise_count)
    truth = np.asarray(truth, dtype=np.int64)
    shuffle = rng.permutation(points.shape[0])
    return points[shuffle], truth[shuffle], clusters


FAMILIES = ("gauss_iso", "gauss_aniso", "manifold", "filament")


# ----------------------------------------------------------------------------
# Methodes comparees.
# ----------------------------------------------------------------------------


def single_linkage_tower(distances, count):
    """Liaison simple euclidienne : poids `D_ij / 4`, aucune naissance."""
    rows, columns = upper_pairs(count)
    return LinkageTower(
        distances[rows, columns] / 4.0, np.zeros(count), count
    )


def dendrogram_labels(tower, clusters, min_size, max_clusters=25):
    """Les trois coupes d'un arbre de liaison.

    `fixed` : au vrai nombre de classes, tous les points etiquetes.
    `auto` : plus grand saut de niveau de fusion, regle commune a tous les
    arbres du banc, plus le rejet des non-nes et des groupes trop petits.
    `peak` : niveau ou le nombre de groupes recevables est maximal, seule
    regle du banc qui utilise vraiment l'axe des naissances.
    """
    level, _groups = tower.spontaneous_level(max_clusters=max_clusters)
    summit, _peak = tower.peak_level(min_size=min_size, max_clusters=max_clusters)
    return (
        tower.labels_fixed(clusters),
        tower.labels_at(level, min_size=min_size),
        tower.labels_at(summit, min_size=min_size),
    )


def ward_labels(points, clusters):
    """Liaison de Ward, temoin non densitaire."""
    if not HAS_SCIPY:
        return None
    tree = linkage(pdist(points), method="ward")
    return np.asarray(fcluster(tree, t=clusters, criterion="maxclust"), dtype=np.int64)


def kmeans_labels(points, clusters, seed):
    """k-moyennes, temoin non densitaire."""
    if HAS_SKLEARN:
        model = KMeans(n_clusters=clusters, n_init=10, random_state=seed)
        return np.asarray(model.fit_predict(points), dtype=np.int64)
    return None


def dbscan_labels(points, order, clusters, truth):
    """DBSCAN regle par ORACLE sur une grille de `eps`.

    Deux sorties : celle dont le nombre de groupes est le plus proche du vrai
    nombre de classes, et celle qui maximise l'indice de Rand ajuste. La
    seconde est un MAJORANT de ce que DBSCAN ferait sans connaitre la verite.
    """
    if not HAS_SKLEARN:
        return None, None
    distances = squared_distances(points)
    core = np.sqrt(np.sort(distances, axis=1)[:, min(order, points.shape[0]) - 1])
    grid = np.quantile(core[core > 0.0], np.linspace(0.05, 0.95, 12))
    grid = np.unique(np.concatenate([grid, grid * 1.5, grid * 0.75]))
    best_close = None
    best_score = None
    for eps in grid:
        labels = DBSCAN(eps=float(eps), min_samples=order).fit_predict(points)
        labels = np.asarray(labels, dtype=np.int64)
        groups = int(np.unique(labels[labels >= 0]).size)
        value = adjusted_rand_index(truth, labels)
        gap = abs(groups - clusters)
        if best_close is None or (gap, -value) < best_close[0]:
            best_close = ((gap, -value), labels)
        if best_score is None or value > best_score[0]:
            best_score = (value, labels)
    return best_close[1], best_score[1]


def hdbscan_labels(points, order, min_size):
    """HDBSCAN de scikit-learn, reglage par defaut sauf `min_cluster_size`."""
    if not HAS_SKLEARN:
        return None
    model = HDBSCAN(min_cluster_size=min_size, min_samples=order, copy=True)
    return np.asarray(model.fit_predict(points), dtype=np.int64)


# ----------------------------------------------------------------------------
# Une cellule du banc.
# ----------------------------------------------------------------------------


def run_cell(family, dim, noise_share, seed, count, orders, intervals, mode, min_size):
    """Execute toutes les methodes sur un jeu, renvoie une ligne de mesures."""
    rng = np.random.default_rng(seed * 1000003 + dim * 101 + hash(family) % 997)
    points, truth, clusters = make_dataset(family, rng, count, dim, noise_share)
    row = {
        "family": family,
        "dim": dim,
        "noise_share": noise_share,
        "seed": seed,
        "count": int(points.shape[0]),
        "clusters": clusters,
        "orders": list(orders),
        "intervals": intervals,
        "mode": mode,
        "min_size": min_size,
        "methods": {},
    }
    start = time.perf_counter()
    tower = FastPointTower(
        points, orders=orders, intervals=intervals, mode=mode, certify=(mode == "tube")
    )
    tower_wall = time.perf_counter() - start
    row["tower_wall"] = tower_wall
    row["bracket"] = {
        "exact_fraction": list(tower.bracket["exact_fraction"]),
        "gap_mean": list(tower.bracket["gap_mean"]),
        "gap_max": list(tower.bracket["gap_max"]),
        "tube_fraction": list(tower.bracket["tube_fraction"]),
        "order_violation": tower.bracket["order_violation"],
        "width": tower.bracket["width"],
    }
    row["digest"] = tower.digest()

    def record(name, fixed, auto, peak, wall, note=""):
        entry = {"wall": wall, "note": note}
        for key, labels in (("fixed", fixed), ("auto", auto), ("peak", peak)):
            if labels is not None:
                entry[key] = score_labels(truth, labels)
        row["methods"][name] = entry
        return entry

    for order in tower.orders:
        start = time.perf_counter()
        upper = tower.tower(order, "upper")
        fixed, auto, peak = dendrogram_labels(upper, clusters, min_size)
        wall = time.perf_counter() - start
        start = time.perf_counter()
        lower = tower.tower(order, "lower")
        fixed_low, auto_low, peak_low = dendrogram_labels(lower, clusters, min_size)
        wall_low = time.perf_counter() - start
        agreement = adjusted_rand_index(fixed, fixed_low)
        entry = record(
            "ehgp_k%d" % order,
            fixed,
            auto,
            peak,
            tower_wall + wall,
            note="accord majorant/minorant ari=%.4f" % agreement,
        )
        entry["bracket_agreement"] = agreement
        # La MEME tour lue du cote minorant. Elle n'est pas un objet publiable
        # (le minorant n'est pas un poids de fusion), mais elle borne l'autre
        # bout de l'encadrement : si les deux bouts donnent le meme verdict,
        # le verdict ne depend pas de la largeur de l'encadrement.
        record(
            "ehgp_min_k%d" % order,
            fixed_low,
            auto_low,
            peak_low,
            tower_wall + wall_low,
            note="lecture du MINORANT de l'encadrement, sensibilite seulement",
        )
        start = time.perf_counter()
        reach = tower.reachability_tower(order, quarter=True)
        fixed, auto, peak = dendrogram_labels(reach, clusters, min_size)
        record("reach_k%d" % order, fixed, auto, peak, time.perf_counter() - start)
    start = time.perf_counter()
    simple = single_linkage_tower(tower.distances, tower.count)
    fixed, auto, peak = dendrogram_labels(simple, clusters, min_size)
    record("single_linkage", fixed, auto, peak, time.perf_counter() - start)
    start = time.perf_counter()
    fixed = ward_labels(points, clusters)
    record("ward", fixed, None, None, time.perf_counter() - start)
    start = time.perf_counter()
    fixed = kmeans_labels(points, clusters, seed)
    record("kmeans", fixed, None, None, time.perf_counter() - start)
    reference_order = max(order for order in tower.orders if order <= 5)
    start = time.perf_counter()
    close, best = dbscan_labels(points, reference_order, clusters, truth)
    record(
        "dbscan_oracle_k%d" % reference_order,
        close,
        best,
        best,
        time.perf_counter() - start,
        note="fixed = eps le plus proche du vrai nombre, auto et peak = meilleur ari (ORACLE)",
    )
    start = time.perf_counter()
    labels = hdbscan_labels(points, reference_order, min_size)
    record(
        "hdbscan_k%d" % reference_order,
        None,
        labels,
        labels,
        time.perf_counter() - start,
        note="regle spontanee propre a HDBSCAN (exces de masse)",
    )
    return row


PROFILES = {
    "quick": {
        "count": 120,
        "dims": (2, 50, 1000),
        "noise": (0.0, 0.3),
        "seeds": (1, 2),
        "families": FAMILIES,
        "intervals": 8,
        "mode": "full",
    },
    "full": {
        "count": 200,
        "dims": (2, 10, 50, 200, 1000),
        "noise": (0.0, 0.3),
        "seeds": (1, 2, 3, 4, 5),
        "families": FAMILIES,
        "intervals": 8,
        "mode": "full",
    },
    "noise": {
        "count": 200,
        "dims": (10, 200),
        "noise": (0.0, 0.1, 0.3),
        "seeds": (1, 2, 3, 4, 5),
        "families": FAMILIES,
        "intervals": 8,
        "mode": "full",
    },
}


def run_campaign(profile, orders, output, limit=None, min_size=5):
    """Balaye la grille du profil, ecrit une ligne JSON par cellule."""
    plan = PROFILES[profile]
    rows = []
    handle = open(output, "w", encoding="ascii") if output else None
    done = 0
    for family in plan["families"]:
        for dim in plan["dims"]:
            for noise_share in plan["noise"]:
                for seed in plan["seeds"]:
                    if limit is not None and done >= limit:
                        break
                    start = time.perf_counter()
                    row = run_cell(
                        family,
                        dim,
                        noise_share,
                        seed,
                        plan["count"],
                        orders,
                        plan["intervals"],
                        plan["mode"],
                        min_size,
                    )
                    row["cell_wall"] = time.perf_counter() - start
                    rows.append(row)
                    done += 1
                    if handle is not None:
                        handle.write(
                            json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n"
                        )
                        handle.flush()
                    sys.stderr.write(
                        "cellule %3d %-12s d=%4d bruit=%.1f graine=%d %.1fs\n"
                        % (done, family, dim, noise_share, seed, row["cell_wall"])
                    )
                    sys.stderr.flush()
    if handle is not None:
        handle.close()
    return rows


# ----------------------------------------------------------------------------
# Tableaux.
# ----------------------------------------------------------------------------


def collect(rows, method, cut, field, keep=None):
    """Valeurs d'un champ pour une methode et une coupe, filtrees."""
    values = []
    for row in rows:
        if keep is not None and not keep(row):
            continue
        entry = row["methods"].get(method)
        if entry is None or cut not in entry:
            continue
        values.append(entry[cut][field])
    return np.asarray(values, dtype=np.float64)


def method_names(rows):
    """Noms de methodes presents, dans un ordre de lecture stable."""
    seen = []
    for row in rows:
        for name in row["methods"]:
            if name not in seen:
                seen.append(name)
    priority = {
        "single_linkage": 0,
        "ward": 90,
        "kmeans": 91,
    }

    def rank(name):
        if name in priority:
            return (priority[name], name)
        if name.startswith("ehgp_k"):
            return (10, "%03d" % int(name[6:]))
        if name.startswith("ehgp_min_k"):
            return (15, "%03d" % int(name[10:]))
        if name.startswith("reach_k"):
            return (20, "%03d" % int(name[7:]))
        if name.startswith("dbscan"):
            return (92, name)
        if name.startswith("hdbscan"):
            return (93, name)
        return (99, name)

    return sorted(seen, key=rank)


def table_by_dimension(rows, cut, field, families=None, noise=None, title=""):
    """Tableau texte : methodes en lignes, dimensions en colonnes."""
    dims = sorted({row["dim"] for row in rows})
    lines = []
    if title:
        lines.append(title)
    header = "%-22s" % "methode" + "".join("  d=%-11d" % dim for dim in dims)
    lines.append(header)
    lines.append("-" * len(header))
    for name in method_names(rows):
        cells = []
        for dim in dims:

            def keep(row, dim=dim):
                if row["dim"] != dim:
                    return False
                if families is not None and row["family"] not in families:
                    return False
                if noise is not None and row["noise_share"] not in noise:
                    return False
                return True

            values = collect(rows, name, cut, field, keep)
            if values.size == 0:
                cells.append("%-13s" % "    -")
            else:
                cells.append(
                    "%5.3f+-%-6.3f" % (float(values.mean()), float(values.std()))
                )
        lines.append("%-22s" % name + "  " + "  ".join(cells))
    return "\n".join(lines)


def table_by_family(rows, cut, field, title=""):
    """Tableau texte : methodes en lignes, familles en colonnes."""
    families = sorted({row["family"] for row in rows})
    lines = []
    if title:
        lines.append(title)
    header = "%-22s" % "methode" + "".join("  %-13s" % family for family in families)
    lines.append(header)
    lines.append("-" * len(header))
    for name in method_names(rows):
        cells = []
        for family in families:
            values = collect(
                rows, name, cut, field, lambda row, f=family: row["family"] == f
            )
            if values.size == 0:
                cells.append("%-13s" % "    -")
            else:
                cells.append(
                    "%5.3f+-%-6.3f" % (float(values.mean()), float(values.std()))
                )
        lines.append("%-22s" % name + "  " + "  ".join(cells))
    return "\n".join(lines)


def bracket_table(rows):
    """Qualite de l'encadrement par dimension : part prouvee exacte, ecart."""
    dims = sorted({row["dim"] for row in rows})
    orders = rows[0]["orders"]
    lines = ["encadrement du maximum de segment (part prouvee exacte / ecart relatif moyen)"]
    header = "%-22s" % "ordre" + "".join("  d=%-11d" % dim for dim in dims)
    lines.append(header)
    lines.append("-" * len(header))
    for slot, order in enumerate(orders):
        cells = []
        for dim in dims:
            selected = [row for row in rows if row["dim"] == dim]
            exact = np.asarray(
                [row["bracket"]["exact_fraction"][slot] for row in selected]
            )
            gap = np.asarray([row["bracket"]["gap_mean"][slot] for row in selected])
            cells.append("%5.3f / %-5.3f" % (float(exact.mean()), float(gap.mean())))
        lines.append("%-22s" % ("k=%d" % order) + "  " + "  ".join(cells))
    accord = []
    for dim in dims:
        values = []
        for row in rows:
            if row["dim"] != dim:
                continue
            for name, entry in row["methods"].items():
                if name.startswith("ehgp_k") and "bracket_agreement" in entry:
                    values.append(entry["bracket_agreement"])
        accord.append("%13.3f" % (float(np.mean(values)) if values else float("nan")))
    lines.append("%-22s" % "accord maj/min (ari)" + "  " + "  ".join(accord))
    return "\n".join(lines)


def timing_table(rows):
    """Temps mur par methode et par dimension."""
    dims = sorted({row["dim"] for row in rows})
    lines = ["temps mur moyen par cellule (secondes)"]
    header = "%-22s" % "methode" + "".join("  d=%-11d" % dim for dim in dims)
    lines.append(header)
    lines.append("-" * len(header))
    for name in method_names(rows):
        cells = []
        for dim in dims:
            values = [
                row["methods"][name]["wall"]
                for row in rows
                if row["dim"] == dim and name in row["methods"]
            ]
            cells.append(
                "%13.3f" % (float(np.mean(values)) if values else float("nan"))
            )
        lines.append("%-22s" % name + "  " + "  ".join(cells))
    return "\n".join(lines)


def verdict_table(rows, cut):
    """La question tranchee, chiffre par chiffre : `k > 1` contre `k = 1`.

    Pour chaque dimension, l'ecart d'indice de Rand ajuste entre le meilleur
    ordre `k > 1` et l'ordre `k = 1` (qui est la liaison simple), pour la
    tour E-HGP et pour la reachability mutuelle, sur tous les jeux puis sur
    les seuls jeux bruites. Un ecart negatif dit que l'axe d'ordre COUTE.
    """
    dims = sorted({row["dim"] for row in rows})
    lines = ["verdict : ari(meilleur k>1) - ari(k=1), coupe %s" % cut]
    header = "%-30s" % "grandeur" + "".join("  d=%-11d" % dim for dim in dims)
    lines.append(header)
    lines.append("-" * len(header))
    orders = [order for order in rows[0]["orders"] if order > 1]
    for prefix, label in (("ehgp_k", "tour E-HGP"), ("reach_k", "reach mutuelle")):
        for noise, tag in ((None, "tous jeux"), ((0.3,), "jeux bruites")):
            cells = []
            for dim in dims:

                def keep(row, dim=dim, noise=noise):
                    if row["dim"] != dim:
                        return False
                    return noise is None or row["noise_share"] in noise

                base = collect(rows, prefix + "1", cut, "ari", keep)
                best = None
                for order in orders:
                    values = collect(rows, prefix + str(order), cut, "ari", keep)
                    if values.size == 0:
                        continue
                    score = float(values.mean())
                    if best is None or score > best:
                        best = score
                if base.size == 0 or best is None:
                    cells.append("%13s" % "-")
                else:
                    cells.append("%+13.3f" % (best - float(base.mean())))
            lines.append("%-30s" % (label + ", " + tag) + "  " + "  ".join(cells))
    return "\n".join(lines)


def divergence_probe(families, dims, count, orders, seeds, intervals=8, mode="full"):
    """La tour est-elle un autre objet que la reachability mutuelle, et ou ?

    Compare les POIDS eux-memes, pas seulement les partitions :

    * `hors_extremites` : part des couples ou le maximum de segment depasse
      strictement `max(a_k(x_i), a_k(x_j))`, donc ou l'interieur du segment
      dit quelque chose que les extremites ignorent. Si cette part tombe a
      zero, la tour DEGENERE : son poids est celui des extremites, et elle ne
      peut plus se distinguer de la reachability mutuelle.
    * `different` / `au_dessus` / `en_dessous` : part des couples ou la tour
      et la reachability mutuelle (echelle du quart) donnent des poids
      differents, et de quel cote.
    """
    lines = ["poids : tour E-HGP contre reachability mutuelle, et part hors extremites"]
    header = "%-12s %5s %4s %14s %12s %12s %12s" % (
        "famille",
        "d",
        "k",
        "hors_extremites",
        "different",
        "au_dessus",
        "en_dessous",
    )
    lines.append(header)
    lines.append("-" * len(header))
    rows = []
    for family in families:
        for dim in dims:
            gathered = {order: [] for order in orders}
            for seed in seeds:
                rng = np.random.default_rng(seed * 7919 + dim)
                points, _truth, _clusters = make_dataset(family, rng, count, dim, 0.1)
                tower = FastPointTower(
                    points, orders=orders, intervals=intervals, mode=mode, certify=False
                )
                pairs = upper_pairs(tower.count)
                for order in tower.orders:
                    weights = tower.weights(order, "upper")
                    reach = mutual_reachability(
                        tower.entry, tower.distances, order, quarter=True
                    )
                    ends = np.maximum(
                        tower.entry[pairs[0], order - 1], tower.entry[pairs[1], order - 1]
                    )
                    scale = np.maximum(weights, 1e-300)
                    gathered[order].append(
                        (
                            float(np.mean((weights - ends) / scale > 1e-9)),
                            float(np.mean(np.abs(weights - reach) / scale > 1e-9)),
                            float(np.mean((weights - reach) / scale > 1e-9)),
                            float(np.mean((reach - weights) / scale > 1e-9)),
                        )
                    )
            for order in orders:
                values = np.asarray(gathered[order], dtype=np.float64)
                if values.size == 0:
                    continue
                means = values.mean(axis=0)
                rows.append(
                    {
                        "family": family,
                        "dim": dim,
                        "order": order,
                        "beyond_ends": means[0],
                        "different": means[1],
                        "above": means[2],
                        "below": means[3],
                    }
                )
                lines.append(
                    "%-12s %5d %4d %14.4f %12.4f %12.4f %12.4f"
                    % (family, dim, order, means[0], means[1], means[2], means[3])
                )
                sys.stderr.write(lines[-1] + "\n")
                sys.stderr.flush()
    return "\n".join(lines), rows


def scale_probe(counts, dim, orders, intervals, seed=11, modes=("tube", "full")):
    """Cout et qualite de l'encadrement quand `n` monte, jusqu'a `n = 2000`.

    Mesure ce que le module PEUT faire, pas ce qu'on aimerait qu'il fasse :
    temps mur, part des couples prouves exacts, ecart relatif moyen, part des
    couples dont le tube est certifie. Le mode complet est laisse de cote
    au-dela de la taille ou il devient deraisonnable.
    """
    rng = np.random.default_rng(seed)
    lines = [
        "cout de l'encadrement, famille gauss_iso, d=%d, m=%d intervalles" % (dim, intervals)
    ]
    header = "%-6s %-6s %10s %10s %12s %12s %12s" % (
        "n",
        "mode",
        "couples",
        "temps_s",
        "exact_k1",
        "exact_kmax",
        "ecart_moyen",
    )
    lines.append(header)
    lines.append("-" * len(header))
    rows = []
    for count in counts:
        points, _truth, _clusters = make_dataset("gauss_iso", rng, count, dim, 0.1)
        for mode in modes:
            if mode == "full" and count > 500:
                continue
            start = time.perf_counter()
            bracket = segment_brackets(
                points,
                orders,
                intervals=intervals,
                mode=mode,
                certify=(mode == "tube"),
            )
            wall = time.perf_counter() - start
            rows.append(
                {
                    "count": count,
                    "mode": mode,
                    "pairs": bracket["pairs"],
                    "wall": wall,
                    "exact_fraction": list(bracket["exact_fraction"]),
                    "gap_mean": list(bracket["gap_mean"]),
                    "tube_fraction": list(bracket["tube_fraction"]),
                }
            )
            lines.append(
                "%-6d %-6s %10d %10.2f %12.3f %12.3f %12.4f"
                % (
                    count,
                    mode,
                    bracket["pairs"],
                    wall,
                    bracket["exact_fraction"][0],
                    bracket["exact_fraction"][-1],
                    float(np.mean(bracket["gap_mean"])),
                )
            )
            sys.stderr.write(lines[-1] + "\n")
            sys.stderr.flush()
    return "\n".join(lines), rows


def report(rows):
    """Tous les tableaux du banc, en texte aligne."""
    blocks = [
        verdict_table(rows, "fixed"),
        verdict_table(rows, "peak"),
        table_by_dimension(
            rows,
            "fixed",
            "ari",
            title="ari, coupe au vrai nombre de classes, toutes familles, tous bruits",
        ),
        table_by_dimension(
            rows,
            "fixed",
            "ari",
            noise=(0.3,),
            title="ari, coupe au vrai nombre, SEULEMENT avec 30 pour cent de bruit de fond",
        ),
        table_by_dimension(
            rows,
            "auto",
            "ari",
            title="ari, coupe SPONTANEE (regle commune), toutes familles, tous bruits",
        ),
        table_by_dimension(
            rows,
            "peak",
            "ari",
            title="ari, coupe au PIC de groupes recevables (regle qui utilise les naissances)",
        ),
        table_by_dimension(
            rows,
            "peak",
            "noise_recall",
            noise=(0.3,),
            title="part du bruit de fond rejete, coupe au pic, bruit 30 pour cent",
        ),
        table_by_dimension(
            rows,
            "peak",
            "cluster_kept",
            noise=(0.3,),
            title="part des vrais points conserves, coupe au pic, bruit 30 pour cent",
        ),
        table_by_dimension(
            rows,
            "peak",
            "ari",
            noise=(0.3,),
            title="ari, coupe au pic, SEULEMENT avec 30 pour cent de bruit de fond",
        ),
        table_by_family(
            rows, "fixed", "ari", title="ari, coupe au vrai nombre, par famille"
        ),
        table_by_family(rows, "peak", "ari", title="ari, coupe au pic, par famille"),
        bracket_table(rows),
        timing_table(rows),
    ]
    return "\n\n".join(blocks)


# ----------------------------------------------------------------------------
# Porte.
# ----------------------------------------------------------------------------


GRAVEN_CLOUDS = (
    (
        "carre_plus_centre_d2",
        ((0, 0), (10, 0), (0, 10), (10, 10), (5, 5), (5, 0), (0, 5), (13, 13)),
    ),
    (
        "doublons_d3",
        ((0, 0, 0), (0, 0, 0), (5, 0, 0), (5, 0, 0), (2, 3, 0), (2, 3, 7), (9, 9, 9)),
    ),
    (
        "colineaires_sept_d3",
        ((0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 0, 0), (4, 0, 0), (5, 0, 0), (6, 0, 0)),
    ),
    (
        "cospherique_d3",
        ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1), (0, 0, 0)),
    ),
    (
        "simplexe_d5",
        (
            (0, 0, 0, 0, 0),
            (7, 0, 0, 0, 0),
            (0, 7, 0, 0, 0),
            (0, 0, 7, 0, 0),
            (0, 0, 0, 7, 0),
            (0, 0, 0, 0, 7),
            (3, 3, 3, 3, 3),
        ),
    ),
)

MINIMUM_PAIRS_CHECKED = 600
MINIMUM_CERTIFIED = 200
MINIMUM_TUBE_COMPARISONS = 4000
MINIMUM_ORDER_ONE_CHECKS = 12
REQUIRED_DIMENSIONS = (2, 3, 5, 20)


class TestBracketAgainstExact(unittest.TestCase):
    """L'encadrement flottant contre la version rationnelle de `segment.py`."""

    def test_graven_clouds(self):
        checked = 0
        certified = 0
        for name, cloud in GRAVEN_CLOUDS:
            points = np.asarray(cloud, dtype=np.float64)
            orders = tuple(range(1, min(5, points.shape[0]) + 1))
            for mode in ("full", "tube"):
                report_card = certify_against_exact(
                    points, orders, intervals=8, mode=mode
                )
                self.assertEqual(
                    report_card["inside"], report_card["checked"], name + "/" + mode
                )
                self.assertEqual(
                    report_card["certified_good"],
                    report_card["certified"],
                    name + "/" + mode,
                )
                self.assertEqual(
                    report_card["witness_agree"],
                    report_card["checked"],
                    name + "/" + mode,
                )
                self.assertLessEqual(report_card["low_violation"], 0.0, name)
                self.assertLessEqual(report_card["high_violation"], 0.0, name)
                checked += report_card["checked"]
                certified += report_card["certified"]
        self.assertGreaterEqual(checked, MINIMUM_PAIRS_CHECKED)
        self.assertGreaterEqual(certified, MINIMUM_CERTIFIED)

    def test_random_clouds_every_dimension(self):
        rng = np.random.default_rng(20260925)
        checked = 0
        for dim in REQUIRED_DIMENSIONS:
            for _trial in range(2):
                points = rng.integers(0, 40, size=(9, dim)).astype(np.float64)
                report_card = certify_against_exact(points, (1, 2, 3, 5), intervals=8)
                self.assertEqual(report_card["inside"], report_card["checked"])
                self.assertEqual(
                    report_card["certified_good"], report_card["certified"]
                )
                self.assertLessEqual(report_card["low_violation"], 0.0)
                self.assertLessEqual(report_card["high_violation"], 0.0)
                checked += report_card["checked"]
        self.assertGreaterEqual(checked, MINIMUM_PAIRS_CHECKED)


class TestTubeAgainstFull(unittest.TestCase):
    """Le mode tube egale le mode complet sur les couples qu'il certifie."""

    def test_bit_identical_on_certified_pairs(self):
        rng = np.random.default_rng(31)
        compared = 0
        for count, dim in ((40, 2), (40, 20), (60, 3), (80, 5)):
            points = rng.normal(size=(count, dim))
            orders = (1, 2, 5, 10)
            full = segment_brackets(points, orders, intervals=8, mode="full")
            tube = segment_brackets(points, orders, intervals=8, mode="tube")
            keep = tube["tube_certified"]
            for slot in range(len(orders)):
                mask = keep[slot]
                compared += int(mask.sum())
                self.assertTrue(
                    np.array_equal(full["upper"][slot][mask], tube["upper"][slot][mask])
                )
                self.assertTrue(
                    np.array_equal(full["lower"][slot][mask], tube["lower"][slot][mask])
                )
            self.assertGreaterEqual(full["order_violation"], -1e-9)
            self.assertGreaterEqual(tube["order_violation"], -1e-9)
            self.assertTrue(np.all(tube["upper"] >= tube["lower"] - 1e-9))
        self.assertGreaterEqual(compared, MINIMUM_TUBE_COMPARISONS)

    def test_upper_bound_increases_with_order(self):
        rng = np.random.default_rng(32)
        for count, dim in ((40, 2), (40, 50)):
            points = rng.normal(size=(count, dim))
            bracket = segment_brackets(points, (1, 2, 3, 4, 5), intervals=8, mode="full")
            for slot in range(1, 5):
                self.assertTrue(
                    np.all(
                        bracket["upper"][slot]
                        >= bracket["upper"][slot - 1] - 1e-12
                    )
                )
                self.assertTrue(
                    np.all(bracket["entry"][:, slot] >= bracket["entry"][:, slot - 1])
                )


class TestOrderOneIsSingleLinkage(unittest.TestCase):
    """Fait 1 du chantier, au niveau des partitions : `k = 1` est la liaison simple."""

    def test_partitions_agree(self):
        rng = np.random.default_rng(33)
        checks = 0
        for count, dim in ((60, 2), (60, 5), (60, 20), (40, 200)):
            points = rng.normal(size=(count, dim))
            tower = FastPointTower(points, orders=(1, 2), intervals=8, mode="full")
            simple = single_linkage_tower(tower.distances, count)
            for clusters in (2, 3, 5):
                left = tower.tower(1).labels_fixed(clusters)
                right = simple.labels_fixed(clusters)
                self.assertAlmostEqual(adjusted_rand_index(left, right), 1.0, places=12)
                checks += 1
        self.assertGreaterEqual(checks, MINIMUM_ORDER_ONE_CHECKS)


class TestMetrics(unittest.TestCase):
    """Les mesures d'accord, confrontees a scikit-learn et a leurs cas limites."""

    def test_extreme_cases(self):
        truth = np.array([0, 0, 1, 1, 2, 2, -1, -1])
        self.assertAlmostEqual(adjusted_rand_index(truth, truth), 1.0, places=12)
        self.assertAlmostEqual(normalized_mutual_information(truth, truth), 1.0, places=12)
        constant = np.zeros(truth.size, dtype=np.int64)
        self.assertAlmostEqual(adjusted_rand_index(truth, constant), 0.0, places=12)
        self.assertAlmostEqual(
            normalized_mutual_information(truth, constant), 0.0, places=12
        )

    def test_against_sklearn(self):
        if not HAS_SKLEARN:
            self.skipTest("scikit-learn absent")
        rng = np.random.default_rng(34)
        for _trial in range(20):
            truth = rng.integers(-1, 4, size=60)
            labels = rng.integers(-1, 5, size=60)
            self.assertAlmostEqual(
                adjusted_rand_index(truth, labels),
                float(adjusted_rand_score(truth, labels)),
                places=10,
            )
            self.assertAlmostEqual(
                normalized_mutual_information(truth, labels),
                float(
                    normalized_mutual_info_score(
                        truth, labels, average_method="arithmetic"
                    )
                ),
                places=10,
            )


class TestDatasets(unittest.TestCase):
    """Les jeux synthetiques ont bien la forme annoncee."""

    def test_shapes_and_noise(self):
        rng = np.random.default_rng(35)
        for family in FAMILIES:
            for dim in (2, 50):
                points, truth, clusters = make_dataset(family, rng, 120, dim, 0.3)
                self.assertEqual(points.shape, (120, dim))
                self.assertEqual(truth.size, 120)
                self.assertGreaterEqual(clusters, 3)
                self.assertGreater(int(np.sum(truth < 0)), 0)
                self.assertEqual(
                    int(np.unique(truth[truth >= 0]).size), clusters, family
                )

    def test_isometry_is_isometric(self):
        rng = np.random.default_rng(36)
        basis = random_isometry(rng, 50, 5)
        local = rng.normal(size=(20, 5))
        embedded = local @ basis.T
        left = squared_distances(local)
        right = squared_distances(embedded)
        self.assertTrue(np.allclose(left, right, atol=1e-9))


class TestReachabilityIsAnotherObject(unittest.TestCase):
    """La reachability mutuelle n'est pas la tour, et on le montre."""

    def test_disagreement_exists_in_low_dimension(self):
        rng = np.random.default_rng(37)
        points = rng.normal(size=(60, 2))
        tower = FastPointTower(points, orders=(1, 5), intervals=8, mode="full")
        distances = tower.distances
        reach = mutual_reachability(tower.entry, distances, 5, quarter=True)
        weights = tower.weights(5, "upper")
        self.assertEqual(weights.shape, reach.shape)
        self.assertGreater(int(np.sum(np.abs(weights - reach) > 1e-9)), 0)

    def test_order_one_coincides_without_intrusion(self):
        points = np.asarray([[0.0, 0.0], [10.0, 0.0]])
        tower = FastPointTower(points, orders=(1,), intervals=8, mode="full")
        reach = mutual_reachability(tower.entry, tower.distances, 1, quarter=True)
        self.assertAlmostEqual(float(tower.weights(1, "upper")[0]), 25.0, places=9)
        self.assertAlmostEqual(float(reach[0]), 25.0, places=9)


def run_gate():
    """Lance toutes les portes, renvoie le code de sortie exact."""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite(
        loader.loadTestsFromTestCase(case)
        for case in (
            TestBracketAgainstExact,
            TestTubeAgainstFull,
            TestOrderOneIsSingleLinkage,
            TestMetrics,
            TestDatasets,
            TestReachabilityIsAnotherObject,
        )
    )
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        return 0
    if result.errors:
        return 3
    return 1


def main(argv=None):
    """Point d'entree : `gate`, `run`, `table`."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("gate", help="portes de correction")
    runner = sub.add_parser("run", help="campagne de comparaison")
    runner.add_argument("--profile", default="quick", choices=sorted(PROFILES))
    runner.add_argument("--orders", default="1,2,5,10")
    runner.add_argument("--out", default=None)
    runner.add_argument("--limit", type=int, default=None)
    runner.add_argument("--min-size", type=int, default=5)
    shower = sub.add_parser("table", help="tableaux d'une campagne deja ecrite")
    shower.add_argument("--input", required=True)
    diverger = sub.add_parser("divergence", help="poids tour contre reachability")
    diverger.add_argument("--dims", default="2,10,50,200,1000")
    diverger.add_argument("--families", default="gauss_iso,manifold,filament")
    diverger.add_argument("--count", type=int, default=200)
    diverger.add_argument("--orders", default="2,5,10")
    diverger.add_argument("--seeds", default="1,2,3")
    scaler = sub.add_parser("scale", help="cout de l'encadrement quand n monte")
    scaler.add_argument("--counts", default="250,500,1000,2000")
    scaler.add_argument("--dim", type=int, default=50)
    scaler.add_argument("--orders", default="1,2,5,10")
    scaler.add_argument("--intervals", type=int, default=8)
    options = parser.parse_args(argv)
    if options.command == "gate":
        return run_gate()
    if options.command == "divergence":
        text, measured = divergence_probe(
            tuple(options.families.split(",")),
            tuple(int(item) for item in options.dims.split(",")),
            options.count,
            tuple(int(item) for item in options.orders.split(",")),
            tuple(int(item) for item in options.seeds.split(",")),
        )
        print(text)
        return 0 if measured else 3
    if options.command == "scale":
        text, measured = scale_probe(
            tuple(int(item) for item in options.counts.split(",")),
            options.dim,
            tuple(int(item) for item in options.orders.split(",")),
            options.intervals,
        )
        print(text)
        return 0 if measured else 3
    if options.command == "run":
        orders = tuple(int(item) for item in options.orders.split(","))
        rows = run_campaign(
            options.profile,
            orders,
            options.out,
            limit=options.limit,
            min_size=options.min_size,
        )
        if not rows:
            return 3
        print(report(rows))
        return 0
    rows = []
    with open(options.input, "r", encoding="ascii") as handle:
        for line in handle:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
    if not rows:
        return 3
    print(report(rows))
    return 0


if __name__ == "__main__":
    sys.exit(main())
