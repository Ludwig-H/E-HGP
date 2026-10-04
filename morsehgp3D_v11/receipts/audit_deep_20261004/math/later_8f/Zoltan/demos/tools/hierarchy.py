"""Hiérarchie HDBSCAN* non condensée (min_samples = K, min_cluster_size = 1).

Convention de K (celle de scikit-learn, reprise par le plan de qualité du
dépôt, ``docs/PERFORMANCE_MORSEHGP3D.md``) : la distance-cœur d'un point est
la distance à son K-ième plus proche voisin, **le point lui-même compté**.
K = 1 donne donc le lien simple euclidien (clustering euclidien), et la
boule de rayon ``core_K(x)`` centrée en x contient K points de l'échantillon,
comme une boule d'ordre K de la tour HGP.

Le niveau r de la hiérarchie est le graphe dont les sommets sont les points
de distance-cœur <= r et les arêtes les paires avec
``max(core_K(a), core_K(b), |a - b|) <= r`` ; ses composantes connexes sont
les clusters au niveau r (DBSCAN(eps=r, min_samples=K) sans ses points de
bord). Avec ``min_cluster_size = 1`` rien n'est condensé : l'arbre complet
est l'arbre du lien simple de cet arbre couvrant minimal (MST), et toute
extraction HDBSCAN (EOM, feuilles, coupe) choisit des nœuds de cet arbre.

Le MST est calculé par Borůvka sur kd-arbre (bibliothèque ``hdbscan``,
``approx_min_span_tree=False``) sur la trame **entière** ; la bibliothèque
compte K sans le point lui-même, d'où ``min_samples = K - 1``. L'oracle
``brute_mst`` (Prim dense, O(n^2)) le vérifie aux petites tailles.
"""
from __future__ import annotations

import numpy as np
from scipy.spatial import cKDTree


def core_distances(X: np.ndarray, K: int) -> np.ndarray:
    if K <= 1:
        return np.zeros(len(X))
    d, _ = cKDTree(X).query(X, k=K, workers=-1)
    return d[:, K - 1]


def mreach_mst(X: np.ndarray, K: int, n_jobs: int = 4):
    """(distances-cœur, arêtes (n-1, 3) [a, b, poids] triées par poids)."""
    from hdbscan._hdbscan_boruvka import KDTree, KDTreeBoruvkaAlgorithm

    X = np.ascontiguousarray(X, dtype=np.float64)
    tree = KDTree(X, metric='euclidean', leaf_size=40)
    alg = KDTreeBoruvkaAlgorithm(tree, max(K - 1, 1), metric='euclidean', leaf_size=40 // 3,
                                 approx_min_span_tree=False, n_jobs=n_jobs)
    mst = alg.spanning_tree()
    mst = mst[np.argsort(mst[:, 2], kind='stable')]
    return core_distances(X, K), mst


def brute_mst(X: np.ndarray, K: int):
    """Oracle O(n^2) : poids triés du MST de la distance d'atteignabilité mutuelle."""
    core = core_distances(X, K)
    D = np.sqrt(((X[:, None, :] - X[None, :, :]) ** 2).sum(-1))
    M = np.maximum(D, np.maximum(core[:, None], core[None, :]))
    n = len(X)
    in_tree = np.zeros(n, bool)
    in_tree[0] = True
    best = M[0].copy()
    weights = []
    for _ in range(n - 1):
        best[in_tree] = np.inf
        j = int(np.argmin(best))
        weights.append(best[j])
        in_tree[j] = True
        best = np.minimum(best, M[j])
    return core, np.sort(np.array(weights))


class DSU:
    def __init__(self, n: int):
        self.p = np.arange(n)

    def find(self, a: int) -> int:
        p = self.p
        r = a
        while p[r] != r:
            r = p[r]
        while p[a] != r:
            p[a], a = r, p[a]
        return r


def weight_groups(mst):
    """Intervalles [début, fin) des arêtes de même poids (le MST est trié).

    Un nœud de l'arbre n'existe qu'une fois toutes les arêtes d'un même poids
    traitées : une union partielle au milieu d'un groupe d'ex æquo n'est une
    composante à aucun niveau.
    """
    w = mst[:, 2]
    cuts = np.flatnonzero(np.diff(w) != 0) + 1
    return list(zip(np.concatenate([[0], cuts]).tolist(), np.concatenate([cuts, [len(w)]]).tolist()))


def best_and_first_nodes(births, mst, labels, objects, ignore=None):
    """Pour chaque objet : meilleur nœud (IoU maximal) et premier nœud apparié (IoU > 0,5).

    ``labels`` : identifiant d'objet par point (-1 = fond). Les fusions d'un
    même poids sont traitées en bloc ; seuls les objets dont le compte a
    augmenté dans le bloc peuvent améliorer leur IoU, ce qui rend le parcours
    quasi linéaire. Un nœud est décrit par (niveau, indice de la dernière
    arête du bloc, racine), indice -1 pour un singleton.

    Toutes les composantes d'IoU > 0,5 avec un objet en contiennent plus de
    la moitié des points : elles forment une chaîne emboîtée. Une graine prise
    dans le premier nœud apparié traverse donc tous les nœuds appariés.
    ``ignore`` (booléens) : points exclus de l'IoU, comme les points « void »
    de l'évaluation panoptique de SemanticKITTI (retirés avant l'appariement) ;
    ils restent dans l'arbre et peuvent y servir de ponts.
    Retour : (best, first) avec best[o] = (IoU, niveau, indice, racine) et
    first[o] = (niveau, indice, racine) ou None.
    """
    n = len(births)
    objects = [int(o) for o in objects]
    wanted = set(objects)
    m = {o: int((labels == o).sum()) for o in objects}
    best, first = {}, {o: None for o in objects}
    # un singleton {i} n'est un nœud que si i naît strictement avant sa première fusion
    first_merge = np.full(n, np.inf)
    if len(mst):
        ends = mst[:, :2].astype(np.int64)
        np.minimum.at(first_merge, ends[:, 0], mst[:, 2])
        np.minimum.at(first_merge, ends[:, 1], mst[:, 2])
    alone = births < first_merge
    for o in objects:
        idx = np.flatnonzero((labels == o) & alone)
        if len(idx):
            i = int(idx[np.argmin(births[idx])])
            best[o] = (1.0 / m[o], float(births[i]), -1, i)
            if m[o] == 1:
                first[o] = (float(births[i]), -1, i)
        else:
            best[o] = (0.0, float('nan'), -1, -1)
    size = np.ones(n, np.int64) if ignore is None else (~np.asarray(ignore, bool)).astype(np.int64)
    counts = [({int(labels[i]): 1} if int(labels[i]) in wanted else {}) for i in range(n)]
    dsu = DSU(n)
    for g0, g1 in weight_groups(mst):
        gained: dict[int, set] = {}
        for e in range(g0, g1):
            ra, rb = dsu.find(int(mst[e, 0])), dsu.find(int(mst[e, 1]))
            if ra == rb:
                continue
            if len(counts[ra]) < len(counts[rb]):
                ra, rb = rb, ra
            dsu.p[rb] = ra
            size[ra] += size[rb]  # taille évaluée : points non ignorés
            ca, cb = counts[ra], counts[rb]
            for o, c in cb.items():
                ca[o] = ca.get(o, 0) + c
            counts[rb] = None
            gained[ra] = gained.pop(ra, set()) | gained.pop(rb, set()) | set(cb)
        w = float(mst[g1 - 1, 2])
        for root, objs in gained.items():
            sz, ca = size[root], counts[root]
            for o in objs:
                c = ca[o]
                iou = c / (sz + m[o] - c)
                if iou > best[o][0]:
                    best[o] = (iou, w, g1 - 1, int(root))
                if iou > 0.5 and first[o] is None:
                    first[o] = (w, g1 - 1, int(root))
    return best, first


def best_nodes(births, mst, labels, objects, ignore=None):
    """Meilleur IoU sur tous les nœuds de l'arbre : objet -> (IoU, niveau, indice, racine)."""
    return best_and_first_nodes(births, mst, labels, objects, ignore)[0]


def node_members(n, mst, merge_index, root):
    """Points du nœud existant après les arêtes 0..``merge_index`` (racine ``root``).

    ``merge_index`` doit être la dernière arête d'un bloc de même poids."""
    dsu = DSU(n)
    for e in range(merge_index + 1):
        a, b = int(mst[e, 0]), int(mst[e, 1])
        ra, rb = dsu.find(a), dsu.find(b)
        if ra != rb:
            dsu.p[rb] = ra
    r = dsu.find(int(root))
    return np.array([i for i in range(n) if dsu.find(i) == r])


# États d'une branche suivie, critère d'appariement de la qualité panoptique (PQ).
UNBORN, FRAGMENTED, MATCHED, FUSED = 0, 1, 2, 3


def branch_state(iou, recall, precision, other_seed):
    """IoU > 0,5 : apparié (critère de la qualité panoptique). Sinon la cause
    dominante décide : fusion si la branche contient un autre objet suivi ou si
    sa précision est plus faible que son rappel, fragmentation sinon."""
    if iou > 0.5:
        return MATCHED
    if other_seed or precision < recall:
        return FUSED
    return FRAGMENTED


def replay_tracks(births, mst, obj, seeds, coords, r_cap, ignore=None):
    """Rejoue l'algorithme de Kruskal et suit la composante de chaque graine.

    ``obj`` : indice d'objet suivi (0..k-1) ou -1 ; ``seeds`` : un point par
    objet ; ``coords`` : coordonnées (n, 3) du repère d'affichage (boîtes).
    Retourne ``join`` (k, n) = niveau où chaque point entre dans la branche o
    (inf sinon), et pour chaque branche la liste d'événements
    [niveau, boîte(6), rappel, précision, taille, masque des autres graines, état, IoU].
    ``ignore`` : points exclus de la précision et de l'IoU (void), comptés dans la taille et la boîte.
    """
    n, k = len(births), len(seeds)
    m = np.array([(obj == o).sum() for o in range(k)], dtype=np.float64)
    dsu = DSU(n)
    size = np.ones(n, np.int64)
    evald = np.ones(n, np.int64) if ignore is None else (~np.asarray(ignore, bool)).astype(np.int64)
    cnt = np.zeros((n, k), np.int64)
    for o in range(k):
        cnt[obj == o, o] = 1
    box = np.concatenate([coords, coords], axis=1).astype(np.float64)
    members: list[list[int] | None] = [[i] for i in range(n)]
    join = np.full((k, n), np.inf)
    events = [[] for _ in range(k)]

    def record(o, level, root):
        mask = 0
        for q in range(k):
            if q != o and dsu.find(int(seeds[q])) == root:
                mask |= 1 << q
        c = cnt[root, o]
        rec, prec = c / m[o], c / max(evald[root], 1)
        iou = c / (evald[root] + m[o] - c)
        st = branch_state(iou, rec, prec, mask != 0)
        events[o].append([float(level), *map(float, box[root]), float(rec), float(prec), int(size[root]), mask, st, float(iou)])

    for o in range(k):
        s = int(seeds[o])
        join[o, s] = births[s]
        record(o, births[s], s)
    for g0, g1 in weight_groups(mst):
        w = float(mst[g0, 2])
        if w > r_cap:
            break
        touched = set()
        for e in range(g0, g1):
            ra, rb = dsu.find(int(mst[e, 0])), dsu.find(int(mst[e, 1]))
            if ra == rb:
                continue
            roots_before = [dsu.find(int(s)) for s in seeds]
            if len(members[ra]) < len(members[rb]):
                ra, rb = rb, ra
            dsu.p[rb] = ra
            size[ra] += size[rb]
            evald[ra] += evald[rb]
            cnt[ra] += cnt[rb]
            box[ra, :3] = np.minimum(box[ra, :3], box[rb, :3])
            box[ra, 3:] = np.maximum(box[ra, 3:], box[rb, 3:])
            for o in range(k):
                if roots_before[o] == ra:
                    newcomers = members[rb]
                elif roots_before[o] == rb:
                    newcomers = members[ra]
                else:
                    continue
                jo = join[o]
                for i in newcomers:
                    if jo[i] == np.inf:
                        jo[i] = w
                touched.add(o)
            members[ra].extend(members[rb])
            members[rb] = None
        # un seul événement par niveau : l'état de la composante une fois le bloc fusionné
        for o in sorted(touched):
            record(o, w, dsu.find(int(seeds[o])))
    return join, events
