#!/usr/bin/env python3
"""Outils communs des experiences locales du dossier lidar/ (petits nuages seulement).

- Hierarchies de points EXACTES depuis la reference v11 (etage A `Definition` ou B `Reference`) : regles du banc
  v11 `core`, `cover`, `first` (bench/points_hierarchy.py) et H^r_{k+1} = `margin_r` (bench/points_radius.py),
  lues comme suite exacte de partitions (dates et niveaux compares par RValue, aucune decision flottante).
- Hierarchie de HDBSCAN : arbre du lien simple de l'atteignabilite mutuelle de scikit-learn (`min_samples = k`,
  point compte), un point « entre » a sa premiere fusion (equivalent pour mcs >= 2).
- Une seule tete pour toutes les hierarchies : condensation a mcs (N-aire, multifusions jamais binarisees),
  selection EOM avec lambda = r^-z (z = 1, 3) ou lambda = -log r, ou feuilles ; racine exclue
  (allow_single_cluster = False) ; egalite -> parent (comme scikit-learn). Validee contre les etiquettes de
  scikit-learn sur l'arbre de HDBSCAN (selftest_selection).
Les flottants ne servent qu'aux lambda (lecture) ; les partitions sont exactes.
"""
from fractions import Fraction
from functools import cmp_to_key
import math
import os
import sys

import numpy as np

V11 = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11'
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(V11, 'reference'))
sys.path.insert(0, os.path.join(V11, 'bench'))

from hgp11_ref import Definition, Reference  # noqa: E402
import points_hierarchy as ph  # noqa: E402
import points_radius as prad  # noqa: E402
import points_reference as pr  # noqa: E402

TOWER_RULES = ('core', 'cover', 'first', 'margin_r')


class Hier(object):
    """Hierarchie de points avec diagonale : steps = [(rayon flottant, partition)] croissants ; partition = liste de
    frozensets des sites ENTRES, groupes par bloc. Le dernier pas doit etre un bloc unique de tous les sites."""

    def __init__(self, name, n, steps):
        self.name, self.n, self.steps = name, n, steps


# ---------------------------------------------------------------------------------------------------------------
# Tour exacte -> hierarchies de points

def tower_order(points, k, stage='B'):
    points = [tuple(int(c) for c in p) for p in points]
    res = Definition(points).order(k) if stage == 'A' else Reference(points, kmax=k).order(k)
    return pr.order_from_definition(res, len(points), k), res


def tower_hangings(order, k):
    m = 1 if k == 1 else k + 1
    return {'core': ph.hang_core(order), 'cover': ph.hang_first(order, 1, 'cover'),
            'first': ph.hang_first(order, m, 'first'), 'margin_r': prad.hang_margin_radius(order, m)}


def _entry_values(h):
    if isinstance(h, prad.RadiusHanging):
        return list(h.values)
    return [prad.RValue(Fraction(int(h.num[i]), int(h.den[i]))) for i in range(h.order.n)]


def hier_from_hanging(name, h):
    order = h.order
    n = order.n
    entries = _entry_values(h)
    cands = list(entries)
    for v in range(order.size):
        cands.append(prad.RValue(Fraction(*order.levels.exact(int(order.rank[v])))))
    cands.sort(key=cmp_to_key(lambda a, b: a.cmp(b)))
    uniq = []
    for c in cands:
        if not uniq or uniq[-1].cmp(c) != 0:
            uniq.append(c)
    lo = min(range(n), key=cmp_to_key(lambda i, j: entries[i].cmp(entries[j])))
    uniq = [c for c in uniq if c.cmp(entries[lo]) >= 0]
    order_entries = sorted(range(n), key=cmp_to_key(lambda i, j: entries[i].cmp(entries[j])))
    steps, ptr, entered = [], 0, []
    for c in uniq:
        while ptr < n and entries[order_entries[ptr]].cmp(c) <= 0:
            entered.append(order_entries[ptr])
            ptr += 1
        groups = {}
        for i in entered:
            top = prad.ancestor_at_radius(order, int(h.owner[i]), c)
            groups.setdefault(top, []).append(i)
        part = [frozenset(g) for g in groups.values()]
        if steps and set(steps[-1][1]) == set(part):
            continue  # rien ne change pour les sites (evenement de noeud sans effet sur les blocs)
        steps.append((c.approx(), part))
    if len(steps[-1][1]) != 1 or len(steps[-1][1][0]) != n:
        raise AssertionError('le dernier pas doit reunir tous les sites')
    return Hier(name, n, steps)


def tower_hiers(points, k, stage='B', rules=TOWER_RULES):
    order, res = tower_order(points, k, stage)
    hs = tower_hangings(order, k)
    return {r: hier_from_hanging(r, hs[r]) for r in rules}, order, res


# ---------------------------------------------------------------------------------------------------------------
# HDBSCAN de scikit-learn -> hierarchie de points

def hdbscan_tree(xyz, k):
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_samples=int(k), min_cluster_size=2, metric='euclidean', algorithm='kd_tree', n_jobs=1,
                    copy=True)
    model.fit(np.asarray(xyz, dtype=np.float64))
    return np.asarray(model._single_linkage_tree_)


def hier_from_linkage(name, tree, n, group_ties=True):
    """group_ties=False : une fusion binaire par pas, dans l'ordre de scikit-learn (validation seulement : c'est
    la binarisation des ex aequo que fait scikit-learn) ; True : fusions de meme valeur en un seul pas N-aire."""
    left = tree['left_node'].astype(np.int64).tolist()
    right = tree['right_node'].astype(np.int64).tolist()
    value = tree['value'].astype(np.float64).tolist()
    parent = list(range(2 * n - 1))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    members = {i: [i] for i in range(n)}
    entered = set()
    steps, j = [], 0
    while j < len(value):
        v = value[j]
        while j < len(value) and value[j] == v:
            a, b = find(left[j]), find(right[j])
            new = n + j
            parent[a] = parent[b] = new
            members[new] = members.pop(a) + members.pop(b)
            entered.update(members[new])
            j += 1
            if not group_ties:
                break  # un pas par fusion binaire, meme a valeur egale
        part = [frozenset(x for x in mem if x in entered) for mem in members.values()]
        part = [p for p in part if p]
        steps.append((v, part))
    return Hier(name, n, steps)


# ---------------------------------------------------------------------------------------------------------------
# Tete commune : condensation, EOM, feuilles

def lam_fn(kind):
    if kind == 'log':
        return lambda r: -math.log(r) if r > 0 else float('inf')
    z = float(kind)
    return lambda r: r ** (-z) if r > 0 else float('inf')


def condense(hier, mcs, kind):
    """Arbre condense a mcs : clusters [{parent, lam, children, leave: {site: lambda}}], racine exclue des
    selections. Regle N-aire : 0 enfant lourd -> fin, 1 -> continuation, >= 2 -> scission simultanee."""
    g = lam_fn(kind)
    steps = hier.steps
    clusters = [dict(parent=None, lam=None, children=[], leave={})]
    active = {0: steps[-1][1][0]}
    for j in range(len(steps) - 1, -1, -1):
        r = steps[j][0]
        lam = g(r)
        below = steps[j - 1][1] if j > 0 else []
        where = {}
        for b in below:
            for x in b:
                where[x] = b
        nxt = {}
        for cid, block in active.items():
            kids = {}
            for x in block:
                b = where.get(x)
                if b is not None:
                    kids[b] = True
            kids = list(kids)
            heavy = [b for b in kids if len(b) >= mcs]
            heavy_pts = set().union(*heavy) if heavy else set()
            c = clusters[cid]
            if len(heavy) == 1:
                for x in block:
                    if x not in heavy_pts:
                        c['leave'][x] = lam
                nxt[cid] = heavy[0]
            else:
                for x in block:
                    c['leave'][x] = lam
                if len(heavy) >= 2:
                    for b in heavy:
                        clusters.append(dict(parent=cid, lam=lam, children=[], leave={}))
                        c['children'].append(len(clusters) - 1)
                        nxt[len(clusters) - 1] = b
        active = nxt
    for c in clusters:
        c['stability'] = None if c['lam'] is None else sum(l - c['lam'] for l in c['leave'].values())
    return clusters


def select(clusters, method):
    """Ensemble des clusters choisis ('eom' ou 'leaf'), racine exclue."""
    if method == 'leaf':
        return [i for i, c in enumerate(clusters) if i != 0 and not c['children']]
    best, chosen = {}, {}
    for i in range(len(clusters) - 1, 0, -1):
        c = clusters[i]
        if not c['children']:
            best[i], chosen[i] = c['stability'], [i]
            continue
        s = sum(best[ch] for ch in c['children'])
        if s > c['stability']:
            best[i], chosen[i] = s, [x for ch in c['children'] for x in chosen[ch]]
        else:
            best[i], chosen[i] = c['stability'], [i]
    return [x for ch in clusters[0]['children'] for x in chosen[ch]]


def labels_of(clusters, chosen, n):
    sel = set(chosen)
    last = {}
    for i, c in enumerate(clusters):
        for x in c['leave']:
            last[x] = i  # un site quitte un seul cluster (le plus profond qui le contient)
    out = np.full(n, -1, dtype=np.int64)
    rank = {c: j for j, c in enumerate(sorted(sel))}
    for x in range(n):
        i = last.get(x)
        while i is not None and i not in sel:
            i = clusters[i]['parent']
        if i is not None:
            out[x] = rank[i]
    return out


def flat(hier, mcs, kind, method):
    cl = condense(hier, mcs, kind)
    return labels_of(cl, select(cl, method), hier.n), cl


# ---------------------------------------------------------------------------------------------------------------
# Mesures

def best_block_iou(hier, gt):
    """Niveau B : meilleur IoU de chaque groupe vrai parmi tous les blocs publies."""
    out = []
    for g in gt:
        best = 0.0
        for _, part in hier.steps:
            for b in part:
                inter = len(b & g)
                if inter:
                    best = max(best, inter / len(b | g))
        out.append(best)
    return out


def flat_scores(labels, gt):
    """Pour chaque groupe vrai : IoU du meilleur cluster ; appariement > 1/2 (unique par construction) ; PQ-like."""
    clusters = {}
    for x, l in enumerate(labels.tolist()):
        if l >= 0:
            clusters.setdefault(l, set()).add(x)
    per, matched_c = [], set()
    for g in gt:
        best, arg = 0.0, None
        for l, c in clusters.items():
            inter = len(c & g)
            if inter:
                iou = inter / len(c | g)
                if iou > best:
                    best, arg = iou, l
        per.append(best)
        if best > 0.5:
            matched_c.add(arg)
    tp = sum(1 for v in per if v > 0.5)
    fp = len(clusters) - len(matched_c)
    fn = len(gt) - tp
    sq = sum(v for v in per if v > 0.5) / tp if tp else 0.0
    rq = tp / (tp + 0.5 * fp + 0.5 * fn) if (tp + fp + fn) else 0.0
    noise = int(np.sum(labels < 0))
    return dict(per=per, clusters=len(clusters), tp=tp, fp=fp, fn=fn, pq=sq * rq, noise=noise)


def selftest_selection(seeds=40, verbose=False):
    """La tete commune, appliquee a l'arbre de HDBSCAN avec lambda = 1/r, doit rendre les etiquettes de
    scikit-learn (EOM et feuilles), a permutation pres, sur des nuages sans ex aequo."""
    from sklearn.cluster import HDBSCAN
    bad = 0
    total = 0
    for s in range(seeds):
        rng = np.random.RandomState(1000 + s)
        n = rng.randint(30, 90)
        X = np.concatenate([rng.randn(n // 3, 3) * 0.3 + rng.randn(3) * 3 for _ in range(3)])
        n = len(X)
        for k in (2, 3, 5):
            tree = hdbscan_tree(X, k)
            hier = hier_from_linkage('hdbscan', tree, n, group_ties=False)
            for mcs in (3, 5, 8):
                for method in ('eom', 'leaf'):
                    ref = HDBSCAN(min_samples=k, min_cluster_size=mcs, cluster_selection_method=method,
                                  copy=True).fit(X).labels_
                    got, _ = flat(hier, mcs, 1, method)
                    total += 1
                    if not same_partition(ref, got):
                        bad += 1
                        if verbose:
                            print('ecart', s, k, mcs, method)
    return total, bad


def same_partition(a, b):
    a, b = np.asarray(a), np.asarray(b)
    if not np.array_equal(a < 0, b < 0):
        return False
    m = {}
    for x, y in zip(a.tolist(), b.tolist()):
        if x < 0:
            continue
        if m.setdefault(x, y) != y:
            return False
    return len(set(m.values())) == len(m)


def select_box(clusters, coords, box, kind_note=''):
    """Tete « EOM-boite » (cout geometrique du § 5.2 de la these, a la maniere du decoupage par boites d'ALPINE) :
    V(C) = max(S(C), somme V(enfants)) si C tient dans la boite de reference, sinon somme V(enfants) ; une feuille
    qui ne tient pas est gardee (politique declaree). Faisabilite monotone (un sous-ensemble d'un ensemble qui tient
    tient aussi) : c'est l'EOM restreinte aux sous-arbres maximaux faisables. box = etendues maximales par axe."""
    def fits(i):
        pts = [coords[x] for x in clusters[i]['leave']]
        return all(max(p[a] for p in pts) - min(p[a] for p in pts) <= box[a] for a in range(len(box)))
    best, chosen = {}, {}
    for i in range(len(clusters) - 1, 0, -1):
        c = clusters[i]
        ok = fits(i)
        if not c['children']:
            best[i], chosen[i] = (c['stability'] if ok else 0.0), [i]
            continue
        s = sum(best[ch] for ch in c['children'])
        if ok and s <= c['stability']:
            best[i], chosen[i] = c['stability'], [i]
        else:
            best[i], chosen[i] = s, [x for ch in c['children'] for x in chosen[ch]]
    return [x for ch in clusters[0]['children'] for x in chosen[ch]]
