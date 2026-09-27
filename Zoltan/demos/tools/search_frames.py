#!/usr/bin/env python3
"""Criblage des trames difficiles pour la hiérarchie HDBSCAN.

Usage :
    python3 Zoltan/demos/tools/search_frames.py --seq 08 --step 8 --out criblage.jsonl [--workers 4]
    python3 Zoltan/demos/tools/search_frames.py --seq 08 --frames 000877 000882 --out dense.jsonl

Critère (sans seuil réglé à la main) : pour chaque instance « thing » de la
vérité terrain (au moins 40 points après retrait du sol), le **meilleur IoU
atteignable par un nœud** de l'arbre HDBSCAN complet (min_samples = K,
point compté ; min_cluster_size = 1), pour K = 1, 5 et 10. L'IoU suit
l'évaluation panoptique de SemanticKITTI : les points « void » (non
étiqueté, aberrant, autre structure, autre objet) en sont exclus. Toute
extraction HDBSCAN (EOM, feuilles, coupe à eps) rend des nœuds de cet
arbre : si le meilleur IoU est <= 0,5 (seuil d'appariement de la qualité
panoptique), aucune extraction ne peut compter l'objet comme vrai positif.

Présélection par les seuls labels : trames ayant au moins une petite
instance (vélo, moto, piéton, cycliste, motard) et au moins deux voitures,
une sur ``--step``. Chaque trame est lue par requêtes partielles
(``kitti.py``), sans sol (Patchwork++ v8), et l'arbre est calculé sur la
trame entière (30 000 à 80 000 points).
"""
from __future__ import annotations

import argparse
import json
import multiprocessing as mp
import sys
import time
from pathlib import Path

import numpy as np
from scipy.spatial import cKDTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
import kitti  # noqa: E402
from ground import ground_mask  # noqa: E402
from hierarchy import best_nodes, mreach_mst  # noqa: E402

KS = (1, 5, 10)
SMALL = {'bicycle', 'motorcycle', 'person', 'bicyclist', 'motorcyclist'}


def preselect(seq: str, step: int):
    """Trames retenues d'après les seuls labels (pas de géométrie)."""
    z = kitti._zip(kitti.LABELS_ZIP)
    names = sorted(n for n in z.namelist() if n.startswith(f'dataset/sequences/{seq}/labels/') and n.endswith('.label'))
    keep = []
    for n in names:
        lab = np.frombuffer(z.read(n), np.uint32)
        sem, inst = lab & 0xFFFF, lab >> 16
        things = np.isin(sem, list(kitti.THING)) & (inst > 0)
        keys, cnt = np.unique(lab[things], return_counts=True)
        classes = [kitti.THING[int(k & 0xFFFF)] for k, c in zip(keys, cnt) if c >= 60]
        if sum(c in SMALL for c in classes) >= 1 and classes.count('car') >= 2:
            keep.append(n.split('/')[-1][:6])
    return keep[::step]


def analyse(item):
    seq, frame = item
    t0 = time.time()
    try:
        xyzi, label, digest = kitti.load_frame(seq, frame)
        keep = ground_mask(xyzi) != 1
        X = xyzi[keep, :3].astype(np.float64)
        L = label[keep]
        sem = (L & 0xFFFF).astype(np.int64)
        inst = (L >> 16).astype(np.int64)
        lbl = np.where(np.isin(sem, list(kitti.THING)) & (inst > 0), L.astype(np.int64), -1)
        objs, cnt = np.unique(lbl[lbl >= 0], return_counts=True)
        sel = objs[cnt >= 40]
        void = np.isin(sem, kitti.VOID)
        best = {}
        for K in KS:
            births, mst = mreach_mst(X, K, n_jobs=1)
            best[K] = best_nodes(births, mst, lbl, sel, void)
        tree = cKDTree(X)
        rows = []
        for o in sel:
            idx = np.flatnonzero(lbl == o)
            near = np.unique(np.concatenate([np.asarray(v, np.int64) for v in tree.query_ball_point(X[idx], r=0.3)]))
            near = near[lbl[near] != o]
            neigh = {}
            for s in sem[near]:
                name = kitti.class_name(int(s))
                neigh[name] = neigh.get(name, 0) + 1
            others = np.flatnonzero((lbl >= 0) & (lbl != o))
            dmin = float(cKDTree(X[others]).query(X[idx], k=1)[0].min()) if len(others) else None
            rows.append({'sem': int(o) & 0xFFFF, 'inst': int(o) >> 16, 'cls': kitti.THING[int(o) & 0xFFFF],
                         'points': int(len(idx)), 'range_m': round(float(np.hypot(*X[idx, :2].mean(0))), 2),
                         'neighbours_0_3m': dict(sorted(neigh.items(), key=lambda kv: -kv[1])[:4]),
                         'dmin_other_instance_m': None if dmin is None else round(dmin, 3),
                         'best_iou': {str(K): round(best[K][o][0], 4) for K in KS}})
        return {'seq': seq, 'frame': frame, 'velodyne_sha256': digest, 'n_raw': int(len(xyzi)),
                'n_without_ground': int(len(X)), 'instances': rows, 'seconds': round(time.time() - t0, 1)}
    except Exception as exc:  # une trame illisible ne doit pas arrêter le criblage
        return {'seq': seq, 'frame': frame, 'error': repr(exc)}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--seq', default='08')
    ap.add_argument('--step', type=int, default=8)
    ap.add_argument('--frames', nargs='*')
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--workers', type=int, default=4)
    a = ap.parse_args()
    frames = a.frames or preselect(a.seq, a.step)
    done = set()
    if a.out.exists():
        done = {json.loads(line)['frame'] for line in a.out.read_text().splitlines() if line.strip()}
    items = [(a.seq, f) for f in frames if f not in done]
    print(f'{len(items)} trames à analyser', flush=True)
    with mp.Pool(a.workers) as pool, a.out.open('a') as out:
        for k, row in enumerate(pool.imap_unordered(analyse, items)):
            out.write(json.dumps(row, ensure_ascii=False) + '\n')
            out.flush()
            if k % 20 == 0:
                print(k, row['frame'], row.get('seconds', row.get('error')), flush=True)


if __name__ == '__main__':
    main()
