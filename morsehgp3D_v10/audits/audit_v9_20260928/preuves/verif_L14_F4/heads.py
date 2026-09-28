"""Tetes de clustering candidates sur une tranche K (proxy degree-Rips = HDBSCAN mreach).

Ecrit pour l'audit L14 (lecture seule du depot). Implementation independante :
- condensation HDBSCAN standard sur l'arbre binaire de sklearn (_single_linkage_tree_),
- stabilites EOM avec lambda = (1/d)^z (z reel) ou echelle log,
- selection EOM (racine exclue) ou feuilles,
- ToMATo sur l'ordre des fusions (regle de l'aine), seuil au plus grand ecart de proeminence,
- remplissage du bruit par plus proche point classe.
"""
import numpy as np
from sklearn.cluster import HDBSCAN
from sklearn.neighbors import NearestNeighbors


def slt(points, ms, mcs_for_fit=5):
    model = HDBSCAN(min_cluster_size=mcs_for_fit, min_samples=int(ms), copy=True).fit(points)
    return model._single_linkage_tree_


def condense(tree, n, mcs):
    """Arbre condense : liste de clusters (parent, birth_d) et sorties des points.

    Rend clusters: dict id -> dict(parent, birth, children, falls=[(point, d)] , child_splits=[(child, d, size)])
    Distances d (pas lambda) ; la stabilite est calculee ensuite pour toute echelle.
    """
    left = tree['left_node']; right = tree['right_node']; value = tree['value']; size = tree['cluster_size']
    m = len(tree)
    root = n + m - 1

    def node_size(x):
        return 1 if x < n else int(size[x - n])

    def leaves(x):
        out, st = [], [x]
        while st:
            y = st.pop()
            if y < n:
                out.append(y)
            else:
                st.append(int(left[y - n])); st.append(int(right[y - n]))
        return out

    clusters = {0: dict(parent=None, birth=np.inf, children=[], falls=[], splits=[])}
    stack = [(root, 0)]
    while stack:
        node, cid = stack.pop()
        if node < n:
            # a single point reached while still in cluster: it falls at d=0 -> use its own exit at parent's split
            clusters[cid]['falls'].append((node, 0.0))
            continue
        d = float(value[node - n]); a = int(left[node - n]); b = int(right[node - n])
        sa, sb = node_size(a), node_size(b)
        if sa >= mcs and sb >= mcs:
            for child in (a, b):
                new = len(clusters)
                clusters[new] = dict(parent=cid, birth=d, children=[], falls=[], splits=[])
                clusters[cid]['children'].append(new)
                clusters[cid]['splits'].append((new, d, node_size(child)))
                stack.append((child, new))
        elif sa >= mcs:
            for p in leaves(b):
                clusters[cid]['falls'].append((p, d))
            stack.append((a, cid))
        elif sb >= mcs:
            for p in leaves(a):
                clusters[cid]['falls'].append((p, d))
            stack.append((b, cid))
        else:
            for p in leaves(a) + leaves(b):
                clusters[cid]['falls'].append((p, d))
    return clusters


def _lam(d, z):
    if z == 'log':
        return -np.log(d) if d > 0 else np.inf
    return (1.0 / d) ** z if d > 0 else np.inf


def stabilities(clusters, z):
    out = {}
    for cid, c in clusters.items():
        if c['parent'] is None:
            # sklearn: root born at lambda 0 (d = inf) ; for log we use the root's first split / max d
            lb = 0.0 if z != 'log' else None
        else:
            lb = _lam(c['birth'], z)
        s = 0.0
        if lb is None:
            # log scale root: birth at the largest distance seen inside it
            ds = [d for _, d in c['falls'] if d > 0] + [d for _, d, _ in c['splits']]
            lb = _lam(max(ds), z) if ds else 0.0
        for _, d in c['falls']:
            if d > 0:
                s += max(0.0, _lam(d, z) - lb) if np.isfinite(_lam(d, z)) else 0.0
        for _, d, sz in c['splits']:
            s += sz * max(0.0, _lam(d, z) - lb)
        out[cid] = s
    return out


def select(clusters, stab, method='eom', allow_single=False):
    order = sorted(clusters)  # parents have smaller ids than children (created in DFS order)
    if method == 'leaf':
        sel = [c for c in order if not clusters[c]['children'] and (allow_single or clusters[c]['parent'] is not None)]
        return sel
    best, chosen = {}, {}
    for c in reversed(order):
        kids = clusters[c]['children']
        below = sum(best[k] for k in kids)
        own = stab[c]
        is_root = clusters[c]['parent'] is None
        if not kids:
            best[c], chosen[c] = own, (allow_single or not is_root)
        elif own >= below and (allow_single or not is_root):
            best[c], chosen[c] = own, True
        else:
            best[c], chosen[c] = below, False
    sel, st = [], [c for c in order if clusters[c]['parent'] is None]
    while st:
        c = st.pop()
        if chosen[c]:
            sel.append(c)
        else:
            st.extend(clusters[c]['children'])
    return sel


def labels_from(clusters, sel, n):
    lab = np.full(n, -1, dtype=np.int64)
    for j, c in enumerate(sel):
        st = [c]
        while st:
            x = st.pop()
            for p, _ in clusters[x]['falls']:
                lab[p] = j
            st.extend(clusters[x]['children'])
    return lab


def fill_noise(points, lab):
    lab = lab.copy()
    noise = lab < 0
    if noise.all() or not noise.any():
        return lab
    nn = NearestNeighbors(n_neighbors=1).fit(points[~noise])
    idx = nn.kneighbors(points[noise], return_distance=False)[:, 0]
    lab[noise] = lab[~noise][idx]
    return lab


def core_distances(points, ms):
    nn = NearestNeighbors(n_neighbors=int(ms)).fit(points)
    return nn.kneighbors(points)[0][:, -1]


def tomato(tree, core, n, mcs, max_clusters=64):
    """ToMATo sur l'ordre des fusions de l'arbre mreach, regle de l'aine.

    Pic d'une composante = plus petite distance-coeur (densite max). A la fusion de hauteur h,
    la plus jeune (pic plus grand) meurt avec persistance log(h/pic) si sa taille >= mcs.
    Nombre de groupes = 1 + indice du plus grand ecart des persistances triees.
    Rend (labels couvrant tout, persistances triees, ecart choisi, k).
    """
    left = tree['left_node']; right = tree['right_node']; value = tree['value']
    m = len(tree)
    parent = list(range(n + m))
    peak = np.concatenate([core, np.zeros(m)])
    sz = np.concatenate([np.ones(n, dtype=np.int64), np.zeros(m, dtype=np.int64)])
    rep = list(range(n + m))  # representative (mode id) of each tree node
    deaths = []  # (persistence, younger_mode, node)
    for i in range(m):
        a, b, h = int(left[i]), int(right[i]), float(value[i])
        ra, rb = rep[a], rep[b]
        pa, pb = peak[ra], peak[rb]
        young, old = (ra, rb) if pa > pb or (pa == pb and ra > rb) else (rb, ra)
        s_young = sz[a] if rep[a] == young else sz[b]
        node = n + i
        sz[node] = sz[a] + sz[b]
        if s_young >= mcs and h > 0 and peak[young] > 0:
            deaths.append((np.log(h / peak[young]), young, i))
        rep[node] = old
    pers = sorted((d[0] for d in deaths), reverse=True)
    if not pers:
        return np.zeros(n, dtype=np.int64), pers, 0.0, 1
    padded = pers[:max_clusters] + [0.0]
    gaps = [padded[j] - padded[j + 1] for j in range(len(padded) - 1)]
    j = int(np.argmax(gaps))
    k = j + 2  # j+1 prominent deaths kept + eldest
    tau = 0.5 * (padded[j] + padded[j + 1])
    # second pass: finalize prominent components
    prominent = {(young, i) for p, young, i in deaths if p > tau}
    lab = np.full(n + m, -1, dtype=np.int64)
    # components as sets of points, via union of tree children; finalize on prominent death
    members = {}
    for p in range(n):
        members[p] = [p]
    final = []
    for i in range(m):
        a, b = int(left[i]), int(right[i])
        ra, rb = rep[a], rep[b]
        node = n + i
        young = ra if rep[node] == rb else rb
        ma, mb = members.pop(a), members.pop(b)
        if (young, i) in prominent:
            # younger branch is finalized as its own cluster
            ym, om = (ma, mb) if young == ra else (mb, ma)
            final.append(ym)
            members[node] = om
        else:
            ma.extend(mb)
            members[node] = ma
    root_members = members.pop(n + m - 1)
    final.append(root_members)
    out = np.full(n, -1, dtype=np.int64)
    for c, pts in enumerate(final):
        out[np.asarray(pts, dtype=np.int64)] = c
    return out, pers, float(gaps[j]), len(final)
