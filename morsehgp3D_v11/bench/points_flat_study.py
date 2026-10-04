#!/usr/bin/env python3
"""Choix de la selection plate sur les exemples LiDAR ou la hierarchie HGP marche (E1, etape 0, hors VM).

    python3 points_flat_study.py --dumps DIR [--dumps DIR ...] --scenes DIR [--scenes DIR ...] --out FICHIER.json
        [--jobs 4] [--mcs 10,20] [--rules eom1,eom2,eom3,leaf]

Entrees : arbres de points de H^r_{k+1} et arbres de sklearn exportes par bench/points_flat_dump.py sur G4, scenes
(sites et etiquettes) des dossiers de bench/points_unpack.py. Aucune hierarchie n'est recalculee ici.

Consigne de l'utilisateur (4 octobre 2026) : la selection plate doit etre elegante mathematiquement ET retrouver des
clusters pertinents, sans trop de fusions parasites, sur les demos LiDAR ou la hierarchie HGP marche.

Critere ecrit AVANT toute lecture d'une sortie plate LiDAR (4 octobre 2026, 10 h 30 UTC) :
  objets      instances 'thing' d'au moins 50 points non void (classes void 0, 1, 52, 99 retirees des comptes), comme
              bench/points_hierarchy.lidar_objects ;
  population  couples (scene, k) ou la hierarchie marche : TOUS les objets ont un bloc de H^r_{k+1} d'IoU > 1/2
              (niveau B recalcule sur l'arbre de points exporte, meme evaluateur que bench/points_hierarchy) ;
              secondaire : tous les objets trouves par la hierarchie (niveau B > 1/2), toutes scenes ;
  mesures     par (regle, mcs, k) et par cote (tour T, meme tete sur l'arbre de sklearn A ; sklearn tel quel R0) :
                tous_retrouves  part des couples ou chaque objet est apparie un-a-un a IoU > 1/2 ;
                retrouves       part des objets apparies a IoU > 1/2 (appariement unique, test entier 3|G n C| > |G|+|C|);
                fusions         part des objets dont le cluster majoritaire contient aussi plus de la moitie d'un autre
                                objet (fusion parasite) ;
                fragmentes      part des objets sans cluster contenant plus de la moitie de leurs points, mais coupes
                                en au moins deux clusters d'au moins 20 % chacun ;
                bruit           part des objets dont plus de la moitie des points est du bruit ;
                miou            IoU un-a-un moyen (hongrois, non apparie = 0) ;
  regles      EOM phi = r^-z, z = 1, 2, 3, et feuilles, mcs = 10 et 20, racine exclue, aucune completion ;
  decision    parmi les regles elegantes (aucun parametre libre ajoute a mcs), on retient celle qui maximise
              tous_retrouves sur la population, sous la garde fusions <= celle de l'EOM z = 1 ; l'egalite a 0,01 pres
              va a la plus simple (z = 1, puis feuilles, puis z = 3, puis z = 2). Les mesures de toutes les regles sont
              publiees, la decision est relue par l'utilisateur.
Les lignes A et R0 sont descriptives ici (sklearn local pour les mcs autres que 20 : jamais une sortie officielle).
"""
import argparse
import concurrent.futures as cf
import json
import os
from pathlib import Path
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_flat as pf  # noqa: E402
import points_hierarchy as ph  # noqa: E402

RULES = {'eom1': ('eom', 1), 'eom2': ('eom', 2), 'eom3': ('eom', 3), 'leaf': ('leaf', 1)}


def best_blocks(pt, nobj, nvoid, objects):
    """Niveau B sur l'arbre de points : meilleur IoU de chaque objet parmi les blocs des plateaux fermes."""
    ev = ph.Evaluator(pt.blocks(), nobj, nvoid, objects)
    children = {}
    for b, parent in enumerate(pt.block_parent):
        if parent >= 0:
            children.setdefault(parent, []).append(b)
    created = {}
    for b, p in enumerate(pt.block_plateau):
        created.setdefault(p, []).append(b)
    entering = {}
    for s, p in enumerate(pt.site_plateau.tolist()):
        entering.setdefault(p, []).append(s)
    for p in range(len(pt.levels)):
        for b in created.get(p, ()):
            if pt.block_merged[b]:
                ev.merge(children.get(b, []), b)
        for s in entering.get(p, ()):
            ev.enter(s, int(pt.site_block[s]))
        ev.level = p
        ev.close()
    return list(ev.best)


def object_rows(labels, obj, void, objects):
    """Mesures par objet d'une partition (labels par PointId, -1 bruit ; obj, void dans le meme ordre)."""
    from scipy.optimize import linear_sum_assignment
    keep = ~void
    lab, ob = labels[keep], obj[keep]
    clusters, csize = np.unique(lab[lab >= 0], return_counts=True)
    cindex = {int(c): j for j, c in enumerate(clusters.tolist())}
    gsize = np.bincount(ob[ob >= 0], minlength=objects)
    inter = np.zeros((objects, len(clusters)), dtype=np.int64)
    noise = np.zeros(objects, dtype=np.int64)
    sel = ob >= 0
    for o, c in zip(ob[sel].tolist(), lab[sel].tolist()):
        if c >= 0:
            inter[o, cindex[c]] += 1
        else:
            noise[o] += 1
    iou = np.zeros(inter.shape)
    if inter.size:
        union = gsize[:, None] + csize[None, :] - inter
        iou = np.where(inter > 0, inter / np.maximum(union, 1), 0.0)
    matched = np.zeros(objects)
    if iou.size:
        r, c = linear_sum_assignment(-iou)
        matched[r] = iou[r, c]
    found = [bool(np.any(3 * inter[o] > gsize[o] + csize)) for o in range(objects)]
    rows = []
    for o in range(objects):
        major = int(np.argmax(inter[o])) if len(clusters) and inter[o].max() > 0 else -1
        merged = major >= 0 and any(2 * inter[p, major] > gsize[p] for p in range(objects) if p != o)
        share = inter[o] / max(1, gsize[o])
        fragmented = (not np.any(2 * inter[o] > gsize[o])) and int(np.sum(share >= 0.2)) >= 2
        rows.append(dict(iou=round(float(matched[o]), 4), found=found[o], merged=bool(merged),
                         fragmented=bool(fragmented), noise=bool(2 * noise[o] > gsize[o])))
    return rows, int(len(clusters))


def sklearn_labels(sk, mcs, method):
    from sklearn.cluster._hdbscan._tree import tree_to_labels
    hierarchy = np.zeros(len(sk['left']), dtype=[('left_node', np.intp), ('right_node', np.intp),
                                                 ('value', np.float64), ('cluster_size', np.intp)])
    hierarchy['left_node'], hierarchy['right_node'] = sk['left'], sk['right']
    hierarchy['value'], hierarchy['cluster_size'] = sk['value'], sk['size']
    return np.asarray(tree_to_labels(hierarchy, mcs, method, False, 0.0, None)[0], dtype=np.int64)


def one(job):
    name, k, dumps, scenes, mcs_list, rules = job
    xyz = np.fromfile(scenes / (name + '_sites.u32le'), dtype='<u4').reshape(-1, 3)
    raw = np.fromfile(scenes / (name + '_labels.u32le'), dtype='<u4')
    obj, void, keys = ph.lidar_objects(raw, 50)
    objects = len(keys)
    out = dict(name=name, k=k, sites=len(xyz), objects=objects, lines={})
    if objects == 0:
        return out
    started = time.monotonic()
    pt = pf.PointTree.load(dumps / ('%s_k%d_tower.npz' % (name, k)))
    with np.load(dumps / ('%s_k%d_sklearn.npz' % (name, k))) as z:
        sk = {key: z[key] for key in z.files}
    tree = np.stack([sk['left'], sk['right'], sk['value']], 1)
    apt = pf.linkage_point_tree(tree, len(xyz))
    nobj, nvoid = obj[pt.ids], void[pt.ids]
    out['level_b_tower'] = [round(x, 4) for x in best_blocks(pt, nobj, nvoid, objects)]
    out['level_b_hdbscan'] = [round(x, 4) for x in best_blocks(apt, obj, void, objects)]
    out['exact'] = isinstance(pt.levels[0], pf.Level)
    for mcs in mcs_list:
        for side, tree_ in (('T', pt), ('A', apt)):
            cond, first = pf.condense(tree_, mcs)
            for rule in rules:
                method, z = RULES[rule]
                key = '%s_%s_mcs%d' % (side, rule, mcs)
                try:
                    sel = pf.select(tree_, cond, z, method)
                except pf.Refusal as error:
                    out['lines'][key] = dict(refusal=str(error))
                    continue
                lab = pf.labels(tree_, cond, first, sel)
                rows, nclusters = object_rows(lab, obj, void, objects)
                out['lines'][key] = dict(rows=rows, clusters=nclusters, exact=sel.stats['exact'],
                                         equalities=sel.stats['equalities'])
        for method in ('eom', 'leaf'):
            key = 'R0_%s_mcs%d' % (method, mcs)
            lab = sk['labels_mcs20'].astype(np.int64) if (mcs == 20 and method == 'eom') else \
                sklearn_labels(sk, mcs, method)
            rows, nclusters = object_rows(lab, obj, void, objects)
            out['lines'][key] = dict(rows=rows, clusters=nclusters, official=(mcs == 20 and method == 'eom'))
    out['seconds'] = round(time.monotonic() - started, 3)
    return out


def summarize(results, mcs_list, rules):
    """Tableaux du critere : population ou la hierarchie marche (tous les objets), et tous les objets trouves."""
    keys = sorted(set(key for r in results for key in r.get('lines', {})))
    table = {}
    for key in keys:
        for k in sorted(set(r['k'] for r in results)) + ['all']:
            pop = [r for r in results if (k == 'all' or r['k'] == k) and r['objects'] and
                   all(x > 0.5 for x in r.get('level_b_tower', [0]))]
            objs = [(r, o) for r in results if (k == 'all' or r['k'] == k) and r['objects']
                    for o in range(r['objects']) if r['level_b_tower'][o] > 0.5]
            lines = [r['lines'].get(key) for r in pop]
            if not pop or any(line is None for line in lines):
                continue
            refused = sum(1 for line in lines if 'refusal' in line)
            ok = [line for line in lines if 'rows' in line]
            allfound = sum(1 for line in ok if all(row['found'] for row in line['rows']))
            rows = [row for line in ok for row in line['rows']]
            orows = [r['lines'][key]['rows'][o] for r, o in objs if 'rows' in r['lines'].get(key, {})]
            table['%s|k=%s' % (key, k)] = dict(
                scenes=len(pop), refused=refused, all_found=round(allfound / max(1, len(ok)), 4),
                found=round(np.mean([row['found'] for row in rows]), 4) if rows else None,
                merged=round(np.mean([row['merged'] for row in rows]), 4) if rows else None,
                fragmented=round(np.mean([row['fragmented'] for row in rows]), 4) if rows else None,
                noise=round(np.mean([row['noise'] for row in rows]), 4) if rows else None,
                miou=round(np.mean([row['iou'] for row in rows]), 4) if rows else None,
                objects_found_by_hierarchy=len(orows),
                found_among_hierarchy_objects=round(np.mean([row['found'] for row in orows]), 4) if orows else None,
                merged_among_hierarchy_objects=round(np.mean([row['merged'] for row in orows]), 4) if orows else None)
    return table


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dumps', type=Path, action='append', required=True)
    parser.add_argument('--scenes', type=Path, action='append', required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--mcs', default='10,20')
    parser.add_argument('--rules', default='eom1,eom2,eom3,leaf')
    parser.add_argument('--orders', default='2,3,5,10')
    parser.add_argument('--only', default='', help='noms de scenes separes par des virgules')
    args = parser.parse_args()
    mcs_list = [int(x) for x in args.mcs.split(',')]
    rules = args.rules.split(',')
    only = set(args.only.split(',')) if args.only else None
    jobs = []
    for dumps, scenes in zip(args.dumps, args.scenes):
        for meta in sorted(dumps.glob('*.json')):
            if meta.name == 'gate.json':
                continue
            info = json.loads(meta.read_text())
            if info.get('status') != 'ok' or (only and info['name'] not in only):
                continue
            for k in [int(x) for x in args.orders.split(',')]:
                if str(k) in info['orders']:
                    jobs.append((info['name'], k, dumps, scenes, mcs_list, rules))
    results = []
    with cf.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(one, jobs, chunksize=1):
            results.append(result)
    table = summarize(results, mcs_list, rules)
    args.out.write_text(json.dumps(dict(results=results, table=table), sort_keys=True) + '\n')
    print(json.dumps(dict(jobs=len(jobs), table_rows=len(table)), sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
