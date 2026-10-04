#!/usr/bin/env python3
"""Bouts de scène SemanticKITTI difficiles : deux ou trois objets proches, réduits aux SEULS points de ces objets.

    python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out SORTIE [--step 10] [--sequences 00,...,10]
    python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out SORTIE --rebuild bouts.json   # refaire des bouts
                                                   # (écrit SORTIE/data et SORTIE/bouts_refaits.json)

Familles : « voitures » (car, moving-car) ; « vélos » (bicycle, bicyclist, et leurs versions mobiles) ; « vélos et
piétons » (au moins un vélo et au moins un piéton). Un bout ne contient que les points de ses objets : ni sol, ni
fond, ni autre objet.

1. Étiquettes d'une trame sur --step, lues dans l'archive locale des labels (CACHE/data_odometry_labels.zip).
2. Objets d'au moins --min-points points ; trames ayant au moins deux objets d'une même famille.
3. Nuage de ces trames (archive velodyne, requêtes partielles, tools/kitti.py) ; écart entre deux objets = plus courte
   distance entre leurs points ; espacement interne d'un objet = plus longue arête de son arbre couvrant minimal
   (graphe des 10 plus proches voisins, composantes raccordées par leurs paires les plus proches).
4. Groupes : objets reliés par un écart <= --gap-voiture (voitures) ou --gap-velo (vélos, piétons) ; composantes de deux
   ou trois objets, et, dans une composante plus grande, les paires et triplets connexes.
5. Difficulté = min sur les paires reliées de écart / max(espacements internes) : sous 1, la liaison simple réunit les
   deux objets avant d'avoir assemblé le plus clairsemé. Un bout par groupe (même séquence, mêmes instances) : la trame
   la plus difficile parmi celles parcourues.
6. Bout : points des objets du groupe, quantifiés au millimètre (floor(1000 x + 1/2), calcul exact depuis le
   flottant), doublons retirés (première occurrence), translatés à l'origine ; <nom>_sites.u32le (x, y, z),
   <nom>_labels.u32le (sémantique | instance << 16) et points_manifest.json au format de
   morsehgp3D_v11/bench/points_campaign.py (rôle « bout ») ; bouts.json décrit chaque bout (trame, empreintes,
   instances, écarts) et suffit à le refaire depuis les archives officielles.

Aucun octet KITTI ni aucune coordonnée n'est versionné (CC BY-NC-SA) : SORTIE et CACHE restent hors du dépôt.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import time

import numpy as np
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components, minimum_spanning_tree
from scipy.spatial import cKDTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
import kitti  # noqa: E402

CARS = (10, 252)
BIKES = (11, 31, 253)
PEOPLE = (30, 254)
FAMILY = {s: 'voitures' for s in CARS}
FAMILY.update({s: 'deux_roues' for s in BIKES + PEOPLE})
SEQUENCES = ['%02d' % s for s in range(11)]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def quantize(values):
    """floor(x * 1000 + 1/2) exact depuis le flottant (même formule que la préparation LiDAR de la v11)."""
    out = np.empty(values.size, np.int64)
    for i, v in enumerate(values.reshape(-1).tolist()):
        num, den = (0.0 if v == 0 else v).as_integer_ratio()
        out[i] = (2 * num * 1000 + den) // (2 * den)
    return out.reshape(values.shape)


def labels_of(seq, frame):
    return np.frombuffer(kitti._zip(kitti.LABELS_ZIP).read('dataset/sequences/%s/labels/%s.label' % (seq, frame)),
                         dtype='<u4')


def objects_of(label, minimum):
    """{clé (sem | inst << 16) : indices} des objets d'intérêt d'au moins `minimum` points."""
    sem, inst = label & 0xFFFF, label >> 16
    keep = np.isin(sem, list(FAMILY)) & (inst > 0)
    keys, inverse, counts = np.unique(label[keep], return_inverse=True, return_counts=True)
    where = np.flatnonzero(keep)
    out = {}
    for j, key in enumerate(keys.tolist()):
        if counts[j] >= minimum:
            out[int(key)] = where[inverse == j]
    return out


def spacing(P, k=10):
    """Plus longue arête de l'arbre couvrant minimal (graphe des k plus proches voisins, composantes raccordées)."""
    n = len(P)
    if n < 2:
        return 0.0
    tree = cKDTree(P)
    kk = min(k + 1, n)
    dist, idx = tree.query(P, k=kk)
    rows = np.repeat(np.arange(n), kk - 1)
    graph = coo_matrix((dist[:, 1:].ravel() + 1e-12, (rows, idx[:, 1:].ravel())), shape=(n, n)).tocsr()
    mst = minimum_spanning_tree(graph.maximum(graph.T))
    inner = float(mst.data.max()) if mst.nnz else 0.0
    ncomp, comp = connected_components(mst, directed=False)
    if ncomp == 1:
        return inner
    parts = [P[comp == c] for c in range(ncomp)]
    trees = [cKDTree(p) for p in parts]
    bridge = np.full((ncomp, ncomp), np.inf)
    for a in range(ncomp):
        for b in range(a + 1, ncomp):
            small, big = (a, b) if len(parts[a]) <= len(parts[b]) else (b, a)
            bridge[a, b] = bridge[b, a] = float(trees[big].query(parts[small], k=1)[0].min())
    inside, best, worst = {0}, bridge[0].copy(), 0.0  # Prim sur les composantes
    while len(inside) < ncomp:
        c = min((x for x in range(ncomp) if x not in inside), key=lambda x: best[x])
        worst = max(worst, float(best[c]))
        inside.add(c)
        best = np.minimum(best, bridge[c])
    return max(inner, worst)


def family_ok(name, sems):
    if name == 'voitures':
        return all(s in CARS for s in sems)
    bikes = sum(s in BIKES for s in sems)
    people = sum(s in PEOPLE for s in sems)
    return bikes >= 1 and bikes + people == len(sems)


def kind_of(sems):
    if all(s in CARS for s in sems):
        return 'voitures'
    return 'velos' if all(s in BIKES for s in sems) else 'velos_pietons'


def groups_of(keys, gap, limit):
    """Paires et triplets connexes du graphe des écarts <= limit (clés triées)."""
    adj = {a: set() for a in keys}
    for (a, b), g in gap.items():
        if g <= limit:
            adj[a].add(b)
            adj[b].add(a)
    out = set()
    for a in keys:
        for b in adj[a]:
            out.add(tuple(sorted((a, b))))
            for c in adj[a] | adj[b]:
                if c not in (a, b):
                    out.add(tuple(sorted((a, b, c))))
    return sorted(out)


def analyse_frame(seq, frame, args):
    xyzi, label, digest = kitti.load_frame(seq, frame)
    objs = objects_of(label, args.min_points)
    P = {k: xyzi[v, :3].astype(np.float64) for k, v in objs.items()}
    space = {k: spacing(p) for k, p in P.items()}
    trees = {k: cKDTree(p) for k, p in P.items()}
    found = []
    for fam, limit in (('voitures', args.gap_voiture), ('deux_roues', args.gap_velo)):
        if fam not in args.familles:
            continue
        keys = sorted(k for k in objs if FAMILY[k & 0xFFFF] == fam)
        if len(keys) < 2:
            continue
        gap = {}
        for i, a in enumerate(keys):
            for b in keys[i + 1:]:
                small, big = (a, b) if len(P[a]) <= len(P[b]) else (b, a)
                gap[(a, b)] = float(trees[big].query(P[small], k=1)[0].min())
        for group in groups_of(keys, gap, limit):
            sems = [g & 0xFFFF for g in group]
            if not family_ok(fam, sems):
                continue
            pairs = [(a, b) for i, a in enumerate(group) for b in group[i + 1:] if gap[(a, b)] <= limit]
            hard = min(gap[p] / max(space[p[0]], space[p[1]], 1e-9) for p in pairs)
            found.append(dict(seq=seq, frame=frame, velodyne_sha256=digest,
                              labels_sha256=sha(labels_of(seq, frame).tobytes()), kind=kind_of(sems),
                              keys=list(group), classes=[kitti.class_name(s) for s in sems],
                              points=[int(len(P[g])) for g in group],
                              spacing=[round(space[g], 4) for g in group],
                              gaps={'%d-%d' % p: round(gap[p], 4) for p in pairs}, difficulty=round(hard, 4)))
    return found


def write_crop(entry, out):
    xyzi, label, digest = kitti.load_frame(entry['seq'], entry['frame'], entry['velodyne_sha256'])
    idx = np.concatenate([np.flatnonzero(label == k) for k in entry['keys']])
    idx.sort(kind='stable')
    q = quantize(xyzi[idx, :3])
    _, first = np.unique(q, axis=0, return_index=True)
    first.sort()
    q, lab = q[first], label[idx][first]
    q = q - q.min(axis=0)
    if q.max() >= 1 << 21:
        raise SystemExit('bout hors du domaine u21 : ' + entry['name'])
    sites = q.astype('<u4')
    sites.tofile(out / (entry['name'] + '_sites.u32le'))
    lab.astype('<u4').tofile(out / (entry['name'] + '_labels.u32le'))
    entry.update(sites=int(len(sites)), duplicates=int(len(idx) - len(sites)),
                 sites_sha256=sha((out / (entry['name'] + '_sites.u32le')).read_bytes()),
                 crop_labels_sha256=sha((out / (entry['name'] + '_labels.u32le')).read_bytes()))


def name_of(e):
    insts = '_'.join(str(k >> 16) for k in e['keys'])
    return 'b%s_%s_%s_%s' % (e['seq'], e['frame'], e['kind'], insts)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--cache', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--sequences', default=','.join(SEQUENCES))
    p.add_argument('--step', type=int, default=10)
    p.add_argument('--min-points', type=int, default=50)
    p.add_argument('--gap-voiture', type=float, default=1.0)
    p.add_argument('--gap-velo', type=float, default=0.6)
    p.add_argument('--quota', default='voitures:300,velos:300,velos_pietons:200')
    p.add_argument('--rebuild', type=Path, help='bouts.json : refaire exactement ces bouts')
    p.add_argument('--keep-frames', action='store_true', help='garder les nuages en cache')
    p.add_argument('--familles', default='voitures,deux_roues', help='familles cherchées (voitures, deux_roues)')
    args = p.parse_args()
    args.familles = args.familles.split(',')
    kitti.CACHE = args.cache
    data = args.out / 'data'
    data.mkdir(parents=True, exist_ok=True)
    if args.rebuild:
        chosen = json.loads(args.rebuild.read_text())['bouts']
    else:
        found, t0 = [], time.time()
        names = kitti._zip(kitti.LABELS_ZIP).namelist()
        for seq in args.sequences.split(','):
            frames = sorted(n.split('/')[-1][:-6] for n in names
                            if n.startswith('dataset/sequences/%s/labels/' % seq) and n.endswith('.label'))
            for frame in frames[::args.step]:
                objs = objects_of(labels_of(seq, frame), args.min_points)
                fams = [FAMILY[k & 0xFFFF] for k in objs if FAMILY[k & 0xFFFF] in args.familles]
                if max((fams.count(f) for f in set(fams)), default=0) < 2:
                    continue
                try:
                    got = analyse_frame(seq, frame, args)
                finally:
                    if not args.keep_frames:
                        for sub, ext in (('velodyne', '.bin'), ('labels', '.label')):
                            f = args.cache / sub / ('%s_%s%s' % (seq, frame, ext))
                            if f.exists():
                                f.unlink()
                found.extend(got)
                print('trame %s/%s : %d groupes (total %d, %.0f s)' % (seq, frame, len(got), len(found),
                                                                       time.time() - t0), flush=True)
        (args.out / 'candidats.json').write_text(json.dumps(found, indent=1))
        best = {}
        for e in found:  # un bout par groupe : la trame la plus difficile
            key = (e['seq'], tuple(sorted(k >> 16 for k in e['keys'])))
            if key not in best or (e['difficulty'], -sum(e['points'])) < (best[key]['difficulty'], -sum(best[key]['points'])):
                best[key] = e
        quota = dict((q.split(':')[0], int(q.split(':')[1])) for q in args.quota.split(','))
        chosen = []
        for kind, count in quota.items():
            pool = sorted((e for e in best.values() if e['kind'] == kind), key=lambda e: (e['difficulty'], e['seq'], e['frame']))
            chosen.extend(pool[:count])
            print('famille %s : %d groupes distincts, %d retenus' % (kind, len(pool), min(count, len(pool))), flush=True)
    for e in chosen:
        e['name'] = name_of(e)
        write_crop(e, data)
        if not args.keep_frames:
            for sub, ext in (('velodyne', '.bin'), ('labels', '.label')):
                f = args.cache / sub / ('%s_%s%s' % (e['seq'], e['frame'], ext))
                if f.exists():
                    f.unlink()
    manifest = dict(schema='ehgp.v11.points_lidar_manifest.v1',
                    note='bouts de scene derives de KITTI/SemanticKITTI (CC BY-NC-SA) : jamais versionnes',
                    scenes=[dict(name=e['name'], kind='bout', role='bout', sites=e['sites'], sites_sha256=e['sites_sha256'],
                                 labels_sha256=e['crop_labels_sha256']) for e in chosen])
    (data / 'points_manifest.json').write_text(json.dumps(manifest, indent=1))
    target = 'bouts_refaits.json' if args.rebuild else 'bouts.json'  # ne jamais écraser un catalogue publié
    (args.out / target).write_text(json.dumps(dict(
        schema='ehgp.zoltan.bouts.v1', parametres=dict(step=args.step, min_points=args.min_points,
                                                       gap_voiture=args.gap_voiture, gap_velo=args.gap_velo,
                                                       quota=args.quota), bouts=chosen), indent=1))
    print('bouts', len(chosen), 'sites', sum(e['sites'] for e in chosen), flush=True)


if __name__ == '__main__':
    main()
