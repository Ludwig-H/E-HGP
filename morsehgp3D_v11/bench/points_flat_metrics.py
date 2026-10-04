#!/usr/bin/env python3
"""Metriques d'une sortie plate avec bruit contre une verite terrain (experience E1, v11).

    python3 bench/points_flat_metrics.py --self-test     # code 0 si conforme, 3 sinon (fixture F15 de la porte)

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, public_status=not_claimed. Specification :
build/v11-points-select/juge/SPEC.md, par. 3.1 (metriques), par. 4.1 (signatures), par. 4.2 point 7,
par. 4.3 fixture F15.

PROVENANCE (port explicite, aucun import du code prive build/v11-points-select/) :
  - build/v11-points-select/mesure/metriques.py
      sha256 9f33b1a90a230f6538478054f8370651fdccb275951643a44ed74951bd43c8ec
    contingence, reduction M2 du hongrois, seuil entier M1, conventions du bruit. RETIRE : le " certificat
    d'optimalite (borne duale) " de hungarian_iou, refute par build/v11-points-select/verif_mesure (C08, v09 b) :
    la borne " somme des maxima de lignes " accepte une affectation sous-optimale. Aucun certificat d'optimalite
    n'est revendique ici : l'affectation vient de scipy (flottant), sa somme est recalculee exactement.
  - build/v11-points-select/mesure/fixtures_metriques.py
      sha256 fd57b1771a98e4ef9d6ae0a48a6130be3230a932cffdbfd4aab3bbd50938b8c5
    fixtures F1 a F11 (valeurs exactes), reprises dans self_test ; F6 est gravee (le prototype la cherchait par
    enumeration) ; F10 passe par le masque `void` ; F11 passe par la regle FP de la SPEC (>= fp_min_thing points
    thing), qui rend les memes comptes que le filtre " majorite stuff " du prototype sur ce cas.
  - build/v11-points-select/verif_mesure/scripts/v09_metriques.py
      sha256 d71020bc79ea0ea71a340dff544fe509f0f0c17d32c4b71728237da435f9e42f
    controle de la reduction M2 contre la force brute exacte (repris dans self_test, graine propre).
  - build/v11-points-select/equite/nary_head.py
      sha256 cbf4be1d8bc5516d20814fd38b94542b4ba8f68285fe430d7065d4b1d0bfbdc1
    definition de m05 (score_flat : somme des IoU > 1/2 sur le nombre de groupes) et programme dynamique de
    oracle_matched, f(v) = max(w(v), somme des f(enfants)), racine exclue ; controle contre l'enumeration des
    antichaines (antichains) repris dans self_test.
  - morsehgp3D_v11/bench/points_hierarchy.py (meme depot) : classes THING et VOID de SemanticKITTI, recopiees.

Conventions (SPEC par. 3.1). Verite : groupe >= 0, bruit vrai -1 (il compte dans |C| du cluster qui l'avale).
Prediction : cluster >= 0, bruit < 0. `void` : masque booleen des points retires de TOUT calcul (LiDAR : classes
0, 1, 52, 99), clusters et comptes compris. Une etiquette de verite < -1 est refusee (pas de second codage du void).
IoU(G, C) = n / (|G| + |C| - n), n = |G inter C|.

Decisions exactes (SPEC par. 4.2 point 7) : IoU > 1/2 <=> 3 n > |G| + |C| (M1, test entier ; l'appariement est alors
unique). Les sommes d'IoU, PQ, SQ, RQ, m05, precisions et rappels sont calcules en Fraction ; ARI_s est calcule
exactement par la matrice de confusion des paires (meme formule que sklearn.metrics.adjusted_rand_score).
La valeur hongroise reste un flottant de lecture : l'affectation est celle de scipy.optimize.linear_sum_assignment
(somme d'IoU maximale au flottant pres), sa somme est exacte ; aucune decision ne depend d'un ex aequo flottant.

M2 (reduction du hongrois, prouvee et verifiee contre la force brute) : il existe une affectation optimale ou
chaque groupe prend une de ses min(g, c) meilleures colonnes ; on ne garde que ces colonnes (et les colonnes
d'intersection nulle ne servent jamais).

Aucun tableau indexe par paire de points ; tout est lineaire en points, la table est groupes x clusters.
Tourne avec numpy >= 2.2, scipy, sklearn >= 1.7 et Python >= 3.10 (VM G4) ; aucune decision par assert.
"""
import argparse
from fractions import Fraction
import sys

import numpy as np
from scipy.optimize import linear_sum_assignment

VOID_CLASSES = (0, 1, 52, 99)
THING_CLASSES = (10, 11, 13, 15, 16, 18, 20, 30, 31, 32, 252, 253, 254, 255, 256, 257, 258, 259)
HALF = Fraction(1, 2)


def need(ok, why):
    if not ok:
        raise ValueError(why)


def _frac_text(x):
    return '%d/%d' % (x.numerator, x.denominator)


# --------------------------------------------------------------------------------------------- preparation

def _prepare(truth, pred, void=None):
    """Retire les points void ; bruit predit ramene a -1. Rend (truth, pred, keep)."""
    truth = np.asarray(truth)
    pred = np.asarray(pred)
    need(truth.ndim == 1 and pred.shape == truth.shape, 'verite et prediction : tableaux 1-D de meme longueur')
    need(truth.dtype.kind in 'iu' and pred.dtype.kind in 'iu', 'etiquettes entieres')
    truth = truth.astype(np.int64)
    pred = pred.astype(np.int64)
    if void is None:
        keep = np.ones(len(truth), dtype=bool)
    else:
        void = np.asarray(void)
        need(void.shape == truth.shape and void.dtype == np.bool_, 'void : masque booleen de meme longueur')
        keep = ~void
    truth, pred = truth[keep], pred[keep]
    need(bool(np.all(truth >= -1)), 'verite : etiquettes >= -1 (le void passe par le masque)')
    pred = np.where(pred < 0, -1, pred)
    return truth, pred, keep


class Table(object):
    """Contingence groupes x clusters (entiers), tailles |G| (inliers vrais) et |C| (tous points non void)."""

    def __init__(self, truth, pred):
        self.n = int(len(truth))
        self.groups, gi = np.unique(truth[truth >= 0], return_inverse=True)
        self.clusters, cj = np.unique(pred[pred >= 0], return_inverse=True)
        g, c = len(self.groups), len(self.clusters)
        self.gsize = np.bincount(gi, minlength=g).astype(np.int64)
        self.csize = np.bincount(cj, minlength=c).astype(np.int64)
        gi_all = np.searchsorted(self.groups, truth)
        cj_all = np.searchsorted(self.clusters, pred)
        both = (truth >= 0) & (pred >= 0)
        flat = np.bincount(gi_all[both] * max(c, 1) + cj_all[both], minlength=g * c)
        self.table = flat.reshape(g, c).astype(np.int64)
        self.noise_true = int(np.sum(truth < 0))
        self.noise_pred = int(np.sum(pred < 0))
        self.both = int(np.sum(both))

    def iou(self, i, j):
        n = int(self.table[i, j])
        return Fraction(n, int(self.gsize[i] + self.csize[j]) - n) if n else Fraction(0)


# --------------------------------------------------------------------------------------------- hongrois (M2)

def _hungarian(tab):
    """Affectation un-a-un groupes -> clusters de somme d'IoU maximale (scipy), reduite par M2.
    Rend (paires (i, j) d'intersection non nulle, somme exacte)."""
    g, c = tab.table.shape
    if g == 0 or c == 0:
        return [], Fraction(0)
    need(tab.n < (1 << 26), 'hongrois : au plus 2^26 points (ordre flottant des IoU = ordre exact)')
    union = tab.gsize[:, None] + tab.csize[None, :] - tab.table
    iou = np.where(tab.table > 0, tab.table / np.maximum(union, 1), 0.0)
    width = min(g, c)
    keep = set()
    for i in range(g):
        nz = np.flatnonzero(tab.table[i])
        if len(nz) == 0:
            continue
        # deux IoU distinctes de denominateurs < 2^26 different de plus de 2^-52 : l'ordre flottant est exact
        best = nz[np.argsort(-iou[i, nz], kind='stable')[:width]]
        keep.update(best.tolist())
    if not keep:
        return [], Fraction(0)
    cols = np.array(sorted(keep), dtype=np.int64)
    rows, picks = linear_sum_assignment(iou[:, cols], maximize=True)
    pairs = [(int(i), int(cols[j])) for i, j in zip(rows, picks) if tab.table[i, cols[j]] > 0]
    total = sum((tab.iou(i, j) for i, j in pairs), Fraction(0))
    return pairs, total


def _hungarian_full(tab):
    """Meme probleme sans reduction M2 (controle)."""
    g, c = tab.table.shape
    if g == 0 or c == 0:
        return Fraction(0)
    union = tab.gsize[:, None] + tab.csize[None, :] - tab.table
    iou = np.where(tab.table > 0, tab.table / np.maximum(union, 1), 0.0)
    rows, picks = linear_sum_assignment(iou, maximize=True)
    return sum((tab.iou(int(i), int(j)) for i, j in zip(rows, picks)), Fraction(0))


def _brute(tab):
    """Optimum exact par programme dynamique sur les sous-ensembles de colonnes (petites tables seulement) :
    enumeration complete des affectations partielles, en Fraction, independante de scipy."""
    g, c = tab.table.shape
    need(c <= 12, 'force brute : au plus 12 colonnes')
    iou = [[tab.iou(i, j) for j in range(c)] for i in range(g)]
    best = {0: Fraction(0)}
    for i in range(g):
        nxt = dict(best)  # le groupe i peut rester non affecte
        for mask, val in best.items():
            for j in range(c):
                if not mask & (1 << j):
                    key = mask | (1 << j)
                    cand = val + iou[i][j]
                    if cand > nxt.get(key, Fraction(-1)):
                        nxt[key] = cand
        best = nxt
    return max(best.values())


def _miou(tab):
    pairs, total = _hungarian(tab)
    g = len(tab.groups)
    per = [Fraction(0)] * g
    for i, j in pairs:
        per[i] = tab.iou(i, j)
    matches = [(int(tab.groups[i]), int(tab.clusters[j]), int(tab.table[i, j]), int(tab.gsize[i]),
                int(tab.csize[j])) for i, j in pairs]
    exact = total / g if g else None
    return exact, per, matches


def miou_hungarian(truth, pred, void=None):
    """mIoU_h (primaire) : moyenne sur les groupes vrais de l'IoU de l'affectation un-a-un de somme maximale ;
    groupe non apparie = 0. Rend (mIoU flottant, IoU par groupe (ordre des etiquettes croissantes), appariements).
    Appariement : (etiquette du groupe, etiquette du cluster, |G inter C|, |G|, |C|). nan s'il n'y a aucun groupe."""
    truth, pred, _ = _prepare(truth, pred, void)
    exact, per, matches = _miou(Table(truth, pred))
    return (float(exact) if exact is not None else float('nan')), [float(x) for x in per], matches


# --------------------------------------------------------------------------------------------- PQ (M1)

def _panoptic(tab, thing=None, fp_min_thing=None, pred=None):
    """PQ, SQ, RQ ; TP ssi 3 n > |G| + |C| (entier, unique par M1). FP : cluster non apparie ; avec la regle
    LiDAR (thing, fp_min_thing), seulement s'il contient au moins fp_min_thing points thing."""
    ii, jj = np.nonzero(3 * tab.table > tab.gsize[:, None] + tab.csize[None, :])
    need(len(set(ii.tolist())) == len(ii) and len(set(jj.tolist())) == len(jj),
         'appariement IoU > 1/2 non unique (contredit M1)')
    tp = len(ii)
    matched = set(jj.tolist())
    c = len(tab.clusters)
    if thing is not None and fp_min_thing is not None:
        cj = np.searchsorted(tab.clusters, pred)
        tcount = np.bincount(cj[(pred >= 0) & thing], minlength=c)
        fp = sum(1 for j in range(c) if j not in matched and tcount[j] >= fp_min_thing)
    else:
        fp = c - tp
    fp_ignored = c - tp - fp
    fn = len(tab.groups) - tp
    iou_sum = sum((tab.iou(int(i), int(j)) for i, j in zip(ii, jj)), Fraction(0))
    sq = iou_sum / tp if tp else Fraction(0)
    rq = Fraction(2 * tp, 2 * tp + fp + fn) if (2 * tp + fp + fn) else Fraction(0)
    g = len(tab.groups)
    out = dict(tp=tp, fp=fp, fn=fn, fp_ignored=fp_ignored, pq=float(sq * rq), sq=float(sq), rq=float(rq),
               pq_exact=_frac_text(sq * rq), sq_exact=_frac_text(sq), rq_exact=_frac_text(rq),
               obj_precision=float(Fraction(tp, tp + fp)) if tp + fp else 0.0,
               obj_recall=float(Fraction(tp, g)) if g else float('nan'),
               m05=float(iou_sum / g) if g else float('nan'), m05_exact=_frac_text(iou_sum / g) if g else None,
               pq_pairs=[(int(tab.groups[i]), int(tab.clusters[j])) for i, j in zip(ii, jj)])
    return out


def panoptic(truth, pred, void=None, thing_mask=None, fp_min_thing=None):
    """Qualite panoptique sans semantique. TP ssi 3|G inter C| > |G| + |C| (test entier). Synthetique : tout
    cluster non apparie est FP. LiDAR : thing_mask (points des classes thing, avant retrait du void) et
    fp_min_thing (50) : un cluster non apparie n'est FP que s'il contient au moins fp_min_thing points thing ;
    les autres sont comptes dans fp_ignored. m05 = somme des IoU > 1/2 / nombre de groupes."""
    truth_c, pred_c, keep = _prepare(truth, pred, void)
    thing = None
    if thing_mask is not None:
        thing_mask = np.asarray(thing_mask)
        need(thing_mask.shape == keep.shape and thing_mask.dtype == np.bool_, 'thing_mask : masque booleen')
        need(fp_min_thing is not None and int(fp_min_thing) >= 1, 'fp_min_thing >= 1 avec thing_mask')
        thing = thing_mask[keep]
    else:
        need(fp_min_thing is None, 'fp_min_thing sans thing_mask')
    return _panoptic(Table(truth_c, pred_c), thing, fp_min_thing, pred_c)


# --------------------------------------------------------------------------------------------- ARI_s, AMI_s

def _ari_s_exact(tab):
    """ARI avec le bruit en singletons des deux cotes, exact (matrice de confusion des paires ordonnees, formule de
    sklearn.metrics.adjusted_rand_score). Chaque point de bruit est sa propre classe : sa cellule vaut 1."""
    n = tab.n
    need(n < (1 << 31), 'ARI_s : au plus 2^31 points (carres exacts en int64)')
    sum_sq = int(np.sum(tab.table * tab.table)) + (n - tab.both)
    sk = int(np.sum(tab.csize * tab.csize)) + tab.noise_pred
    sc = int(np.sum(tab.gsize * tab.gsize)) + tab.noise_true
    tp = sum_sq - n
    fp = sk - sum_sq
    fn = sc - sum_sq
    tn = n * n - fp - fn - sum_sq
    if fn == 0 and fp == 0:
        return Fraction(1)
    return Fraction(2 * (tp * tn - fn * fp), (tp + fn) * (fn + tn) + (tp + fp) * (fp + tn))


def _ari_labels_exact(a, b):
    """ARI exact de deux etiquetages quelconques (chaque etiquette est une classe, bruit compris) : meme formule
    que sklearn.metrics.adjusted_rand_score. Sert aux temoins ARI_nc et ARI restreinte des fixtures F1."""
    a = np.asarray(a, dtype=np.int64)
    b = np.asarray(b, dtype=np.int64)
    n = int(len(a))
    if n == 0:
        return Fraction(1)
    _, ai = np.unique(a, return_inverse=True)
    _, bi = np.unique(b, return_inverse=True)
    _, cells = np.unique(ai * (int(bi.max()) + 1) + bi, return_counts=True)
    sum_sq = int(np.sum(cells * cells))
    sk = int(np.sum(np.bincount(bi) ** 2))
    sc = int(np.sum(np.bincount(ai) ** 2))
    tp, fp, fn = sum_sq - n, sk - sum_sq, sc - sum_sq
    tn = n * n - fp - fn - sum_sq
    if fn == 0 and fp == 0:
        return Fraction(1)
    return Fraction(2 * (tp * tn - fn * fp), (tp + fn) * (fn + tn) + (tp + fp) * (fp + tn))


def _diagnostics(tab):
    """Descriptifs exacts (fixtures F2, F3, F8 du prototype) : mIoU_best (meilleur cluster de chaque groupe, sans
    contrainte un-a-un : recompense la sous-segmentation), purete (part du groupe majoritaire de chaque cluster,
    micro : recompense la sur-segmentation) et purete inverse (part du meilleur cluster de chaque groupe, micro)."""
    g, c = tab.table.shape
    best = Fraction(0)
    if g and c:
        need(tab.n < (1 << 26), 'diagnostics : au plus 2^26 points (ordre flottant des IoU = ordre exact)')
        union = tab.gsize[:, None] + tab.csize[None, :] - tab.table
        iou = np.where(tab.table > 0, tab.table / np.maximum(union, 1), 0.0)
        for i in range(g):
            if tab.table[i].any():
                best += tab.iou(i, int(np.argmax(iou[i])))
    clustered = int(np.sum(tab.csize))
    inl = int(np.sum(tab.gsize))
    pur = Fraction(int(tab.table.max(axis=0).sum()) if g and c else 0, clustered) if clustered else Fraction(0)
    inv = Fraction(int(tab.table.max(axis=1).sum()) if g and c else 0, inl) if inl else Fraction(0)
    return dict(miou_best=float(best / g) if g else float('nan'), miou_best_exact=_frac_text(best / g) if g else None,
                purity=float(pur), purity_exact=_frac_text(pur), inverse_purity=float(inv),
                inverse_purity_exact=_frac_text(inv))


def ari_s(truth, pred, void=None):
    """ARI_s exact (Fraction) : bruit vrai et bruit predit en singletons."""
    truth, pred, _ = _prepare(truth, pred, void)
    return _ari_s_exact(Table(truth, pred))


def singletons(labels):
    """Etiquettes ou chaque point de bruit (< 0) devient une classe a lui seul."""
    labels = np.asarray(labels, dtype=np.int64).copy()
    noise = labels < 0
    base = int(labels.max()) + 1 if (~noise).any() else 0
    labels[noise] = base + np.arange(int(noise.sum()))
    return labels


def ami_s(truth, pred, void=None):
    """AMI_s (descriptif, flottant de sklearn) : bruit en singletons des deux cotes."""
    from sklearn.metrics import adjusted_mutual_info_score
    truth, pred, _ = _prepare(truth, pred, void)
    return float(adjusted_mutual_info_score(singletons(truth), singletons(pred)))


# --------------------------------------------------------------------------------------------- precision, rappel

def _point_pr(groups, matches):
    """Precision n/|C| et rappel n/|G| des paires hongroises, moyennes sur les groupes (0 si non apparie)."""
    by = dict((m[0], m) for m in matches)
    ps, rs = [], []
    for gl in groups:
        m = by.get(int(gl))
        if m is None:
            ps.append(Fraction(0))
            rs.append(Fraction(0))
        else:
            ps.append(Fraction(m[2], m[4]))
            rs.append(Fraction(m[2], m[3]))
    g = len(groups)
    return dict(point_precision=float(sum(ps, Fraction(0)) / g) if g else float('nan'),
                point_recall=float(sum(rs, Fraction(0)) / g) if g else float('nan'),
                point_precision_per_group=[float(x) for x in ps], point_recall_per_group=[float(x) for x in rs])


def point_precision_recall(truth, pred, matches, void=None):
    """Precision et rappel en points des paires hongroises (matches de miou_hungarian), moyennes sur les groupes,
    0 pour un groupe non apparie ; plus precision objet TP / (TP + FP) et rappel objet TP / #groupes (PQ)."""
    truth_c, pred_c, _ = _prepare(truth, pred, void)
    tab = Table(truth_c, pred_c)
    out = _point_pr(tab.groups, matches)
    pq = _panoptic(tab)
    out.update(obj_precision=pq['obj_precision'], obj_recall=pq['obj_recall'])
    return out


# --------------------------------------------------------------------------------------------- oracle m05

def _tree_check(tree, n):
    """Foret valide : enfants en indices, un parent au plus, sans cycle ; enfants disjoints inclus dans le parent.
    Rend (parent, ordre enfants-avant-parents, membres en tableaux)."""
    m = len(tree)
    members = []
    parent = [-1] * m
    for v, node in enumerate(tree):
        mem = np.asarray(node['members'], dtype=np.int64)
        need(mem.ndim == 1 and (len(mem) == 0 or (mem.min() >= 0 and mem.max() < n)), 'membres hors des sites')
        need(len(np.unique(mem)) == len(mem), 'membres en double')
        members.append(mem)
        for ch in node['children']:
            need(isinstance(ch, (int, np.integer)) and 0 <= int(ch) < m and int(ch) != v, 'enfant invalide')
            need(parent[int(ch)] == -1, 'noeud a deux parents')
            parent[int(ch)] = v
    order, state = [], [0] * m
    for r in range(m):
        if parent[r] != -1:
            continue
        stack = [(r, False)]
        while stack:
            v, done = stack.pop()
            if done:
                order.append(v)
                continue
            need(state[v] == 0, 'cycle')
            state[v] = 1
            stack.append((v, True))
            for ch in tree[v]['children']:
                stack.append((int(ch), False))
    need(len(order) == m, 'cycle ou noeud inaccessible')
    inside = np.zeros(n, dtype=bool)
    taken = np.zeros(n, dtype=bool)
    for v in range(m):
        kids = [int(ch) for ch in tree[v]['children']]
        if not kids:
            continue
        inside[members[v]] = True
        for ch in kids:
            mem = members[ch]
            need(bool(inside[mem].all()), 'enfant hors de son parent')
            need(not bool(taken[mem].any()), 'enfants non disjoints')
            taken[mem] = True
        for ch in kids:
            taken[members[ch]] = False
        inside[members[v]] = False
    return parent, order, members


def _oracle(tree, truth, void=None, measure='iou', root_excluded=True):
    need(measure in ('iou', 'count'), 'measure : iou ou count')
    truth = np.asarray(truth, dtype=np.int64)
    n = len(truth)
    keep = np.ones(n, dtype=bool) if void is None else ~np.asarray(void, dtype=bool)
    need(bool(np.all(truth[keep] >= -1)), 'verite : etiquettes >= -1')
    parent, order, members = _tree_check(tree, n)
    groups, gsize = np.unique(truth[keep & (truth >= 0)], return_counts=True)
    gindex = np.full(n, -1, dtype=np.int64)
    sel = keep & (truth >= 0)
    gindex[sel] = np.searchsorted(groups, truth[sel])
    weight, who = [], []
    for v in range(len(tree)):
        mem = members[v]
        mem = mem[keep[mem]]
        best, gl = Fraction(0), None
        if not (root_excluded and parent[v] == -1) and len(mem):
            gi = gindex[mem]
            gi = gi[gi >= 0]
            if len(gi):
                cnt = np.bincount(gi, minlength=len(groups))
                csize = len(mem)
                for i in np.flatnonzero(3 * cnt > gsize + csize).tolist():
                    x = Fraction(int(cnt[i]), int(gsize[i]) + csize - int(cnt[i]))
                    if x > best:
                        best, gl = x, int(groups[i])
        weight.append(best if measure == 'iou' or gl is None else Fraction(1))
        who.append(gl)
    f = [Fraction(0)] * len(tree)
    take = [False] * len(tree)
    for v in order:  # enfants avant parents
        below = sum((f[int(c)] for c in tree[v]['children']), Fraction(0))
        if weight[v] > below:
            f[v], take[v] = weight[v], True
        else:
            f[v] = below
    total = sum((f[r] for r in range(len(tree)) if parent[r] == -1), Fraction(0))
    chosen, stack = [], [r for r in range(len(tree)) if parent[r] == -1]
    while stack:
        v = stack.pop()
        if take[v]:
            chosen.append(v)
        else:
            stack.extend(int(c) for c in tree[v]['children'])
    g = len(groups)
    return (total / g if g else Fraction(0)), sorted(chosen), dict((v, who[v]) for v in chosen)


def oracle_m05(clusters_as_tree, truth, void=None, measure='iou', root_excluded=True):
    """Oracle exact de m05 sur un arbre condense : max sur les antichaines (racines exclues) de la somme des IoU
    > 1/2 (chaque bloc apparie a son groupe unique, M1), divise par le nombre de groupes ; programme dynamique
    f(v) = max(w(v), somme des f(enfants)). clusters_as_tree : liste de noeuds
    {'members': indices des sites du bloc (cluster a sa naissance), 'children': [indices de noeuds]} ; un noeud
    sans parent est une racine. measure='count' : nombre de groupes apparies a IoU > 1/2, divise par le nombre de
    groupes (variante). Rend une Fraction."""
    return _oracle(clusters_as_tree, truth, void, measure, root_excluded)[0]


# --------------------------------------------------------------------------------------------- MAP

def map_rows(truth, map_labels, pred, void=None):
    """Lignes MAP (SPEC par. 3.1) : niveau de Bayes = mIoU_h des etiquettes MAP contre la verite ; mIoU_h de la
    prediction contre les etiquettes MAP (seconde verite : groupes = etiquettes MAP >= 0 presentes, -1 = bruit) ;
    regret = niveau de Bayes - mIoU_h de la prediction (peut etre negatif, F7 : le MAP n'est pas un plafond)."""
    truth_c, map_c, _ = _prepare(truth, map_labels, void)
    _, pred_c, _ = _prepare(truth, pred, void)
    bayes, _, _ = _miou(Table(truth_c, map_c))
    method, _, _ = _miou(Table(truth_c, pred_c))
    second, _, _ = _miou(Table(map_c, pred_c))
    nan = float('nan')
    return dict(bayes_level=float(bayes) if bayes is not None else nan,
                miou_h=float(method) if method is not None else nan,
                miou_h_vs_map=float(second) if second is not None else nan,
                regret=float(bayes - method) if bayes is not None and method is not None else nan,
                map_groups=int(len(np.unique(map_c[map_c >= 0]))))


def bayes_stratum(level):
    """Strate de Bayes (SPEC par. 3.2) : '>=0.97', '[0.90,0.97)' ou '<0.90'."""
    if level >= 0.97:
        return '>=0.97'
    if level >= 0.90:
        return '[0.90,0.97)'
    return '<0.90'


# --------------------------------------------------------------------------------------------- scores

def scores(truth, pred, void=None, thing_mask=None, fp_min_thing=None, map_labels=None, with_ami=True):
    """Tout ce que publie le par. 3.1 pour une prediction : mIoU_h (primaire) et IoU par groupe ; PQ, SQ, RQ, TP, FP,
    FN (gardes) ; ARI_s exact (garde) ; precision et rappel objets ; precision et rappel en points des paires
    hongroises ; m05 ; nombre de clusters, part de bruit ; AMI_s, mIoU_best, purete et purete inverse
    (descriptifs, jamais seuls) ; lignes MAP si map_labels."""
    truth_c, pred_c, keep = _prepare(truth, pred, void)
    tab = Table(truth_c, pred_c)
    exact, per, matches = _miou(tab)
    thing = None
    if thing_mask is not None:
        thing_mask = np.asarray(thing_mask)
        need(thing_mask.shape == keep.shape and thing_mask.dtype == np.bool_, 'thing_mask : masque booleen')
        need(fp_min_thing is not None and int(fp_min_thing) >= 1, 'fp_min_thing >= 1 avec thing_mask')
        thing = thing_mask[keep]
    else:
        need(fp_min_thing is None, 'fp_min_thing sans thing_mask')
    out = dict(points=tab.n, groups=int(len(tab.groups)), clusters=int(len(tab.clusters)),
               miou_h=float(exact) if exact is not None else float('nan'),
               miou_h_exact=_frac_text(exact) if exact is not None else None,
               iou_h_per_group=[float(x) for x in per], hungarian_matches=matches)
    out.update(_panoptic(tab, thing, fp_min_thing, pred_c))
    ari = _ari_s_exact(tab)
    out.update(ari_s=float(ari), ari_s_exact=_frac_text(ari))
    out.update(_point_pr(tab.groups, matches))
    out.update(_diagnostics(tab))
    out.update(noise_pred=float(Fraction(tab.noise_pred, tab.n)) if tab.n else float('nan'),
               noise_true=float(Fraction(tab.noise_true, tab.n)) if tab.n else float('nan'))
    if with_ami:
        from sklearn.metrics import adjusted_mutual_info_score
        out['ami_s'] = float(adjusted_mutual_info_score(singletons(truth_c), singletons(pred_c)))
    if map_labels is not None:
        out['map'] = map_rows(truth, map_labels, pred, void)
    return out


# --------------------------------------------------------------------------------------------- LiDAR

def lidar_truth(raw, min_points=50):
    """Verite LiDAR R0 depuis les cles SemanticKITTI sem | inst << 16 : instances thing (inst > 0) d'au moins
    min_points points non void (groupes 0..), le reste -1 ; masques void et thing ; cles des instances."""
    raw = np.asarray(raw, dtype=np.int64)
    sem, inst = raw & 0xFFFF, raw >> 16
    void = np.isin(sem, VOID_CLASSES)
    thing = np.isin(sem, THING_CLASSES)
    label = np.where(thing & (inst > 0) & ~void, raw, -1)
    keys, counts = np.unique(label[label >= 0], return_counts=True)
    keys = keys[counts >= min_points]
    truth = np.full(len(raw), -1, dtype=np.int64)
    sel = np.isin(label, keys)
    truth[sel] = np.searchsorted(keys, label[sel])
    return truth, void, thing, [int(k) for k in keys.tolist()]


def lidar_frame(raw, pred, min_points=50, fp_min_thing=50, touch_min=5, share=Fraction(1, 10), with_ami=False):
    """Mesures LiDAR d'une trame (SPEC par. 3.1 ; controles du par. 4 du rapport `lidar`) : scores (mIoU_h, PQ_th avec
    FP = cluster non apparie d'au moins fp_min_thing points thing) ; clusters : total, d'au moins min_points
    points, touchant les things (>= touch_min points thing) ; par instance : cle, taille, IoU hongroise (0 si non
    appariee), fragments (clusters qui en couvrent >= share), part hors de son meilleur cluster ; par cluster :
    instances couvertes a >= share chacune (fusions). Tailles hors void."""
    truth, void, thing, keys = lidar_truth(raw, min_points)
    out = scores(truth, pred, void=void, thing_mask=thing, fp_min_thing=fp_min_thing, with_ami=with_ami)
    truth_c, pred_c, keep = _prepare(truth, pred, void)
    tab = Table(truth_c, pred_c)
    thing_c = thing[keep]
    cj = np.searchsorted(tab.clusters, pred_c)
    c = len(tab.clusters)
    tcount = np.bincount(cj[(pred_c >= 0) & thing_c], minlength=c)
    out['clusters_min_points'] = int(np.sum(tab.csize >= min_points))
    out['clusters_touching_things'] = int(np.sum(tcount >= touch_min))
    rows = []
    for i in range(len(tab.groups)):
        g = int(tab.gsize[i])
        cover = tab.table[i]
        frag = int(np.sum(cover * share.denominator >= share.numerator * g)) if c else 0
        best = int(cover.max()) if c else 0
        rows.append(dict(key=keys[int(tab.groups[i])], points=g, iou_h=out['iou_h_per_group'][i], fragments=frag,
                         outside_best=float(Fraction(g - best, g))))
    out['instances'] = rows
    fusions = []
    for j in range(c):
        col = tab.table[:, j]
        cov = [i for i in np.flatnonzero(col).tolist()
               if int(col[i]) * share.denominator >= share.numerator * int(tab.gsize[i])]
        if len(cov) >= 2:
            fusions.append(dict(cluster=int(tab.clusters[j]), instances=[keys[int(tab.groups[i])] for i in cov]))
    out['fusions'] = fusions
    return out


# --------------------------------------------------------------------------------------------- self-test (F15)

def self_test(verbose=False):
    """Fixtures F1 a F11 de fixtures_metriques.py (valeurs exactes), M2 contre la force brute, oracle m05 contre
    l'enumeration des antichaines, ARI_s exact contre sklearn. Rend la liste des ecarts (vide si conforme)."""
    from sklearn.metrics import adjusted_rand_score
    fails = []

    def eq(name, got, want, tol=None):
        if tol is None:
            ok = got == want
        else:
            ok = abs(float(got) - float(want)) <= tol
        if verbose:
            print('  %-60s %-14s %-14s %s' % (name, str(got)[:14], str(want)[:14], 'ok' if ok else 'ECHEC'))
        if not ok:
            fails.append('%s : obtenu %s, attendu %s' % (name, got, want))

    def sc(truth, pred, **kw):
        return scores(np.array(truth), np.array(pred), **kw)

    def fr(text):
        return Fraction(text) if text is not None else None

    # F1 : un groupe entier en bruit. ARI_s = 4/7 ; mIoU_h = 1/2.
    s = sc([0, 0, 1, 1], [0, 0, -1, -1])
    eq('F1 ari_s = 4/7', fr(s['ari_s_exact']), Fraction(4, 7))
    eq('F1 miou_h = 1/2', fr(s['miou_h_exact']), Fraction(1, 2))
    eq('F1 ari_nc (bruit = une classe) = 1', _ari_labels_exact([0, 0, 1, 1], [0, 0, -1, -1]), Fraction(1))
    eq('F1 ari restreinte aux inliers clusterises = 1', _ari_labels_exact([0, 0], [0, 0]), Fraction(1))
    # F2 : sous-segmentation (deux groupes de 4 fusionnes) : mIoU_h = 1/4, PQ = 0.
    s = sc([0] * 4 + [1] * 4, [0] * 8)
    eq('F2 miou_h = 1/4', fr(s['miou_h_exact']), Fraction(1, 4))
    eq('F2 pq = 0', fr(s['pq_exact']), Fraction(0))
    eq('F2 miou_best = 1/2 (double compte)', fr(s['miou_best_exact']), Fraction(1, 2))
    eq('F2 purete inverse = 1', fr(s['inverse_purity_exact']), Fraction(1))
    # F3 : sur-segmentation en moities : mIoU_h = 1/2, PQ = 0 (IoU 1/2 n'est pas > 1/2).
    s = sc([0] * 4 + [1] * 4, [0, 0, 1, 1, 2, 2, 3, 3])
    eq('F3 miou_h = 1/2', fr(s['miou_h_exact']), Fraction(1, 2))
    eq('F3 pq = 0', fr(s['pq_exact']), Fraction(0))
    eq('F3 tp = 0', s['tp'], 0)
    eq('F3 miou_best = 1/2', fr(s['miou_best_exact']), Fraction(1, 2))
    eq('F3 purete = 1', fr(s['purity_exact']), Fraction(1))
    # F4 : clusters parasites faits de bruit vrai : mIoU_h = 1 (aveugle), PQ = 2/3 ; ARI_s < 1, temoin = 1.
    truth = [0] * 4 + [1] * 4 + [-1] * 4
    s = sc(truth, [0] * 4 + [1] * 4 + [2, 2, 3, 3])
    eq('F4 miou_h = 1', fr(s['miou_h_exact']), Fraction(1))
    eq('F4 pq = 2/3', fr(s['pq_exact']), Fraction(2, 3))
    eq('F4 fp = 2', s['fp'], 2)
    eq('F4 ari_s < 1', fr(s['ari_s_exact']) < 1, True)
    s0 = sc(truth, [0] * 4 + [1] * 4 + [-1] * 4)
    eq('F4 temoin sans parasites : ari_s = 1', fr(s0['ari_s_exact']), Fraction(1))
    # F5 : tout en bruit : 0 partout.
    s = sc([0] * 4 + [1] * 4 + [-1] * 2, [-1] * 10)
    eq('F5 miou_h = 0', fr(s['miou_h_exact']), Fraction(0))
    eq('F5 pq = 0', fr(s['pq_exact']), Fraction(0))
    eq('F5 ari_s = 0', fr(s['ari_s_exact']), Fraction(0))
    # F6 (gravee) : G1 = 7 points (5 dans C1, 2 dans C2), G2 = 2 points (dans C1). La paire PQ G1-C1 vaut 5/9 > 1/2,
    # l'affectation de somme maximale est croisee : 2/7 + 2/7 = 4/7 > 5/9. mIoU_h = 2/7 ; tp = 1.
    s = sc([0] * 7 + [1] * 2, [0] * 5 + [1] * 2 + [0] * 2)
    eq('F6 miou_h = 2/7 (somme croisee 4/7 / 2)', fr(s['miou_h_exact']), Fraction(2, 7))
    eq('F6 tp = 1', s['tp'], 1)
    eq('F6 sq = 5/9', fr(s['sq_exact']), Fraction(5, 9))
    # F7 : le MAP n'est pas un plafond de mIoU : MAP (tout A) 2/5 ; regle b -> B 5/12 ; regret -1/60.
    truth = [0] * 8 + [1] * 2
    eq('F7 miou_h MAP = 2/5', fr(sc(truth, [0] * 10)['miou_h_exact']), Fraction(2, 5))
    eq('F7 miou_h regle b->B = 5/12', fr(sc(truth, [0] * 4 + [1] * 6)['miou_h_exact']), Fraction(5, 12))
    rows = map_rows(np.array(truth), np.array([0] * 10), np.array([0] * 4 + [1] * 6))
    eq('F7 regret = 2/5 - 5/12 = -1/60', rows['regret'], float(Fraction(-1, 60)), tol=1e-15)
    eq('F7 niveau de Bayes = 2/5', rows['bayes_level'], 0.4, tol=1e-15)
    # F8 : solutions triviales : singletons -> mIoU_h 1/5 ; un seul cluster -> 1/4.
    truth = [0] * 5 + [1] * 5
    s = sc(truth, list(range(10)))
    eq('F8a singletons : miou_h = 1/5', fr(s['miou_h_exact']), Fraction(1, 5))
    eq('F8a singletons : purete = 1', fr(s['purity_exact']), Fraction(1))
    s = sc(truth, [0] * 10)
    eq('F8b un cluster : miou_h = 1/4', fr(s['miou_h_exact']), Fraction(1, 4))
    eq('F8b un cluster : purete inverse = 1', fr(s['inverse_purity_exact']), Fraction(1))
    # F9 : partition parfaite, bruit laisse en bruit : mIoU_h = PQ = ARI_s = 1.
    s = sc([0, 0, 1, 1, -1, -1], [5, 5, 7, 7, -1, -1])
    eq('F9 miou_h = 1', fr(s['miou_h_exact']), Fraction(1))
    eq('F9 pq = 1', fr(s['pq_exact']), Fraction(1))
    eq('F9 ari_s = 1', fr(s['ari_s_exact']), Fraction(1))
    # F10 : void retire de tout calcul, meme dans un cluster.
    void = np.array([False, False, True, True, False, False])
    s = sc([0, 0, 0, 0, 1, 1], [0, 0, 0, 0, 1, 1], void=void)
    eq('F10 void : miou_h = 1', fr(s['miou_h_exact']), Fraction(1))
    s = sc([0, 0, 0, 0, 1, 1], [0, 0, 2, 2, 1, 1], void=void)
    eq('F10 void : cluster tout void disparu', s['clusters'], 2)
    # F11 : regle FP LiDAR. Cluster 1 sans point thing : ignore ; cluster 2 (1 point thing) : FP a fp_min_thing 1.
    truth = [0, 0, 0, -1, -1, -1, -1]
    pred = [0, 0, 0, 1, 1, 1, 2]
    thing = np.array([True, True, True, False, False, False, True])
    s = sc(truth, pred, thing_mask=thing, fp_min_thing=1)
    eq('F11 fp = 1', s['fp'], 1)
    eq('F11 fp_ignored = 1', s['fp_ignored'], 1)
    eq('F11 pq = 2/3', fr(s['pq_exact']), Fraction(2, 3))
    s = sc(truth, pred, thing_mask=thing, fp_min_thing=2)
    eq('F11 seuil 2 : fp = 0', s['fp'], 0)
    eq('F11 seuil 2 : pq = 1', fr(s['pq_exact']), Fraction(1))
    # precision et rappel en points (F6) : G1 apparie a C2 (2/2, 2/7), G2 a C1 (2/7, 2/2).
    s = sc([0] * 7 + [1] * 2, [0] * 5 + [1] * 2 + [0] * 2)
    eq('PR F6 precision = (1 + 2/7) / 2', s['point_precision'], float(Fraction(9, 14)), tol=1e-15)
    eq('PR F6 rappel = (2/7 + 1) / 2', s['point_recall'], float(Fraction(9, 14)), tol=1e-15)
    eq('PR F6 precision objet = 1/2', s['obj_precision'], 0.5, tol=0)
    # M2 contre la force brute exacte, et contre le hongrois sans reduction ; ARI_s exact contre sklearn.
    rng = np.random.default_rng(20261004)
    bad_m2 = bad_full = bad_ari = tables = 0
    for _ in range(1500):
        n = int(rng.integers(6, 50))
        g = int(rng.integers(1, 6))
        c = int(rng.integers(1, 8))
        truth = rng.integers(-1, g, size=n)
        pred = rng.integers(-1, c, size=n)
        t, p, _ = _prepare(truth, pred)
        tab = Table(t, p)
        ari = _ari_s_exact(tab)
        if abs(float(ari) - adjusted_rand_score(singletons(t), singletons(p))) > 1e-12:
            bad_ari += 1
        if abs(float(_ari_labels_exact(t, p)) - adjusted_rand_score(t, p)) > 1e-12:
            bad_ari += 1
        if tab.table.size == 0:
            continue
        tables += 1
        _, total = _hungarian(tab)
        if total != _brute(tab):
            bad_m2 += 1
        if total != _hungarian_full(tab):
            bad_full += 1
    eq('M2 : ecarts a la force brute (%d tables)' % tables, bad_m2, 0)
    eq('M2 : ecarts au hongrois sans reduction', bad_full, 0)
    eq('ARI_s et ARI_nc exacts contre sklearn (1500 tirages)', bad_ari, 0)
    eq('M2 : planchers (>= 1000 tables)', tables >= 1000, True)
    # oracle m05 : programme dynamique contre l'enumeration des antichaines, arbres aleatoires emboites.
    bad_or = trees = 0
    for _ in range(300):
        n = int(rng.integers(4, 16))
        truth = rng.integers(-1, 3, size=n)
        tree = _random_tree(rng, n)
        trees += 1
        for measure in ('iou', 'count'):
            got = oracle_m05(tree, truth, measure=measure)
            if got != _oracle_brute(tree, truth, measure):
                bad_or += 1
    eq('oracle m05 : ecarts a l enumeration (%d arbres x 2)' % trees, bad_or, 0)
    # oracle m05 grave : racine {0..5}, enfants {0,1,2} et {3,4,5,6}... (arbre a deux etages, racine exclue).
    truth = np.array([0, 0, 0, 1, 1, 1, -1, -1])
    tree = [dict(members=list(range(8)), children=[1, 2]), dict(members=[0, 1, 2, 6], children=[3, 4]),
            dict(members=[3, 4, 5, 7], children=[]), dict(members=[0, 1], children=[]),
            dict(members=[2, 6], children=[])]
    # noeud 1 : G0 3/4 ; noeud 2 : G1 3/4 ; noeud 3 : G0 2/3 ; somme 3/2 sur 2 groupes = 3/4.
    eq('oracle m05 grave = 3/4', oracle_m05(tree, truth), Fraction(3, 4))
    eq('oracle m05 grave (count) = 1', oracle_m05(tree, truth, measure='count'), Fraction(1))
    eq('oracle m05 grave, racine admise = 3/4', oracle_m05(tree, truth, root_excluded=False), Fraction(3, 4))
    return fails


def _random_tree(rng, n):
    """Arbre condense aleatoire : racine = tous les sites ; chaque noeud se coupe en 0, 2 ou 3 parties disjointes
    (des sites peuvent sortir : points devenus bruit)."""
    tree = [dict(members=list(range(n)), children=[])]
    stack = [0]
    while stack:
        v = stack.pop()
        mem = list(tree[v]['members'])
        if len(mem) < 2 or rng.random() < 0.3:
            continue
        rng.shuffle(mem)
        parts = int(rng.integers(2, 4))
        cuts = sorted(rng.choice(np.arange(1, len(mem) + 1), size=min(parts, len(mem)), replace=False).tolist())
        start = 0
        for cut in cuts:
            piece = sorted(mem[start:cut])
            start = cut
            if not piece or rng.random() < 0.15:
                continue
            tree.append(dict(members=piece, children=[]))
            tree[v]['children'].append(len(tree) - 1)
            stack.append(len(tree) - 1)
    return tree


def _oracle_brute(tree, truth, measure):
    """Enumeration de toutes les antichaines (racines exclues) ; IoU exactes en Fraction par ensembles, sans M1 :
    chaque groupe prend le meilleur bloc choisi d'IoU > 1/2."""
    m = len(tree)
    parent = [-1] * m
    for v, node in enumerate(tree):
        for ch in node['children']:
            parent[ch] = v

    def rec(v):
        opts = [frozenset()]
        for ch in tree[v]['children']:
            sub = rec(ch)
            opts = [a | b for a in opts for b in sub]
        if parent[v] != -1:
            opts.append(frozenset([v]))
        return opts
    families = [frozenset()]
    for r in range(m):
        if parent[r] == -1:
            sub = rec(r)
            families = [a | b for a in families for b in sub]
    groups = sorted(set(int(x) for x in truth if x >= 0))
    gsets = dict((gl, set(i for i in range(len(truth)) if truth[i] == gl)) for gl in groups)
    best = Fraction(0)
    for chosen in families:
        total = Fraction(0)
        for gl in groups:
            vals = []
            for v in chosen:
                cset = set(tree[v]['members'])
                inter = len(gsets[gl] & cset)
                x = Fraction(inter, len(gsets[gl] | cset)) if inter else Fraction(0)
                if x > HALF:
                    vals.append(x)
            if vals:
                total += max(vals) if measure == 'iou' else 1
        best = max(best, total)
    return best / len(groups) if groups else Fraction(0)


def main():
    parser = argparse.ArgumentParser(description='Metriques E1 (sortie plate v11)')
    parser.add_argument('--self-test', action='store_true', help='fixture F15 : code 0 si conforme, 3 sinon')
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()
    if not args.self_test:
        parser.print_help()
        return 2
    try:
        fails = self_test(verbose=args.verbose)
    except Exception as error:  # noqa: BLE001 - une exception de la porte est un echec, code 3
        fails = ['exception : %r' % (error,)]
    for line in fails:
        print('ECHEC', line)
    print('points_flat_metrics self-test', 'conforme' if not fails else 'NON CONFORME (%d)' % len(fails))
    return 0 if not fails else 3


if __name__ == '__main__':
    sys.exit(main())
