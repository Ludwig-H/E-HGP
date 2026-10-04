#!/usr/bin/env python3
"""Metriques d'un clustering plat avec bruit contre une verite terrain (v11, protocole de mesure, 4 octobre 2026).

Conventions des etiquettes (entiers) :
  verite     : groupe >= 0, bruit vrai -1 (compte contre un cluster qui l'avale), ligne ignoree -2 (void LiDAR :
               retiree de TOUT calcul, comme l'evaluation panoptique de SemanticKITTI) ;
  prediction : cluster >= 0, bruit predit < 0.

Toutes les decisions d'appariement sont entieres :
  IoU(G, C) = n / (|G| + |C| - n), |C| = points non ignores du cluster, bruit vrai compris ;
  IoU > 1/2  <=>  3 n > |G| + |C|  (test entier, aucune egalite flottante) ;
  l'affectation un-a-un de somme d'IoU maximale passe par scipy (hongrois en flottant) puis sa somme est
  recalculee en Fraction ; un certificat d'optimalite (borne duale) est verifie a 1e-12 pres.

Metriques rendues par `scores` (une scene) :
  miou_h        IoU moyenne par groupe vrai, affectation un-a-un de somme maximale (groupe non affecte : 0)  [primaire]
  miou_best     IoU moyenne du meilleur cluster de chaque groupe, sans contrainte un-a-un (diagnostic)
  pq, sq, rq    qualite panoptique (appariement IoU > 1/2, unique par construction), rq = F1 objet
  tp, fp, fn    comptes objets ; fp_ign = clusters non apparies ignores par le filtre `fp_ignore` (LiDAR)
  obj_p, obj_r  precision et rappel objets (tp / (tp + fp), tp / (tp + fn))
  pur, inv_pur  purete (part du groupe majoritaire dans chaque cluster, micro) et purete inverse (part du meilleur
                cluster dans chaque groupe, micro) ; f_pur leur moyenne harmonique
  ari_nc, ami_nc  bruit = une classe (convention naive : recompense l'etiquetage d'un groupe entier en bruit)
  ari_s, ami_s    bruit vrai et predit en singletons (convention v10 ARI_s)
  ari_in          ARI restreinte aux points inliers vrais ET clusterises (recompense l'abstention ; diagnostic)
  clusters, noise_pred, noise_true, coverage (part des inliers vrais clusterises), absorbed (part du bruit vrai
  clusterise), over_seg / under_seg (comptes, seuil t points)
"""
from fractions import Fraction
import math

import numpy as np
from scipy.optimize import linear_sum_assignment
from sklearn.metrics import adjusted_mutual_info_score, adjusted_rand_score


def need(ok, why):
    if not ok:
        raise ValueError(why)


def _clean(truth, pred):
    truth = np.asarray(truth, dtype=np.int64)
    pred = np.asarray(pred, dtype=np.int64)
    need(truth.shape == pred.shape and truth.ndim == 1, 'shapes')
    keep = truth != -2
    truth, pred = truth[keep], pred[keep]
    pred = np.where(pred < 0, -1, pred)
    need(bool(np.all(truth >= -1)), 'verite : etiquettes >= -2')
    return truth, pred


def contingency(truth, pred):
    """Groupes, clusters, table n_ij (int64), |G_i| (inliers vrais), |C_j| (tous points non ignores du cluster)."""
    groups = np.unique(truth[truth >= 0])
    clusters = np.unique(pred[pred >= 0])
    gi = np.searchsorted(groups, truth)
    cj = np.searchsorted(clusters, pred)
    both = (truth >= 0) & (pred >= 0)
    table = np.zeros((len(groups), len(clusters)), dtype=np.int64)
    np.add.at(table, (gi[both], cj[both]), 1)
    gsize = np.bincount(gi[truth >= 0], minlength=len(groups)).astype(np.int64)
    csize = np.bincount(cj[pred >= 0], minlength=len(clusters)).astype(np.int64)
    return groups, clusters, table, gsize, csize


def iou_frac(n, g, c):
    return Fraction(int(n), int(g + c - n)) if n else Fraction(0)


def hungarian_iou(table, gsize, csize):
    """Affectation un-a-un groupes -> clusters maximisant la somme des IoU. Rend (paires, somme exacte)."""
    if table.size == 0:
        return [], Fraction(0)
    # colonnes utiles : pour chaque groupe ses min(g, C) meilleures colonnes (un echange vers une colonne libre ne
    # diminue pas la somme) -- reduit la matrice LiDAR (milliers de clusters) sans changer l'optimum
    g, c = table.shape
    union = gsize[:, None] + csize[None, :] - table
    iou = np.where(table > 0, table / np.maximum(union, 1), 0.0)
    keep = set()
    width = min(g, c)
    for i in range(g):
        nz = np.flatnonzero(table[i])
        if len(nz) == 0:
            continue
        best = nz[np.argsort(-iou[i, nz], kind='stable')[:width]]
        keep.update(best.tolist())
    if not keep:
        return [], Fraction(0)
    cols = np.array(sorted(keep), dtype=np.int64)
    sub = iou[:, cols]
    rows, picks = linear_sum_assignment(sub, maximize=True)
    pairs = [(int(i), int(cols[j])) for i, j in zip(rows, picks) if table[i, cols[j]] > 0]
    total = sum((iou_frac(table[i, j], gsize[i], csize[j]) for i, j in pairs), Fraction(0))
    # certificat : somme flottante de l'optimum ~ somme exacte ; borne : somme des maxima par ligne
    need(abs(float(total) - float(sub[rows, picks].sum())) < 1e-9, 'somme hongroise incoherente')
    need(float(total) <= float(iou.max(axis=1).sum()) + 1e-12, 'somme au-dessus de la borne')
    return pairs, total


def pq_match(table, gsize, csize):
    """Paires (i, j) d'IoU > 1/2 (test entier 3 n > |G| + |C|). Unicite verifiee."""
    ii, jj = np.nonzero(3 * table > gsize[:, None] + csize[None, :])
    need(len(set(ii.tolist())) == len(ii) and len(set(jj.tolist())) == len(jj), 'appariement IoU > 1/2 non unique')
    return list(zip(ii.tolist(), jj.tolist()))


def singletons(labels):
    labels = np.asarray(labels).copy()
    noise = labels < 0
    base = int(labels.max()) + 1 if (~noise).any() else 0
    labels[noise] = base + np.arange(int(noise.sum()))
    return labels


def scores(truth, pred, fp_ignore=None, seg_threshold=None, with_ami=True):
    """Toutes les metriques d'une scene. fp_ignore : tableau booleen par ligne (avant retrait des -2) ; un cluster
    non apparie dont plus de la moitie des points non ignores sont marques `fp_ignore` n'est pas compte FP (ex.
    LiDAR : points « stuff » ; convention declaree). seg_threshold : seuil t (points) des comptes de decoupage."""
    raw_truth = np.asarray(truth, dtype=np.int64)
    if fp_ignore is not None:
        fp_ignore = np.asarray(fp_ignore, dtype=bool)[raw_truth != -2]
    truth, pred = _clean(truth, pred)
    groups, clusters, table, gsize, csize = contingency(truth, pred)
    ng, nc = len(groups), len(clusters)
    out = dict(groups=ng, clusters=nc, points=int(len(truth)))
    # mIoU un-a-un et meilleur cluster
    pairs, total = hungarian_iou(table, gsize, csize)
    out['miou_h'] = float(total / ng) if ng else float('nan')
    if ng and nc:
        union = gsize[:, None] + csize[None, :] - table
        best = [max((iou_frac(table[i, j], gsize[i], csize[j]) for j in np.flatnonzero(table[i])),
                    default=Fraction(0)) for i in range(ng)]
        out['miou_best'] = float(sum(best, Fraction(0)) / ng)
        out['iou_best_per_group'] = [float(b) for b in best]
    else:
        out['miou_best'] = 0.0 if ng else float('nan')
        out['iou_best_per_group'] = [0.0] * ng
    per_group = [0.0] * ng
    for i, j in pairs:
        per_group[i] = float(iou_frac(table[i, j], gsize[i], csize[j]))
    out['iou_h_per_group'] = per_group
    # qualite panoptique
    match = pq_match(table, gsize, csize)
    tp = len(match)
    matched_c = set(j for _, j in match)
    fp_ign = 0
    if fp_ignore is not None and nc:
        cj = np.searchsorted(clusters, pred)
        ign_count = np.bincount(cj[(pred >= 0) & fp_ignore], minlength=nc)
        for j in range(nc):
            if j not in matched_c and 2 * ign_count[j] > csize[j]:
                fp_ign += 1
    fp = nc - tp - fp_ign
    fn = ng - tp
    sq = (sum((iou_frac(table[i, j], gsize[i], csize[j]) for i, j in match), Fraction(0)) / tp) if tp else Fraction(0)
    rq = Fraction(2 * tp, 2 * tp + fp + fn) if (2 * tp + fp + fn) else Fraction(1)
    out.update(tp=tp, fp=fp, fn=fn, fp_ign=fp_ign, sq=float(sq), rq=float(rq), pq=float(sq * rq),
               obj_p=float(Fraction(tp, tp + fp)) if tp + fp else 0.0, obj_r=float(Fraction(tp, ng)) if ng else 0.0)
    # puretes (micro)
    clustered = int((pred >= 0).sum())
    out['pur'] = float(Fraction(int(table.max(axis=0).sum()) if nc and ng else 0, clustered)) if clustered else 0.0
    inl = int(gsize.sum())
    out['inv_pur'] = float(Fraction(int(table.max(axis=1).sum()) if nc and ng else 0, inl)) if inl else 0.0
    p, r = out['pur'], out['inv_pur']
    out['f_pur'] = 2 * p * r / (p + r) if p + r > 0 else 0.0
    # ARI / AMI selon trois conventions de bruit
    out['ari_nc'] = float(adjusted_rand_score(truth, pred))
    out['ari_s'] = float(adjusted_rand_score(singletons(truth), singletons(pred)))
    sel = (truth >= 0) & (pred >= 0)
    out['ari_in'] = float(adjusted_rand_score(truth[sel], pred[sel])) if sel.sum() > 1 else float('nan')
    if with_ami:
        out['ami_nc'] = float(adjusted_mutual_info_score(truth, pred))
        out['ami_s'] = float(adjusted_mutual_info_score(singletons(truth), singletons(pred)))
    # bruit, couverture, decoupage
    out['noise_pred'] = float((pred < 0).mean()) if len(pred) else float('nan')
    out['noise_true'] = float((truth < 0).mean()) if len(truth) else float('nan')
    out['coverage'] = float(((truth >= 0) & (pred >= 0)).sum() / max(1, (truth >= 0).sum()))
    out['absorbed'] = float(((truth < 0) & (pred >= 0)).sum() / max(1, (truth < 0).sum()))
    t = seg_threshold if seg_threshold is not None else 1
    out['over_seg'] = int(sum(1 for i in range(ng) if (table[i] >= t).sum() >= 2))
    out['under_seg'] = int(sum(1 for j in range(nc) if (table[:, j] >= t).sum() >= 2))
    return out


PRIMARY = ('miou_h', 'pq', 'rq', 'sq', 'ari_s', 'ami_s', 'f_pur', 'clusters', 'noise_pred')
