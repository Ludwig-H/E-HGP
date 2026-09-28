"""References du banc : HDBSCAN par defaut, HDBSCAN oracle, liaison simple.

L'adversaire declare est HDBSCAN dans ses MEILLEURES conditions : `hdbscan_oracle`
choisit, pour chaque scene, la taille minimale de groupe qui maximise l'ARI
contre la verite. C'est un oracle, donc une borne superieure inatteignable en
usage reel ; le battre est la seule preuve qui vaille. `hdbscan_default` est
l'usage reel (taille fixee d'avance, jamais ajustee par scene).

Le bruit predit (-1) est traite comme une classe a part dans l'ARI, comme le
fait la litterature ; la couverture est publiee a cote pour qu'un score obtenu
en rejetant la moitie des points se voie.
"""

import warnings

import numpy as np

warnings.filterwarnings('ignore')

DEFAULT_MIN_CLUSTER_SIZE = 20
ORACLE_GRID = (5, 10, 15, 20, 30, 50, 75, 100)


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def scores(truth, predicted):
    """ARI, AMI, couverture, nombre de groupes, bruit predit."""
    from sklearn.metrics import adjusted_mutual_info_score, adjusted_rand_score
    truth = np.asarray(truth)
    predicted = np.asarray(predicted)
    need(truth.shape == predicted.shape, 'truth and prediction have the same length')
    covered = predicted >= 0
    return dict(ari=float(adjusted_rand_score(truth, predicted)),
                ami=float(adjusted_mutual_info_score(truth, predicted)),
                coverage=float(covered.mean()),
                clusters=int(len(set(predicted.tolist()) - {-1})),
                noise=int((~covered).sum()))


def hdbscan_labels(points, min_cluster_size, min_samples=None):
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_cluster_size=int(min_cluster_size),
                    min_samples=None if min_samples is None else int(min_samples), copy=True)
    return model.fit(np.asarray(points, dtype=np.float64)).labels_


def hdbscan_default(points, truth):
    labels = hdbscan_labels(points, DEFAULT_MIN_CLUSTER_SIZE)
    return dict(scores(truth, labels), method='hdbscan_default', parameter=DEFAULT_MIN_CLUSTER_SIZE)


def hdbscan_oracle(points, truth, grid=ORACLE_GRID):
    """Le meilleur HDBSCAN par scene : borne superieure, annoncee comme telle."""
    best = None
    for size in grid:
        if size * 2 > len(points):
            continue
        row = dict(scores(truth, hdbscan_labels(points, size)), method='hdbscan_oracle', parameter=int(size))
        if best is None or row['ari'] > best['ari']:
            best = row
    need(best is not None, 'no admissible min_cluster_size for this scene')
    return best


def single_linkage(points, truth, clusters):
    """Temoin negatif : liaison simple au nombre exact de groupes (chainage)."""
    from sklearn.cluster import AgglomerativeClustering
    model = AgglomerativeClustering(n_clusters=int(clusters), linkage='single')
    labels = model.fit_predict(np.asarray(points, dtype=np.float64))
    return dict(scores(truth, labels), method='single_linkage', parameter=int(clusters))
