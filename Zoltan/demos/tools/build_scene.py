#!/usr/bin/env python3
"""Construit les scènes d'une démo : hiérarchie, branches suivies, récit.

Usage : ``python3 Zoltan/demos/tools/build_scene.py Zoltan/demos/<démo>``

Lit ``<démo>/demo.json`` (trame, objets d'intérêt, sol, méthodes), puis
pour chaque méthode demandée :

1. charge la trame SemanticKITTI (lecture partielle des archives officielles)
   et retire le sol par Patchwork++ aux paramètres v8, ou le garde ;
2. calcule sur la trame **entière** l'arbre de la méthode :

   - ``hdbscan`` : MST d'atteignabilité mutuelle, min_samples = K (point
     compté), min_cluster_size = 1 — l'arbre HDBSCAN complet, non condensé ;
   - ``alpine_bev`` : ALPINE sans sémantique — projection en vue de dessus,
     graphe des k = 32 plus proches voisins, arête si distance < t ; le
     balayage de t donne l'arbre du lien simple de ce graphe (partitions
     exactement emboîtées) ;

3. pour chaque objet d'intérêt (instance de vérité terrain), cherche le
   meilleur nœud de l'arbre (IoU maximal) et prend comme graine son point
   le plus dense dans ce nœud : la branche suivie est la composante de la
   graine quand le niveau croît, elle passe donc par le meilleur nœud ;
4. tourne la scène pour aligner les objets sur les axes (boîte englobante
   d'aire minimale de leur union en vue de dessus), capteur côté y < 0 ;
5. écrit ``data/scene_<méthode>.js`` (coordonnées, ignoré par git) pour le
   lecteur et ``resultats_<méthode>.json`` (nombres seulement, versionné).

La vérité terrain ne sert qu'à choisir et noter les objets ; aucune
hiérarchie n'utilise de sémantique.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components, minimum_spanning_tree
from scipy.spatial import cKDTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
import kitti  # noqa: E402
from ground import ground_mask  # noqa: E402
from hierarchy import FUSED, MATCHED, best_and_first_nodes, best_nodes, mreach_mst, node_members, replay_tracks  # noqa: E402

# Couleurs par rôle, résolues par le lecteur selon le thème (clair ou sombre) :
# 'obj0'..'obj2' = objets suivis A, B, C ; 'fusion' = fusion à tort ; 'text' = neutre.
COLORS = ['obj0', 'obj1', 'obj2']
FUSION = 'fusion'
NEUTRAL = 'text'
EV_R, EV_BOX, EV_REC, EV_PREC, EV_SIZE, EV_MASK, EV_STATE, EV_IOU = 0, 1, 7, 8, 9, 10, 11, 12
SWEEP_K = list(range(1, 11))
NUM = {2: 'deux', 3: 'trois'}
LV = '\u2063LV\u2063'  # marque du niveau dans une légende, résolue après tri des événements
# Seuils t de l'article ALPINE (boîtes « web », t = petit côté ; alpine_semantickitti.py, commit 15d7fb3)
ALPINE_T = [(0.61, 'vélo'), (0.94, 'piéton'), (1.8, 'voiture')]


def fr(v: float, d: int = 2) -> str:
    return f'{v:.{d}f}'.replace('.', ',')


def fl(v: float) -> str:
    """Niveau en mètres : trois décimales sous 1 m, pour distinguer des événements proches."""
    return fr(v, 3 if v < 1 else 2)


def nfmt(v: int) -> str:
    return f'{v:,}'.replace(',', ' ')


def pc(v: float) -> str:
    return '< 1 %' if 0 < v < 0.005 else f'{round(100 * v)} %'


# ------------------------------------------------------------------ données
def load(spec):
    xyzi, label, digest = kitti.load_frame(spec['seq'], spec['frame'], spec.get('sha256'), spec.get('labels_sha256'))
    ground = spec.get('ground', 'patchwork_v8')
    if ground == 'patchwork_v8':
        keep = ground_mask(xyzi) != 1
    elif ground == 'none':
        keep = np.ones(len(xyzi), bool)
    else:
        raise ValueError(ground)
    return xyzi[keep, :3].astype(np.float64), label[keep], len(xyzi), digest


def select_object(sel, X, L):
    sem = (L & 0xFFFF).astype(np.int64)
    inst = (L >> 16).astype(np.int64)
    if 'inst' in sel:
        idx = np.flatnonzero((sem == sel['sem']) & (inst == sel['inst']))
    else:
        # pseudo-objet d'une classe « stuff » : composante (lien 0,3 m) la plus proche de `near`
        near = np.asarray(sel['near'], float)
        cand = np.flatnonzero((sem == sel['stuff']) & (np.linalg.norm(X - near, axis=1) <= sel.get('radius', 2.0)))
        if len(cand) == 0:
            raise ValueError(f'aucun point pour {sel}')
        tree = cKDTree(X[cand])
        _, comp = connected_components(tree.sparse_distance_matrix(tree, 0.3), directed=False)
        j = int(np.argmin(np.linalg.norm(X[cand] - near, axis=1)))
        idx = cand[comp == comp[j]]
    if len(idx) == 0:
        raise ValueError(f'objet vide : {sel}')
    return idx


def align(X, objs_idx):
    """Rotation autour de z (degrés) : aire minimale de la boîte des objets en vue de dessus,
    rangée le long de x, capteur (origine) du côté y < 0."""
    P = np.concatenate([X[i, :2] for i in objs_idx])

    def rotated(deg):
        t = math.radians(deg)
        R = np.array([[math.cos(t), -math.sin(t)], [math.sin(t), math.cos(t)]])
        return P @ R.T

    best = min(np.arange(0.0, 90.0, 0.25), key=lambda d: float(np.prod(np.ptp(rotated(d), axis=0))))
    cands = []
    for q in range(4):
        deg = (best + 90 * q) % 360
        Q = rotated(deg)
        ext = np.ptp(Q, axis=0)
        cands.append((ext[0] >= ext[1] - 1e-9, 0.0 < Q.mean(0)[1], deg))
    ok = [c for c in cands if c[0] and c[1]]
    return float((ok or cands)[0][2])


def rot(X, deg):
    t = math.radians(deg)
    R = np.array([[math.cos(t), -math.sin(t), 0], [math.sin(t), math.cos(t), 0], [0, 0, 1]])
    return X @ R.T


# -------------------------------------------------------------- hiérarchies
def alpine_bev_forest(X, k=32):
    """Arbre du lien simple du graphe kNN 2D symétrisé (union), poids = distance BEV."""
    P = np.ascontiguousarray(X[:, :2])
    n = len(P)
    kk = min(k + 1, n)
    d, j = cKDTree(P).query(P, k=kk, workers=-1)
    rows = np.repeat(np.arange(n), kk)
    cols = j.ravel()
    w = d.ravel()
    keep = rows != cols
    rows, cols, w = rows[keep], cols[keep], w[keep]
    # scipy ignore les poids nuls : décalage infime pour les points superposés en BEV
    G = coo_matrix((w + 1e-9, (rows, cols)), shape=(n, n)).tocsr()
    G = G.maximum(G.T)
    T = minimum_spanning_tree(G).tocoo()
    mst = np.stack([T.row.astype(np.float64), T.col.astype(np.float64), T.data - 1e-9], axis=1)
    return np.zeros(n), mst[np.argsort(mst[:, 2], kind='stable')]


def hierarchy_for(run, X):
    if run['method'] == 'hdbscan':
        return mreach_mst(X, run['K'])
    if run['method'] == 'alpine_bev':
        return alpine_bev_forest(X, run.get('k', 32))
    raise ValueError(run['method'])


def run_tag(run):
    return f"hdbscan_K{run['K']}" if run['method'] == 'hdbscan' else 'alpine_bev'


def run_label(run):
    if run['method'] == 'hdbscan':
        return f"HDBSCAN · min_samples = K = {run['K']} · min_cluster_size = 1"
    return f"ALPINE sans sémantique · vue de dessus, k = {run.get('k', 32)} voisins"


def seeds_for(births, mst, best, first, obj, n_obj):
    """Graine de chaque branche suivie.

    Objet apparié quelque part (IoU > 0,5) : un point de l'objet dans le
    premier nœud apparié ; tous les nœuds appariés forment une chaîne, la
    branche de cette graine les traverse tous. Sinon : un point de l'objet
    dans son meilleur nœud. Dans le nœud, le point né le plus tôt (le plus
    dense ; à égalité, le plus petit indice)."""
    seeds = []
    for o in range(n_obj):
        if first[o] is not None:
            _, e, root = first[o]
        else:
            _, _, e, root = best[o]
        idx = np.flatnonzero(obj == o)
        cand = idx if e < 0 else np.intersect1d(node_members(len(obj), mst, e, root), idx)
        seeds.append(int(cand[np.argmin(births[cand])]))
    return seeds


KEY_KINDS = ('merge', 'absorb', 'match_all')


def key_time(captions, rule, fallback):
    """Instant de l'image fixe « instant clé », au milieu de la légende qui porte le message.

    'merge' : première fusion de deux branches suivies (défaut) ; 'absorb' : première
    absorption de fond par une branche suivie ; 'match_all' : dernier appariement avant
    toute absorption ou fusion (le témoin, où tout est apparié). À défaut, on se rabat
    sur la règle suivante, puis sur ``fallback``."""
    if rule not in KEY_KINDS:
        raise ValueError(f'instant clé inconnu : {rule!r} (attendu : {KEY_KINDS})')
    mid = lambda c: round((c['t0'] + c['t1']) / 2, 4)  # noqa: E731
    first = {kind: next((c for c in captions if c['kind'] == kind), None) for kind in ('merge', 'absorb')}
    if rule == 'match_all':
        stop = min((c['t0'] for c in captions if c['kind'] in ('merge', 'absorb')), default=float('inf'))
        matches = [c for c in captions if c['kind'] == 'match' and c['t0'] < stop]
        if matches:
            return mid(matches[-1])
    order = ('absorb', 'merge') if rule == 'absorb' else ('merge', 'absorb')
    for kind in order:
        if first[kind] is not None:
            return mid(first[kind])
    return round(fallback, 4)


def segments(events, rmax):
    segs = []
    for i, ev in enumerate(events):
        r0 = ev[EV_R]
        r1 = events[i + 1][EV_R] if i + 1 < len(events) else rmax
        if r1 <= r0:
            continue
        st = ev[EV_STATE]
        if segs and segs[-1][2] == st and abs(segs[-1][1] - r0) < 1e-12:
            segs[-1][1] = r1
        else:
            segs.append([r0, r1, st])
    return segs


def state_at(events, r):
    cur = None
    for ev in events:
        if ev[EV_R] <= r:
            cur = ev
        else:
            break
    return cur


def contamination(L, obj, keys, join_o, r, own):
    """Ce que la branche absorbe exactement au niveau r, hors de son objet : une partie d'un
    objet suivi, un autre objet annoté, ou une classe de fond. Les points évalués (non void)
    sont préférés, car eux seuls font baisser l'IoU ; à défaut, tout ce qui est absorbé à r,
    puis tout ce que la branche contient hors objet."""
    evaluated = ~np.isin(L & 0xFFFF, kitti.VOID)
    inside = (join_o == r) & ~own & evaluated
    if not inside.any():
        inside = (join_o == r) & ~own
    if not inside.any():
        inside = (join_o <= r) & ~own
    if not inside.any():
        return 'des points hors objet'
    vals, cnt = np.unique(L[inside], return_counts=True)
    order = np.argsort(-cnt)
    v = int(vals[order[0]])
    sem, inst = v & 0xFFFF, v >> 16
    tracked = obj[inside][L[inside] == v]
    if len(tracked) and tracked[0] >= 0:
        return f'une partie de {keys[int(tracked[0])]}'
    name = kitti.FR.get(kitti.class_name(sem), str(sem))
    if sem in kitti.THING and inst > 0:
        return f'des points d\'un autre objet « {name} » (non suivi)'
    if len(order) > 1 and cnt[order[1]] >= 0.3 * cnt.sum():
        v2 = int(vals[order[1]]) & 0xFFFF
        name2 = kitti.FR.get(kitti.class_name(v2), str(v2))
        if name2 != name:
            return f'des points des classes « {name} » et « {name2} »'
    return f'des points de la classe « {name} »'


# -------------------------------------------------------------------- scène
def build_run(demo_dir, spec, data, run, sweep):
    X, L, n_raw, digest, obj, idx_list, sem = data
    n = len(X)
    objs = spec['objects']
    k = len(objs)
    if k > len(COLORS):
        raise ValueError(f'{k} objets suivis : le lecteur n’a que {len(COLORS)} couleurs d’objet par thème')
    m = [len(i) for i in idx_list]
    tag = run_tag(run)
    sym = 't' if run['method'] == 'alpine_bev' else 'r'
    hdb = run['method'] == 'hdbscan'

    view = dict(spec.get('view', {}))
    view.update(run.get('view', {}))
    deg = view.get('rotation_deg')
    if deg is None:
        deg = align(X, idx_list)
    Y = rot(X, deg)
    allobj = np.concatenate(idx_list)
    lo, hi = Y[allobj].min(0), Y[allobj].max(0)
    shift = np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, float(np.percentile(Y[allobj, 2], 0.5))])
    Y = Y - shift
    sensor_xy = -shift[:2] @ np.eye(2)

    void = np.isin(sem, kitti.VOID)
    births, mst = hierarchy_for(run, X)
    best, first = best_and_first_nodes(births, mst, obj, range(k), void)
    seeds = seeds_for(births, mst, best, first, obj, k)
    r_cap = view.get('r_cap', 4.0)
    join, events = replay_tracks(births, mst, obj, seeds, Y, r_cap, void)

    # plage de niveaux
    key_levels = []
    for o in range(k):
        for pred in (lambda ev: ev[EV_STATE] == FUSED, lambda ev: ev[EV_REC] >= 0.9, lambda ev: ev[EV_STATE] == MATCHED):
            ev = next((e for e in events[o] if pred(e)), None)
            if ev is not None:
                key_levels.append(ev[EV_R])
    first_levels = [b for b in (births[s] for s in seeds) if b > 0] + [e[EV_R] for o in range(k) for e in events[o][1:2]]
    rmin = view.get('rmin') or max(0.01, 0.8 * min(first_levels))
    rmax = view.get('rmax') or min(r_cap, max(1.1 * max(key_levels, default=rmin), 2.0 * rmin))
    if not hdb and view.get('alpine_to_thresholds'):
        rmax = max(rmax, 2.0)
    for o in range(k):
        events[o] = [ev for ev in events[o] if ev[EV_R] <= rmax * 1.0001]
    marks = [{'v': v, 'label': f'{sym} {lab}'} for v, lab in ALPINE_T if not hdb and rmin <= v <= rmax]

    branch_merges, seen = [], set()
    for o in range(k):
        for ev in events[o]:
            grp = tuple(sorted({o} | {q for q in range(k) if ev[EV_MASK] & (1 << q)}))
            if len(grp) > 1 and grp not in seen:
                seen.add(grp)
                branch_merges.append({'r': ev[EV_R], 'objects': list(grp)})
    branch_merges.sort(key=lambda d: d['r'])

    levels = sorted({ev[EV_R] for o in range(k) for ev in events[o]})
    best_common, best_common_r, common_end = 0, None, None
    for r in levels:
        c = sum(1 for o in range(k) if (s := state_at(events[o], r)) is not None and s[EV_STATE] == MATCHED)
        if c > best_common:
            best_common, best_common_r, common_end = c, r, None
        elif c < best_common and best_common_r is not None and common_end is None:
            common_end = r

    # ------------------------------------------------------------ récit
    keys = [ob['key'] for ob in objs]
    names = [f"{ob['key']} ({ob['name']})" for ob in objs]
    best_iou = [best[o][0] for o in range(k)]
    tool = 'HDBSCAN' if hdb else 'ALPINE'
    intervals = {}  # objet -> [[début, fin ou None], ...] où la branche (donc l'objet) est appariée
    for o in range(k):
        cur = None
        for ev in events[o]:
            if ev[EV_STATE] == MATCHED and cur is None:
                cur = [ev[EV_R], None]
                intervals.setdefault(o, []).append(cur)
            elif ev[EV_STATE] != MATCHED and cur is not None:
                cur[1], cur = ev[EV_R], None
    first_match = {o: iv[0][0] for o, iv in intervals.items()}

    def iou_txt(v):
        if 0 < v < 0.005:
            return '< 0,01'
        return fr(v, 3) if 0.495 <= v < 0.505 else fr(v)

    def enum(ks, last=' et '):
        return ks[0] if len(ks) == 1 else ', '.join(ks[:-1]) + last + ks[-1]

    merge_levels = {o: {bm['r'] for bm in branch_merges if o in bm['objects']} for o in range(k)}
    notes = []
    for o in range(k):
        ev = next((e for e in events[o] if e[EV_STATE] == MATCHED), None)
        if ev is not None:
            notes.append((ev[EV_R], f"{sym} = {LV} m — la branche {names[o]} est appariée à l'objet : IoU {iou_txt(ev[EV_IOU])} > 0,5\n"
                                    f"précision {pc(ev[EV_PREC])}, rappel {pc(ev[EV_REC])} ; le critère d'appariement de la qualité panoptique (PQ) est IoU > 0,5.",
                          COLORS[o], 'match'))
        prev_state = None
        for ev in events[o]:
            st, r = ev[EV_STATE], ev[EV_R]
            if prev_state == MATCHED and st != MATCHED and r not in merge_levels[o]:
                what = contamination(L, obj, keys, join[o], r, obj == o)
                inside = [keys[q] for q in range(k) if ev[EV_MASK] & (1 << q)]
                ctx_txt = f", qui contient déjà {enum(inside)}," if inside else ''
                notes.append((r, f"{sym} = {LV} m — la branche {names[o]}{ctx_txt} absorbe {what}\n"
                                 f"{keys[o]} n'est plus apparié : précision {pc(ev[EV_PREC])}, rappel {pc(ev[EV_REC])}, IoU {iou_txt(ev[EV_IOU])}.",
                              FUSION, 'absorb'))
            prev_state = st
        ev = next((e for e in events[o] if e[EV_STATE] == FUSED), None)
        if ev is not None and ev[EV_MASK] == 0 and not (o in first_match and first_match[o] < ev[EV_R]):
            what = contamination(L, obj, keys, join[o], ev[EV_R], obj == o)
            notes.append((ev[EV_R], f"{sym} = {LV} m — la branche {names[o]} absorbe {what}\n"
                                    f"IoU {iou_txt(ev[EV_IOU])}, toujours ≤ 0,5 : précision {pc(ev[EV_PREC])}, rappel {pc(ev[EV_REC])} "
                                    f"(meilleur IoU de tout l'arbre : {iou_txt(best_iou[o])}).",
                          FUSION, 'absorb'))

    prev = []
    for bm in branch_merges:
        grp, r = bm['objects'], bm['r']
        parts = []
        for g in sorted(prev, key=len, reverse=True):
            if set(g) <= set(grp) and not any(set(g) <= set(p) for p in parts):
                parts.append(g)
        parts += [[q] for q in grp if not any(q in p for p in parts)]
        label = ' et '.join('+'.join(keys[q] for q in p) for p in parts)
        def soiled(part):
            """Branche déjà mêlée à du fond : moins de la moitié de ses points évalués
            appartiennent aux objets suivis qu'elle contient."""
            evs = [state_at(events[q], r * (1 - 1e-9)) for q in part]
            if any(e is None for e in evs):
                return False
            return sum(e[EV_PREC] for e in evs) < 0.5
        flags = [soiled(p) for p in parts]
        names_p = ['+'.join(keys[q] for q in p) for p in parts]
        after = {q: state_at(events[q], r) for q in grp}
        still = [q for q in grp if after[q] is not None and after[q][EV_STATE] == MATCHED]
        never = [q for q in grp if best_iou[q] <= 0.5]
        was = [q for q in grp if q not in still and q not in never and q in first_match and first_match[q] < r]
        bits = []
        for q in still:
            others = [keys[x] for x in grp if x != q]
            bits.append(f"{keys[q]} absorbe {enum(others)} et reste apparié (IoU {iou_txt(after[q][EV_IOU])} > 0,5) : "
                        f"la PQ compte {keys[q]} juste.")
        if len(never) == 1:
            q = never[0]
            bits.append(f"{keys[q]} n'est isolé par aucun nœud de l'arbre (meilleur IoU {iou_txt(best_iou[q])}) : "
                        f"aucune extraction {tool} ne peut le retrouver.")
        elif never:
            bits.append(f"{enum([keys[q] for q in never])} ne sont isolés par aucun nœud de l'arbre (meilleurs IoU : "
                        + ', '.join(f'{keys[q]} {iou_txt(best_iou[q])}' for q in never)
                        + f") : aucune extraction {tool} ne peut les retrouver.")
        for q in was:
            a0, a1 = [iv for iv in intervals[q] if iv[0] < r][-1]
            bits.append(f"{keys[q]} était apparié de {sym} = {fl(a0)} à {fl(r if a1 is None else min(a1, r))} m.")
        line2 = ' '.join(bits)
        if all(flags):
            who = f"les branches {label}, déjà mêlées à du fond, fusionnent"
        elif any(flags) and len(parts) == 2:
            dirty, clean = (0, 1) if flags[0] else (1, 0)
            who = f"la branche {names_p[dirty]}, déjà mêlée à du fond, et la branche {names_p[clean]} fusionnent"
        else:
            who = f"les branches {label} fusionnent"
        head = f"{sym} = {LV} m — {who} : un seul cluster pour {NUM.get(len(grp), len(grp))} objets"
        notes.append((r, f"{head}\n{line2}", FUSION, 'merge'))
        prev.append(grp)
    for mk in marks:
        notes.append((mk['v'], f"t = {fr(mk['v'])} m : seuil de l'article ALPINE pour la classe « {mk['label'][2:]} »\n"
                               "Sans sémantique, un seul t vaut pour tous les objets, et le découpage par boîte, qui exige la classe, disparaît.",
                      NEUTRAL, 'mark'))
    notes.sort(key=lambda x: x[0])
    # niveau affiché : trois décimales, quatre si deux événements distincts s'afficheraient pareil
    shown = {}
    for r, *_ in notes:
        shown.setdefault(fl(r), set()).add(r)
    level_label = {r: (fr(r, 4) if len(shown[fl(r)]) > 1 else fl(r)) for r, *_ in notes}
    notes = [(r, txt.replace(LV, level_label[r]), col, kind) for r, txt, col, kind in notes]
    holds = []  # une pause par niveau d'événement, au niveau exact : les panneaux montrent l'événement annoncé
    for r, txt, col, kind in notes:
        if holds and abs(math.log(r) - math.log(holds[-1][0])) < 1e-9:
            holds[-1][1].append((txt, col, kind))
        else:
            holds.append((r, [(txt, col, kind)]))

    timing = spec.get('timing', {})
    t_intro, t_sweep, t_hold, t_sum = timing.get('intro', 4.0), timing.get('sweep', 10.0), timing.get('hold', 3.8), timing.get('summary', 8.0)
    lr0, lr1 = math.log(rmin), math.log(rmax)
    ground_txt = ('sans sol (Patchwork++), aucune sémantique.' if spec.get('ground', 'patchwork_v8') != 'none'
                  else 'avec le sol, aucune sémantique.')
    sched = [(t_intro, rmin)]
    captions = [{'t0': 0.0, 't1': t_intro,
                 'text': spec.get('intro') or ('Vérité terrain SemanticKITTI : '
                                               + ', '.join(f"{ob['key']} = {ob['name']} ({m[o]} points)" for o, ob in enumerate(objs)) + '.') + '\n'
                         f'La hiérarchie, elle, ne voit que la géométrie : {nfmt(n)} points {ground_txt}',
                 'color': NEUTRAL, 'kind': 'intro'}]
    extra = 0.0  # pauses cumulées : vitesse constante en log du niveau
    for r, items in holds:
        if r < rmin or r > rmax:
            continue
        t_arr = t_intro + t_sweep * (math.log(r) - lr0) / (lr1 - lr0) + extra
        hold = t_hold * len(items)
        sched += [(t_arr, r), (t_arr + hold, r)]
        for j, (txt, col, kind) in enumerate(items):
            captions.append({'t0': t_arr + j * t_hold, 't1': t_arr + (j + 1) * t_hold, 'text': txt, 'color': col, 'kind': kind})
        extra += hold
    t_end = t_intro + t_sweep + extra
    sched += [(t_end, rmax), (t_end + 1.0, rmax)]
    t_summary = t_end + 1.0
    key_kind = run.get('key', spec.get('key', 'merge'))
    t_key = key_time(captions, key_kind, t_summary - 1.5)

    # ------------------------------------------------------------ bilan
    def matched_txt(o):
        if best_iou[o] <= 0.5 or o not in intervals:
            return ' — jamais apparié'
        parts = [f'de {sym} = {fl(a0)} à {fl(a1)} m' if a1 is not None else f'dès {sym} = {fl(a0)} m' for a0, a1 in intervals[o]]
        return ' — apparié ' + ', puis '.join(parts)

    lines = [f"{ob['key']} · {ob['name']} : meilleur nœud, IoU {iou_txt(best_iou[o])} à {sym} = {fl(best[o][1])} m" + matched_txt(o)
             for o, ob in enumerate(objs)]
    cut = f"Coupe horizontale (DBSCAN*, ε = r, minPts = {run['K']})" if hdb else 'Seuil unique t (ALPINE sans sémantique)'
    if best_common == 0:
        lines.append(f"{cut} : aucun niveau n'apparie ne serait-ce qu'un des {k} objets.")
    elif best_common == 1:
        lines.append(f"{cut} : au mieux 1 objet sur {k} apparié à un même niveau.")
    elif best_common == k:
        window = (f"pour {sym} ∈ [{fl(best_common_r)} ; {fl(common_end)}[ m" if common_end is not None
                  else f"dès {sym} = {fl(best_common_r)} m")
        lines.append(f"{cut} : les {NUM.get(k, k)} objets sont appariés ensemble {window}.")
    else:
        lines.append(f"{cut} : au mieux {best_common} objets sur {k} appariés à un même niveau.")
    never = [keys[o] for o in range(k) if best_iou[o] <= 0.5]
    always = [keys[o] for o in range(k) if max(sweep['hdbscan'][o]) <= 0.5]
    if never:
        lines.append((f"Aucune extraction (excès de masse EOM, feuilles, coupe) ne peut retrouver {enum(never, ' ou ')} : "
                      "aucun nœud n'atteint une IoU supérieure à 0,5.") if hdb else
                     (f"Aucun seuil t ne retrouve {enum(never, ' ou ')} : aucun nœud de l'arbre en t "
                      "n'atteint une IoU supérieure à 0,5."))
    elif best_common < k:
        lines.append(f"Aucun niveau unique ne sépare les {k} objets : il faut un niveau par objet.")
    else:
        lines.append(f"Un même niveau de l'arbre sépare les {NUM.get(k, k)} objets (niveau choisi avec la vérité terrain ; "
                     "la sélection automatique EOM n'est pas évaluée ici).")
    for o in range(k):
        if keys[o] in always:
            vmax = max(sweep['hdbscan'][o])
            kmax = SWEEP_K[int(np.argmax(sweep['hdbscan'][o]))]
            lines.append(f"HDBSCAN : pour tout K de {SWEEP_K[0]} à {SWEEP_K[-1]}, le meilleur IoU de {keys[o]} reste sous 0,5 "
                         f"(au plus {iou_txt(vmax)}, à K = {kmax}) : aucune valeur de min_samples ne l'isole.")
    for o in range(k):
        other = [K for K, v in zip(SWEEP_K, sweep['hdbscan'][o]) if v > 0.5]
        if best_iou[o] <= 0.5 and other:
            lines.append(f"{keys[o]} est en revanche apparié pour K = {', '.join(map(str, other))} "
                         f"(meilleur IoU {iou_txt(max(sweep['hdbscan'][o]))}).")
    with_alpine = spec.get('ground', 'patchwork_v8') != 'none'
    if not hdb:
        passed = [f"{fr(v)} m ({lab})" for v, lab in ALPINE_T if v > rmax]
        if passed:
            lines.append("Seuils de l'article, au-delà de la fin du balayage : " + ', '.join(passed)
                         + ' ; à ces niveaux, les branches suivies sont déjà fusionnées.')
    if hdb and with_alpine:
        lines.append('ALPINE sans sémantique (vue de dessus, arbre en t) : meilleurs IoU '
                     + ', '.join(f"{keys[o]} {iou_txt(sweep['alpine_bev'][o])}" for o in range(k)) + '.')
    alert = [i for i, ln in enumerate(lines) if ln.startswith(('Aucune extraction', 'Aucun seuil', 'HDBSCAN : pour tout K'))]
    if len([i for i in alert if lines[i].startswith('HDBSCAN : pour tout K')]) > 1:  # regrouper en une ligne
        grp = [i for i in alert if lines[i].startswith('HDBSCAN : pour tout K')]
        lines = [ln for i, ln in enumerate(lines) if i not in grp[1:]]
        lines[grp[0]] = (f"HDBSCAN : pour tout K de {SWEEP_K[0]} à {SWEEP_K[-1]}, les meilleurs IoU de {enum(always)} restent sous 0,5 "
                         f"(au plus " + ', '.join(f"{a} {iou_txt(max(sweep['hdbscan'][keys.index(a)]))}" for a in always)
                         + ") : aucune valeur de min_samples ne les isole.")
        alert = [i for i, ln in enumerate(lines) if ln.startswith(('Aucune extraction', 'Aucun seuil', 'HDBSCAN : pour tout K'))]
    tree_name = f'de l\'arbre HDBSCAN (K = {run["K"]})' if hdb else 'de l\'arbre ALPINE en t'
    if never:
        plural = len(never) > 1
        conclusion = (f"{enum(never)} {'ne sont isolés' if plural else 'n’est isolé'} par aucun nœud {tree_name}"
                      + ((f", ni pour aucun K de {SWEEP_K[0]} à {SWEEP_K[-1]}." if hdb else
                          f", ni par HDBSCAN pour aucun K = min_samples de {SWEEP_K[0]} à {SWEEP_K[-1]}.")
                         if set(never) <= set(always) else '.'))
    elif best_common < k:
        conclusion = 'Chaque objet a son nœud, mais aucun niveau commun ne les rend tous à la fois.'
    else:
        conclusion = f'Témoin : un même niveau de la hiérarchie {tool} sépare les trois objets.'
    x_labels = [str(K) for K in SWEEP_K] + (['ALPINE'] if with_alpine else [])
    highlight = SWEEP_K.index(run['K']) if hdb and run['K'] in SWEEP_K else len(SWEEP_K)
    summary = {
        'title': (f"Bilan — HDBSCAN, K = min_samples = {run['K']}, arbre complet (min_cluster_size = 1)" if hdb
                  else 'Bilan — ALPINE sans sémantique : arbre complet en t'),
        'lines': lines, 'alert': alert, 'conclusion': conclusion,
        'chart': {'x_labels': x_labels,
                  'x_title': 'HDBSCAN : K = min_samples' + (' · puis ALPINE (vue de dessus)' if with_alpine else ' (ALPINE omis : il suppose le sol retiré)'),
                  'series': [[round(v, 3) for v in sweep['hdbscan'][o]] + ([round(sweep['alpine_bev'][o], 3)] if with_alpine else [])
                             for o in range(k)],
                  'highlight': highlight, 'gap_before_last': with_alpine, 'iou_ok': 0.5},
    }

    # --------------------------------------------------------- recadrage
    mx = view.get('margin_x', max(2.5, 0.25 * (hi[0] - lo[0])))
    my = view.get('margin_y', 2.5)
    lo, hi = Y[allobj].min(0), Y[allobj].max(0)
    x0, x1, y0, y1 = lo[0] - mx, hi[0] + mx, lo[1] - my, hi[1] + my
    crop = ((Y[:, 0] >= x0) & (Y[:, 0] <= x1) & (Y[:, 1] >= y0) & (Y[:, 1] <= y1)
            & (Y[:, 2] <= hi[2] + view.get('z_above', 2.0)) & (Y[:, 2] >= -1.0))
    ci = np.flatnonzero(crop)
    fov, el = view.get('fov', 34.0), view.get('elevation', 30.0)
    hfov = 2 * math.degrees(math.atan(math.tan(math.radians(fov / 2)) * 1224 / 780))
    w_obj = max(hi[0] - lo[0], 1.5 * (hi[1] - lo[1]), 2.0)
    dist = view.get('distance') or (w_obj / 2) / (view.get('fill', 0.55) * math.tan(math.radians(hfov / 2)))
    in_bev = x0 <= sensor_xy[0] <= x1 and y0 <= sensor_xy[1] <= y1
    ticks = [v for v in (0.02, 0.05, 0.1, 0.2, 0.3, 0.5, 1.0, 2.0, 3.0) if rmin <= v <= rmax]
    ground_meta = ('sans sol (Patchwork++)' if spec.get('ground', 'patchwork_v8') != 'none' else 'avec le sol')
    scene = {
        'meta': {'title': spec['title'] if hdb else spec.get('title_alpine', spec['title']),
                 'subtitle': f"SemanticKITTI {spec['seq']}/{spec['frame']} · {ground_meta} · {nfmt(n)} points · aucune sémantique",
                 'badge': run_label(run), 'badge2': 'Morse HGP 3D : résultat non revendiqué ici',
                 'K': run.get('K'), 'level': sym, 'method': run['method']},
        'objects': [{'key': ob['key'], 'name': ob['name'], 'm': m[o],
                     'gt_box': [round(float(v), 3) for v in (*Y[idx_list[o]].min(0), *Y[idx_list[o]].max(0))]}
                    for o, ob in enumerate(objs)],
        'points': {a: [round(float(v), 3) for v in Y[ci, j]] for j, a in enumerate('xyz')},
        'birth': [round(float(v), 6) for v in births[ci]],
        # gt : indice de l'objet suivi, -1 fond évalué, -2 point void (hors PQ : non étiqueté, aberrant, autre structure, autre objet)
        'gt': [int(v) if v >= 0 else (-2 if vd else -1) for v, vd in zip(obj[ci], void[ci])],
        'join': [[round(float(v), 6) if np.isfinite(v) else 1e9 for v in join[o, ci]] for o in range(k)],
        'tracks': [{'seed_birth': float(births[seeds[o]]), 'best': [round(float(best[o][1]), 6), round(best_iou[o], 4)],
                    'events': [[round(v, 6) if isinstance(v, float) else v for v in ev[:12]] for ev in events[o]],
                    'segments': [[round(a, 6), round(b, 6), s] for a, b, s in segments(events[o], rmax)]} for o in range(k)],
        'branch_merges': [{'r': round(b['r'], 6), 'objects': b['objects']} for b in branch_merges],
        'levels': {'rmin': rmin, 'rmax': rmax, 'ticks': ticks, 'marks': marks},
        'level_labels': [[round(r, 6), txt] for r, txt in sorted(level_label.items())],
        'schedule': [[round(a, 4), round(b, 6)] for a, b in sched],
        'captions': captions,
        'timing': {'intro': t_intro, 'summary': t_summary, 'duration': t_summary + t_sum, 'key': t_key, 'key_rule': key_kind},
        'summary': summary,
        'view': {'azimuth': view.get('azimuth', 0.0), 'elevation': el, 'distance': dist, 'fov': fov,
                 'target': [view.get('target_x', 0.0), view.get('target_y', 0.0), float(hi[2] + lo[2]) * 0.3],
                 'intro_orbit': view.get('intro_orbit', -40.0), 'shift_y': view.get('shift_y', 30),
                 'point_size': view.get('point_size', 3.4), 'bev_point_size': view.get('bev_point_size', 2.4)},
        'bev': {'x0': x0, 'x1': x1, 'y0': y0, 'y1': y1},
        'clip': [x0, y0, -1.0, x1, y1, float(hi[2] + view.get('z_above', 2.0))],
        'grid': {'x0': math.floor(x0), 'x1': math.ceil(x1), 'y0': math.floor(y0), 'y1': math.ceil(y1), 'z': 0.0, 'step': 1.0},
        'sensor_xy': [float(v) for v in sensor_xy] if in_bev else None,
        'sensor_note': f'à {fr(float(np.hypot(*sensor_xy)), 1)} m',
    }
    (demo_dir / 'data').mkdir(exist_ok=True)
    (demo_dir / 'data' / f'scene_{tag}.js').write_text(
        'window.DEMO_SCENE = ' + json.dumps(scene, ensure_ascii=False, separators=(',', ':')) + ';\n', encoding='utf-8')

    def first_level(o, pred):
        return next((round(ev[EV_R], 4) for ev in events[o] if pred(ev)), None)

    results = {
        'demo': demo_dir.name, 'run': tag, 'method': run, 'seq': spec['seq'], 'frame': spec['frame'],
        'velodyne_sha256': digest, 'labels_sha256': kitti.labels_digest(spec['seq'], spec['frame']),
        'ground': spec.get('ground', 'patchwork_v8'),
        'n_raw': n_raw, 'n_points_hierarchy': n,
        'convention': ('min_samples = K, point compté (scikit-learn) ; min_cluster_size = 1 (arbre non condensé)' if hdb
                       else 'ALPINE sans sémantique : kNN 2D (union), arête si distance BEV < t ; arbre du lien simple en t'),
        'iou': 'IoU au sens de la qualité panoptique SemanticKITTI : points void (sem 0, 1, 52, 99) exclus ; arêtes de même poids fusionnées en bloc',
        'void_points': int(np.isin(sem, kitti.VOID).sum()),
        'rotation_deg': deg,
        'objects': [{'key': ob['key'], 'name': ob['name'], 'select': ob['select'], 'points': m[o],
                     'best_iou': round(best_iou[o], 4), 'best_level_m': round(best[o][1], 4),
                     'seed_level_m': round(float(births[seeds[o]]), 4),
                     'first_match_m': first_level(o, lambda ev: ev[EV_STATE] == MATCHED),
                     'first_fusion_m': first_level(o, lambda ev: ev[EV_STATE] == FUSED),
                     'complete90_m': first_level(o, lambda ev: ev[EV_REC] >= 0.9),
                     'matched_intervals_m': [[round(a0, 4), None if a1 is None else round(a1, 4)] for a0, a1 in intervals.get(o, [])]}
                    for o, ob in enumerate(objs)],
        'branch_merges_m': [{'level': round(b['r'], 4), 'objects': [objs[q]['key'] for q in b['objects']]} for b in branch_merges],
        'best_common_cut': {'matched_objects': best_common, 'level': None if best_common_r is None else round(best_common_r, 4),
                            'until': None if common_end is None else round(common_end, 4)},
        'sweep_best_iou': {'hdbscan_K': SWEEP_K, 'hdbscan': [[round(v, 4) for v in s] for s in sweep['hdbscan']],
                           'alpine_bev': [round(v, 4) for v in sweep['alpine_bev']]},
        'levels_shown_m': {'min': round(rmin, 4), 'max': round(rmax, 4)},
        'crop_points': int(len(ci)),
    }
    (demo_dir / f'resultats_{tag}.json').write_text(json.dumps(results, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    return results


def prepare(spec):
    X, L, n_raw, digest = load(spec)
    obj = np.full(len(X), -1, np.int64)
    idx_list = []
    for o, ob in enumerate(spec['objects']):
        idx = select_object(ob['select'], X, L)
        if (obj[idx] >= 0).any():
            raise ValueError('objets non disjoints')
        obj[idx] = o
        idx_list.append(idx)
    return X, L, n_raw, digest, obj, idx_list, (L & 0xFFFF).astype(np.int64)


def sweep_all(data, n_obj):
    X, obj, sem = data[0], data[4], data[6]
    void = np.isin(sem, kitti.VOID)
    out = {'hdbscan': [[] for _ in range(n_obj)]}
    for K in SWEEP_K:
        births, mst = mreach_mst(X, K)
        b = best_nodes(births, mst, obj, range(n_obj), void)
        for o in range(n_obj):
            out['hdbscan'][o].append(b[o][0])
    births, mst = alpine_bev_forest(X)
    b = best_nodes(births, mst, obj, range(n_obj), void)
    out['alpine_bev'] = [b[o][0] for o in range(n_obj)]
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('demo', type=Path)
    ap.add_argument('--only', help='ne construire que cette étiquette (ex. hdbscan_K5)')
    a = ap.parse_args()
    spec = json.loads((a.demo / 'demo.json').read_text(encoding='utf-8'))
    data = prepare(spec)
    sweep = sweep_all(data, len(spec['objects']))
    for run in spec['runs']:
        if a.only and run_tag(run) != a.only:
            continue
        res = build_run(a.demo, spec, data, run, sweep)
        print(res['run'], {o['key']: o['best_iou'] for o in res['objects']}, 'coupe commune :', res['best_common_cut'])


if __name__ == '__main__':
    main()
