#!/usr/bin/env python3
"""Campagne E1 de la sortie plate : tete certifiee sur H^r_{k+1} (T) et sur l'arbre de sklearn (A) contre sklearn
HDBSCAN tel quel (R0), sur scenes synthetiques gelees ou trames LiDAR.

    python3 bench/points_flat_campaign.py --mode synthetic --data DIR --manifest test_8000.manifest.json
        --export BUILD/mhgp11_points_export --gate GATE.json --flat-gate FLAT.json --work DIR --out DIR
        [--jobs 12] [--native-workers 4] [--orders 2,3,5,10] [--mcs 10,20,sqrt] [--rules eom1,eom2,eom3,leaf]
        [--permutations 2] [--rep 1] [--min-samples-plus-one]
    python3 bench/points_flat_campaign.py --mode lidar --data DIR ...   (trames : points_manifest.json)

Par scene : export natif (FULL 1..kmax) ; par ordre k :
  T      pendaison H^r_{k+1} (points_radius, m = k + 1) -> arbre de points (points_flat) ;
  R0     fit public de sklearn HDBSCAN(min_cluster_size = 20, min_samples = k, eom, epsilon 0, alpha 1, racine
         exclue, kd_tree, n_jobs = 1) : labels_ (sortie officielle a mcs 20) ; aux autres mcs et pour 'leaf' (R0L),
         tree_to_labels COMPILE de sklearn sur le meme arbre ; controle tree_to_labels = labels_ au mcs public, sinon
         les lignes R0 de la scene sont refusees ;
  R0p    fits publics sur des permutations de l'entree (etendue due a l'ordre des ex aequo) ;
  A      arbre du lien simple de sklearn relu exactement (niveaux sqrt(N)) -> meme tete que T (attribution) ;
  B      niveau B : meilleur bloc de chaque groupe, pour T et pour A ;
  O      oracle m05 exact sur chaque arbre condense (antichaine optimale, programme dynamique par comptes agreges).
Pour chaque mcs et chaque regle (eom z = 1, 2, 3, feuilles) : labels par PointId, metriques
(points_flat_metrics.scores ; lignes MAP en synthetique ; points_flat_metrics.lidar_frame en LiDAR), refus,
chemins exacts, egalites certifiees. Invariants a l'echelle (jamais exhaustifs) : D2 (z = 3 raffine z = 1, les
feuilles raffinent z = 3, sur T et sur A), D5 (clusters disjoints d'au moins mcs sites). Un JSON par scene ; une scene
en echec est publiee comme telle. Refuse de tourner sans les deux portes conformes de la meme session.
"""
import argparse
import concurrent.futures as cf
import json
import math
import os
from pathlib import Path
import resource
import sys
import time
import traceback

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_campaign as pc  # noqa: E402
import points_flat as pf  # noqa: E402
import points_flat_metrics as pm  # noqa: E402
import points_flat_study as fs  # noqa: E402
import points_hierarchy as ph  # noqa: E402
import points_radius as prad  # noqa: E402
import points_scenes_freeze as fz  # noqa: E402

RULES = fs.RULES
MCS_PUBLIC = 20


def sklearn_fit(xyz, k, order=None):
    """Fit public ; order : permutation de l'entree (labels ramenes a l'ordre du fichier)."""
    from sklearn.cluster import HDBSCAN
    data = np.asarray(xyz, dtype=np.float64)
    if order is not None:
        data = data[order]
    model = HDBSCAN(min_cluster_size=MCS_PUBLIC, min_samples=k, cluster_selection_method='eom',
                    cluster_selection_epsilon=0.0, alpha=1.0, allow_single_cluster=False, metric='euclidean',
                    algorithm='kd_tree', n_jobs=1, copy=True)
    model.fit(data)
    labels = np.asarray(model.labels_, dtype=np.int64)
    if order is not None:
        back = np.empty_like(labels)
        back[order] = labels
        labels = back
    return np.asarray(model._single_linkage_tree_), labels


def tree_labels(tree, mcs, method):
    from sklearn.cluster._hdbscan._tree import tree_to_labels
    return np.asarray(tree_to_labels(tree, mcs, method, False, 0.0, None)[0], dtype=np.int64)


def canonical(labels):
    """Identifiants canoniques (plus petit PointId du cluster) : comparaison de partitions."""
    out = np.full(len(labels), -1, dtype=np.int64)
    first = {}
    for i, c in enumerate(np.asarray(labels).tolist()):
        if c >= 0:
            out[i] = first.setdefault(c, i)
    return out


def refines(fine, coarse):
    """Vrai si chaque cluster de fine est inclus dans un cluster de coarse (bruit de fine libre)."""
    sel = fine >= 0
    if not np.any(sel):
        return True
    pairs = np.unique(np.stack([fine[sel], coarse[sel]], 1), axis=0)
    if np.any(pairs[:, 1] < 0):
        return False
    return len(np.unique(pairs[:, 0])) == len(pairs)


def oracle_m05(cond, first, ids, truth):
    """m05 exact (somme des IoU > 1/2 d'une antichaine optimale, racines exclues, / nombre de groupes) par comptes
    agreges de bas en haut : un cluster ne peut depasser 1/2 qu'avec son groupe majoritaire."""
    from fractions import Fraction
    groups = int(truth.max()) + 1 if np.any(truth >= 0) else 0
    if not groups or not len(cond):
        return 0.0
    gsize = np.bincount(truth[truth >= 0], minlength=groups)
    nc = len(cond)
    counts = [dict() for _ in range(nc)]
    size = [0] * nc
    for s, c in enumerate(first.tolist()):
        if c >= 0:
            g = int(truth[ids[s]])
            size[c] += 1
            if g >= 0:
                counts[c][g] = counts[c].get(g, 0) + 1
    best = [Fraction(0)] * nc
    for c in range(nc):  # enfants avant parents
        for d in cond.children[c]:
            size[c] += size[d]
            for g, v in counts[d].items():
                counts[c][g] = counts[c].get(g, 0) + v
        own = Fraction(0)
        if counts[c]:
            g, inter = max(counts[c].items(), key=lambda kv: kv[1])
            if 3 * inter > size[c] + int(gsize[g]):  # IoU > 1/2, test entier
                own = Fraction(inter, size[c] + int(gsize[g]) - inter)
        below = sum((best[d] for d in cond.children[c]), Fraction(0))
        best[c] = max(own, below) if cond.parent[c] >= 0 else below
    roots = [c for c in range(nc) if cond.parent[c] < 0]
    return float(sum((best[c] for c in roots), Fraction(0)) / groups)


def measure(args, truth, pred, extra):
    if args.mode == 'synthetic':
        row = pm.scores(truth['labels'], pred, map_labels=truth['map'], with_ami=False)
        row.pop('hungarian_matches', None)
    else:
        row = pm.lidar_frame(truth['raw'], pred)
        row.pop('hungarian_matches', None)
        # Critere asymetrique de l'utilisateur (LiDAR : une fusion coute plus qu'une decoupe) : etat par instance.
        states = fs.object_states(np.asarray(pred), truth['objects'], truth['void'], len(truth['keys']))
        row['states'] = [x['state'] for x in states]
        row['pieces'] = [x['pieces'] for x in states]
        row['instance_keys'] = list(truth['keys'])
    row.update(extra)
    return row


def one_scene(args, name, xyz, truth):
    started = time.monotonic()
    n = len(xyz)
    result = dict(name=name, sites=n, orders={}, status='started')
    data, report = pc.export(args.export, xyz, args.work, name, args.kmax, args.orders, args.native_workers)
    ids = data['ids'].astype(np.int64)
    if sorted(ids.tolist()) != list(range(n)):
        raise ValueError('ids')
    result['export'] = report
    mcs_list = [int(math.isqrt(n)) if m == 'sqrt' else int(m) for m in args.mcs]
    labels_eval = truth['labels'] if args.mode == 'synthetic' else truth['objects']
    void = None if args.mode == 'synthetic' else truth['void']
    nobjects = int(labels_eval.max()) + 1 if np.any(labels_eval >= 0) else 0
    for k in args.orders:
        row = dict(lines={}, timing={}, invariants={}, refusals=[])
        t0 = time.monotonic()
        hanging = prad.hang_margin_radius(data['orders'][k], k + 1, 'margin_r')
        pt = pf.tower_point_tree(hanging)
        pt.ids = ids
        row['timing']['tower'] = round(time.monotonic() - t0, 3)
        row['tower'] = dict(blocks=pt.blocks(), plateaus=len(pt.levels), delayed=hanging.extra.get('delayed'))
        t0 = time.monotonic()
        tree, labels_public = sklearn_fit(xyz, k)
        row['timing']['sklearn_fit'] = round(time.monotonic() - t0, 3)
        check = np.array_equal(tree_labels(tree, MCS_PUBLIC, 'eom'), labels_public)
        row['invariants']['tree_to_labels_equal'] = bool(check)
        apt = pf.linkage_point_tree(tree, n)
        row['hdbscan_tree'] = dict(blocks=apt.blocks(), plateaus=len(apt.levels))
        # Niveau B (meilleur bloc), T et A
        obj = np.asarray(labels_eval, dtype=np.int64)
        vmask = np.zeros(n, dtype=bool) if void is None else void
        if nobjects:
            row['level_b'] = dict(T=[round(x, 6) for x in fs.best_blocks(pt, obj[ids], vmask[ids], nobjects)],
                                  A=[round(x, 6) for x in fs.best_blocks(apt, obj, vmask, nobjects)])
        # R0p : permutations de l'entree, fit public a mcs 20
        perms = []
        rng = np.random.default_rng(int.from_bytes(name.encode()[:8].ljust(8, b'\0'), 'little') + k)
        for _ in range(args.permutations):
            order = rng.permutation(n)
            _, lab = sklearn_fit(xyz, k, order)
            perms.append(canonical(lab))
        if args.min_samples_plus_one:
            _, lab = sklearn_fit(xyz, k + 1)
            row['lines']['R0prime_eom_mcs20'] = measure(args, truth, lab, dict(official=True))
        for mcs in mcs_list:
            outputs = {}
            for side, tr in (('T', pt), ('A', apt)):
                t0 = time.monotonic()
                cond, first = pf.condense(tr, mcs)
                if side == 'T':
                    row.setdefault('oracle_m05', {})['T_mcs%d' % mcs] = oracle_m05(cond, first, tr.ids, obj) \
                        if args.mode == 'synthetic' else None
                else:
                    row.setdefault('oracle_m05', {})['A_mcs%d' % mcs] = oracle_m05(cond, first, tr.ids, obj) \
                        if args.mode == 'synthetic' else None
                for rule in args.rules:
                    method, z = RULES[rule]
                    key = '%s_%s_mcs%d' % (side, rule, mcs)
                    try:
                        sel = pf.select(tr, cond, z, method)
                    except pf.Refusal as error:
                        row['refusals'].append(dict(line=key, why=str(error)))
                        continue
                    lab = pf.labels(tr, cond, first, sel)
                    outputs[key] = lab
                    st = sel.stats
                    row['lines'][key] = measure(args, truth, lab, dict(
                        exact_paths=st['exact'], equalities=st['equalities'], decisions=st['decisions'],
                        min_margin=st['min_margin'], margins_below_1e6=st['margins_below_1e-6']))
                row['timing']['%s_mcs%d' % (side, mcs)] = round(time.monotonic() - t0, 3)
            for method in ('eom', 'leaf'):
                key = 'R0%s_mcs%d' % ('' if method == 'eom' else 'L', mcs)
                if not check:
                    row['refusals'].append(dict(line=key, why='tree_to_labels different de labels_'))
                    continue
                lab = labels_public if (method == 'eom' and mcs == MCS_PUBLIC) else tree_labels(tree, mcs, method)
                outputs[key] = lab
                row['lines'][key] = measure(args, truth, lab, dict(official=method == 'eom' and mcs == MCS_PUBLIC))
            if mcs == MCS_PUBLIC and perms:
                base = canonical(labels_public)
                row['lines']['R0p_mcs20'] = [measure(args, truth, p, dict(same_as_public=bool(np.array_equal(p, base))))
                                             for p in perms]
            # Invariants D2 et D5, cote T et cote A
            for side in ('T', 'A'):
                z1, z3, lf = (outputs.get('%s_%s_mcs%d' % (side, r, mcs)) for r in ('eom1', 'eom3', 'leaf'))
                if z1 is not None and z3 is not None and lf is not None:
                    row['invariants']['D2_%s_mcs%d' % (side, mcs)] = bool(refines(z3, z1) and refines(lf, z3))
                for key, lab in outputs.items():
                    if key.startswith(side + '_'):
                        sizes = np.bincount(lab[lab >= 0]) if np.any(lab >= 0) else np.zeros(0, dtype=np.int64)
                        if np.any((sizes > 0) & (sizes < mcs)):
                            row['invariants']['D5_violation_%s' % key] = True
        result['orders'][str(k)] = row
    result.update(status='ok', seconds=round(time.monotonic() - started, 3),
                  max_rss_mb=round(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0, 1))
    return result


def load_synthetic(args, item):
    xyz, pid, truth, mapl = fz.read_scene(str(args.data), item)
    if not np.array_equal(pid, np.arange(len(pid), dtype=pid.dtype)):
        raise ValueError('PointId non dense')
    return xyz.astype(np.int64), dict(labels=truth, map=mapl)


def load_lidar(args, name):
    xyz = np.fromfile(args.data / (name + '_sites.u32le'), dtype='<u4').reshape(-1, 3).astype(np.int64)
    raw = np.fromfile(args.data / (name + '_labels.u32le'), dtype='<u4')
    obj, void, keys = ph.lidar_objects(raw, 50)
    return xyz, dict(raw=raw, objects=obj, void=void, keys=keys)


def worker(args, entry):
    name = entry['name']
    try:
        if args.mode == 'synthetic':
            xyz, truth = load_synthetic(args, entry)
        else:
            xyz, truth = load_lidar(args, name)
        result = one_scene(args, name, xyz, truth)
        result['meta'] = {key: entry.get(key) for key in ('family', 'level', 'groups', 'n_requested', 'repetition',
                                                          'bayes_level', 'bayes_stratum', 'role', 'sequence')}
    except Exception as error:  # la scene est publiee en echec, la campagne continue
        result = dict(name=name, status='failed', error=type(error).__name__ + ': ' + str(error),
                      trace=traceback.format_exc()[-2000:])
    (args.out / (name + '.json')).write_text(json.dumps(result, sort_keys=True, default=str) + '\n')
    return name, result['status']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('synthetic', 'lidar'), required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--manifest', default='', help='manifeste des scenes gelees (synthetique)')
    parser.add_argument('--export', type=Path, required=True)
    parser.add_argument('--gate', type=Path, required=True)
    parser.add_argument('--flat-gate', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=12)
    parser.add_argument('--native-workers', type=int, default=4)
    parser.add_argument('--kmax', type=int, default=10)
    parser.add_argument('--orders', default='2,3,5,10')
    parser.add_argument('--mcs', default='10,20,sqrt')
    parser.add_argument('--rules', default='eom1,eom2,eom3,leaf')
    parser.add_argument('--permutations', type=int, default=2)
    parser.add_argument('--min-samples-plus-one', action='store_true')
    parser.add_argument('--rep', type=int, default=0, help='synthetique : une seule repetition (0 : toutes)')
    parser.add_argument('--roles', default='', help='LiDAR : roles du manifeste retenus')
    parser.add_argument('--limit', type=int, default=0)
    parser.add_argument('--shard', default='', help='i/n : scenes de rang i modulo n dans l ordre des noms')
    args = parser.parse_args()
    args.orders = [int(x) for x in args.orders.split(',')]
    args.mcs = args.mcs.split(',')
    args.rules = args.rules.split(',')
    for gate in (args.gate, args.flat_gate):
        verdict = json.loads(gate.read_text()).get('verdict') if gate.is_file() else None
        if verdict != 'conforme':
            print('refus : porte %s non conforme ou absente' % gate, file=sys.stderr)
            return 2
    args.work.mkdir(parents=True, exist_ok=True)
    args.out.mkdir(parents=True, exist_ok=True)
    if args.mode == 'synthetic':
        manifest = json.loads((args.data / args.manifest).read_text())
        todo = [item for item in manifest['scenes'] if not args.rep or item.get('repetition') == args.rep]
    else:
        manifest = json.loads((args.data / 'points_manifest.json').read_text())
        roles = set(args.roles.split(',')) if args.roles else None
        todo = [entry for entry in manifest['scenes'] if roles is None or entry.get('role') in roles]
    if args.shard:
        i, n = (int(x) for x in args.shard.split('/'))
        todo = [entry for j, entry in enumerate(sorted(todo, key=lambda e: e['name'])) if j % n == i]
    if args.limit:
        todo = todo[:args.limit]
    todo.sort(key=lambda entry: -int(entry.get('n_sites', entry.get('sites', 0))))
    statuses = {}
    with cf.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(worker, args, entry) for entry in todo]
        for future in cf.as_completed(futures):
            name, status = future.result()
            statuses[name] = status
            print(json.dumps(dict(name=name, status=status)), flush=True)
    failed = sorted(name for name, status in statuses.items() if status != 'ok')
    print('points_flat_campaign_%s scenes%d echecs%d' % (args.mode, len(statuses), len(failed)))
    return 0 if not failed else 1


if __name__ == '__main__':
    raise SystemExit(main())
