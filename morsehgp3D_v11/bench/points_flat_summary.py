#!/usr/bin/env python3
"""Lecture des JSON de bench/points_flat_campaign.py : tableaux par ligne, ordre et mcs, ecarts apparies, regle de
decision de l'E1 (spec du juge, § 3.6), choix de la selection synthetique sur le dev.

    python3 bench/points_flat_summary.py --runs DIR [--runs DIR ...] --out FICHIER.json [--rules eom1,eom2,eom3,leaf]
        [--primary eom1] [--decision] [--seed TEXTE]

Synthetique. Unite d'analyse : la scene ; estimande : moyenne equiponderee des moyennes de cellule (famille, niveau,
groupes, taille). Pour une ligne L et l'adversaire R0 : D_s = moyenne sur les mcs de mIoU_h(L) - mIoU_h(R0) a k fixe ;
test t unilateral sur l'erreur type intra-cellule (somme des R_c - 1 degres de liberte), correction de Holm sur les
ordres ; intervalle par bootstrap stratifie par cellule (R_c - 1 tirages parmi R_c, correction de McCarthy-Snowden),
graine tiree du texte --seed. Gardes : PQ et ARI_s (memes ecarts, unilateral, sans correction). Decomposition :
L - R0 = (L - A_L) + (A_L - R0). Aucune decision ne lit un flottant a egalite : les metriques arrivent des JSON.
Choix du dev (--rules) : pour chaque regle de la tour, moyenne equiponderee des cellules de mIoU_h, PQ, ARI_s, sur
tous les k et tous les mcs, et par k ; la regle retenue maximise mIoU_h (garde : PQ pas significativement pire).
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

import numpy as np


def load(dirs):
    out = []
    for d in dirs:
        for path in sorted(Path(d).glob('*.json')):
            r = json.loads(path.read_text())
            if r.get('status') == 'ok' and 'orders' in r:
                out.append(r)
    return out


def cell_of(r):
    m = r.get('meta') or {}
    return '%s|%s|g%s|n%s' % (m.get('family'), m.get('level'), m.get('groups'), m.get('n_requested'))


def value(row, metric):
    v = row.get(metric)
    if v is None and metric == 'pq':
        v = row.get('PQ')
    return float(v) if v is not None else float('nan')


def line_value(order, line, mcs_keys, metric):
    """Moyenne sur les mcs de la metrique d'une ligne ; nan si une valeur manque (refus)."""
    vals = []
    for m in mcs_keys:
        row = order['lines'].get('%s_mcs%s' % (line, m))
        if row is None:
            return float('nan')
        vals.append(value(row, metric))
    return float(np.mean(vals)) if vals else float('nan')


def mcs_keys_of(order):
    keys = set()
    for key in order['lines']:
        if '_mcs' in key and not key.startswith('R0p') and not key.startswith('R0prime'):
            keys.add(key.split('_mcs')[1])
    return sorted(keys, key=lambda x: int(x))


def per_scene(results, line, ref, k, metric):
    """{cellule: [ecarts par scene]} pour la ligne contre la reference a l'ordre k (moyenne sur les mcs)."""
    cells = {}
    for r in results:
        order = r['orders'].get(str(k))
        if order is None:
            continue
        mk = mcs_keys_of(order)
        a, b = line_value(order, line, mk, metric), line_value(order, ref, mk, metric)
        if math.isnan(a) or math.isnan(b):
            continue
        cells.setdefault(cell_of(r), []).append(a - b)
    return cells


def estimate(cells):
    """Moyenne equiponderee des moyennes de cellule, erreur type intra-cellule, degres de liberte."""
    means = [np.mean(v) for v in cells.values() if v]
    if not means:
        return dict(mean=float('nan'), se=float('nan'), df=0, cells=0)
    c = len(means)
    var = 0.0
    df = 0
    for v in cells.values():
        if len(v) >= 2:
            var += np.var(v, ddof=1) / len(v)
            df += len(v) - 1
    se = math.sqrt(var) / c if df else float('nan')
    return dict(mean=float(np.mean(means)), se=float(se), df=int(df), cells=c)


def t_sf(t, df):
    """Queue superieure de la loi de Student (scipy si present)."""
    try:
        from scipy.stats import t as student
        return float(student.sf(t, df))
    except ImportError:  # approximation normale
        return 0.5 * math.erfc(t / math.sqrt(2))


def bootstrap(cells, seed_text, draws=10000):
    """Intervalle a 95 % par bootstrap stratifie (McCarthy-Snowden : R_c - 1 tirages parmi R_c)."""
    seed = int(hashlib.sha256(seed_text.encode()).hexdigest()[:16], 16)
    rng = np.random.default_rng(seed)
    groups = [np.asarray(v) for v in cells.values() if len(v) >= 2]
    if not groups:
        return [float('nan'), float('nan')]
    stats = np.empty(draws)
    for i in range(draws):
        stats[i] = np.mean([g[rng.integers(0, len(g), len(g) - 1)].mean() for g in groups])
    return [float(np.quantile(stats, 0.025)), float(np.quantile(stats, 0.975))]


from points_flat_claims import h_l1_claim, h_l2_claim, holm  # noqa: E402  regles de revendication (stdlib)


def dev_table(results, rules, orders):
    """Choix du dev : moyennes equiponderees des cellules, par regle (tour, meme tete sur sklearn, sklearn)."""
    table = {}
    lines = ['T_' + r for r in rules] + ['A_' + r for r in rules] + ['R0', 'R0L']
    for line in lines:
        for metric in ('miou_h', 'pq', 'ari_s'):
            for k in list(orders) + ['all']:
                cells = {}
                for r in results:
                    for kk in (orders if k == 'all' else [k]):
                        order = r['orders'].get(str(kk))
                        if order is None:
                            continue
                        v = line_value(order, line, mcs_keys_of(order), metric)
                        if not math.isnan(v):
                            cells.setdefault(cell_of(r), []).append(v)
                means = [np.mean(v) for v in cells.values()]
                table['%s|%s|k=%s' % (line, metric, k)] = round(float(np.mean(means)), 4) if means else None
    return table


def decision(results, line, orders, seed_text):
    """Regle 3.6.1 : pour chaque k, L contre R0 (primaire mIoU_h), gardes PQ et ARI_s, Holm sur les ordres."""
    rows = {}
    pvals = []
    for k in orders:
        cells = per_scene(results, line, 'R0', k, 'miou_h')
        est = estimate(cells)
        t = est['mean'] / est['se'] if est['se'] and est['se'] > 0 else float('nan')
        p = t_sf(t, est['df']) if not math.isnan(t) else float('nan')
        guards = {}
        for metric in ('pq', 'ari_s'):
            g = estimate(per_scene(results, line, 'R0', k, metric))
            tg = g['mean'] / g['se'] if g['se'] and g['se'] > 0 else float('nan')
            guards[metric] = dict(mean=g['mean'], p_worse=t_sf(-tg, g['df']) if not math.isnan(tg) else float('nan'))
        attribution = estimate(per_scene(results, line, 'A_' + line.split('_', 1)[1], k, 'miou_h'))
        ties = estimate(per_scene(results, 'A_' + line.split('_', 1)[1], 'R0', k, 'miou_h'))
        rows[str(k)] = dict(delta=est, t=t, p=p, ci95=bootstrap(cells, seed_text + '|%d' % k),
                            guards=guards, hierarchy_term=attribution['mean'], ties_term=ties['mean'])
        pvals.append(p if not math.isnan(p) else 1.0)
    for k, adj in zip(orders, holm(pvals)):
        rows[str(k)]['p_holm'] = adj
    return rows


def lidar_pairs(results, line, ref, k, mcs, field):
    """Couples (instance physique, difference) pour une trame et un ordre : field 'merged' (indicateur d'etat
    fusionne) ou 'iou' (IoU un-a-un par instance, points_flat_metrics.lidar_frame)."""
    out = []
    for r in results:
        order = r['orders'].get(str(k))
        if order is None:
            continue
        a, b = order['lines'].get('%s_mcs%d' % (line, mcs)), order['lines'].get('%s_mcs%d' % (ref, mcs))
        if a is None or b is None:
            continue
        if field == 'merged':
            va = {key: st == 'fusionne' for key, st in zip(a['instance_keys'], a['states'])}
            vb = {key: st == 'fusionne' for key, st in zip(b['instance_keys'], b['states'])}
        else:
            va = {row['key']: row['iou_h'] for row in a['instances']}
            vb = {row['key']: row['iou_h'] for row in b['instances']}
        for key in va:
            if key in vb:  # grappe = instance physique : identifiant d'instance (bits hauts), stable dans la sequence
                out.append((int(key) >> 16, float(va[key]) - float(vb[key])))
    return out


def cluster_bootstrap(pairs, seed_text, draws=10000):
    """Moyenne des differences et sa distribution par bootstrap en grappes sur les instances physiques."""
    if not pairs:
        return float('nan'), np.array([])
    groups = {}
    for key, d in pairs:
        groups.setdefault(key, []).append(d)
    keys = sorted(groups)
    sums = np.array([sum(groups[k]) for k in keys])
    counts = np.array([len(groups[k]) for k in keys])
    seed = int(hashlib.sha256(seed_text.encode()).hexdigest()[:16], 16)
    rng = np.random.default_rng(seed)
    idx = rng.integers(0, len(keys), size=(draws, len(keys)))
    boot = sums[idx].sum(axis=1) / counts[idx].sum(axis=1)
    return float(sums.sum() / counts.sum()), boot


def lidar_decision(results, seed_text, orders=(5, 10), mcs=20):
    """Preenregistrement P08 : H_L1 (fusions de T_eom2 < R0) et H_L2 (non-inferiorite de T_eom1 en IoU, marge 0,02),
    bootstrap en grappes sur les instances physiques, Holm sur les quatre tests."""
    rows, pvals, names = {}, [], []
    for k in orders:
        pairs = lidar_pairs(results, 'T_eom2', 'R0', k, mcs, 'merged')
        mean, boot = cluster_bootstrap(pairs, seed_text + '|L1|%d' % k)
        p = float((np.sum(boot >= 0) + 1) / (len(boot) + 1)) if len(boot) else 1.0
        rows['H_L1_k%d' % k] = dict(delta_merged=mean, ci95=[float(np.quantile(boot, 0.025)), float(np.quantile(boot, 0.975))]
                                    if len(boot) else None, p=p, pairs=len(pairs))
        pvals.append(p)
        names.append('H_L1_k%d' % k)
        pairs = lidar_pairs(results, 'T_eom1', 'R0', k, mcs, 'iou')
        mean, boot = cluster_bootstrap(pairs, seed_text + '|L2|%d' % k)
        p = float((np.sum(boot <= -0.02) + 1) / (len(boot) + 1)) if len(boot) else 1.0
        rows['H_L2_k%d' % k] = dict(delta_iou=mean, ci95=[float(np.quantile(boot, 0.025)), float(np.quantile(boot, 0.975))]
                                    if len(boot) else None, p_noninferiority=p, pairs=len(pairs),
                                    superiority=bool(len(boot) and np.quantile(boot, 0.025) > 0))
        pvals.append(p)
        names.append('H_L2_k%d' % k)
    for name, adj in zip(names, holm(pvals)):
        rows[name]['p_holm'] = adj
        if name.startswith('H_L2'):  # P08 : Holm ET borne basse de l'IC a 95 % strictement > -0,02
            ci = rows[name]['ci95']
            rows[name]['claimed'] = h_l2_claim(adj, ci[0] if ci else None)
        else:
            rows[name]['claimed'] = h_l1_claim(adj)
    return rows


def lidar_states(results, orders=(3, 5, 10), mcs_list=(20, 10)):
    """Parts d'instances intactes, fusionnees, decoupees, au bruit et absorbees par ligne, ordre et mcs."""
    table = {}
    for r in results:
        for k in orders:
            order = r['orders'].get(str(k))
            if order is None:
                continue
            for key, row in order['lines'].items():
                if 'states' not in row:
                    continue
                cell = table.setdefault('%s|k=%d' % (key, k), {})
                for st in row['states']:
                    cell[st] = cell.get(st, 0) + 1
    out = {}
    for key, cell in table.items():
        n = sum(cell.values())
        out[key] = {st: round(v / n, 4) for st, v in cell.items()}
        out[key]['instances'] = n
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runs', action='append', required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--rules', default='eom1,eom2,eom3,leaf')
    parser.add_argument('--orders', default='2,3,5,10')
    parser.add_argument('--primary', default='eom1')
    parser.add_argument('--decision', action='store_true')
    parser.add_argument('--seed', default='v11e1')
    parser.add_argument('--lidar', action='store_true', help='campagne LiDAR : etats par instance et tests P08')
    args = parser.parse_args()
    results = load(args.runs)
    orders = [int(x) for x in args.orders.split(',')]
    rules = args.rules.split(',')
    if args.lidar:
        out = dict(frames=len(results), states=lidar_states(results), decision=lidar_decision(results, args.seed))
        args.out.write_text(json.dumps(out, indent=1, sort_keys=True) + '\n')
        print(json.dumps({k: (v.get('claimed'), round(v.get('delta_merged', v.get('delta_iou', float('nan'))), 4))
                          for k, v in out['decision'].items()}))
        return 0
    out = dict(scenes=len(results), cells=len(set(cell_of(r) for r in results)),
               dev=dev_table(results, rules, orders))
    if args.decision:
        out['decision'] = decision(results, 'T_' + args.primary, orders, args.seed)
    refusals = sum(len(o.get('refusals', [])) for r in results for o in r['orders'].values())
    out['refusals'] = refusals
    out['invariants'] = {key: sum(1 for r in results for o in r['orders'].values()
                                  for kk, v in o.get('invariants', {}).items() if kk.startswith(key) and v is False)
                         for key in ('D2', 'tree_to_labels')}
    out['d5_violations'] = sum(1 for r in results for o in r['orders'].values()
                               for kk in o.get('invariants', {}) if kk.startswith('D5_violation'))
    args.out.write_text(json.dumps(out, indent=1, sort_keys=True) + '\n')
    print(json.dumps(dict(scenes=out['scenes'], cells=out['cells'], refusals=refusals,
                          invariants=out['invariants'], d5=out['d5_violations'])))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
