#!/usr/bin/env python3
"""Petits nuages (100 a 10 000 sites) pour morsehgp3D_v12 (famille c).

    python3 -I prepare_small.py --out DIR [--kitti DIR] [--bouts DIR] [--scene NOM=CHEMIN/<nom> ...]

Quatre sous-familles, toutes reproductibles a l'octet (SplitMix64 entier, aucune loi de numpy.random) :
1. synthetiques (`synth_*`) : cube uniforme (18 bits, comme les uniformes du contrat), huit amas gaussiens
   (Irwin-Hall entier), coquille spherique (rayon 10 m), dalle mince (50 m x 50 m x 5 cm), et deux familles
   DEGENEREES declarees comme telles (reseau au pas de 1 m : cospherites massives ; points alignes) ;
   tailles 100, 300, 1 000, 3 000, 10 000 (MESURE.md § 2) ; positions distinctes par rejet (ordre de generation) ;
   reseaux en boites a x b x c noeuds de memes tailles.
2. objets reels (`kobj_*`) : instances SemanticKITTI (classes « objets » : voiture, velo, pieton, ...) de la
   selection v12set, au moins 100 sites, et leur boite de contexte (boite englobante + 1,5 m, tous les sites sans sol
   de la trame) ; jusqu'a 4 instances par classe, tailles reparties.
3. boules des n plus proches voisins (`knn_*`) : n in {100, 300, 1000, 3000, 10000} autour d'un site tire par
   SplitMix64, dans une trame par sequence et dans les scenes multi-millions passees par --scene ; selection EXACTE
   (distance carree entiere, egalites par indice).
4. bouts de scene de la v11 (`bout_*`, option --bouts) : 2 ou 3 objets proches (voitures, velos, pietons) de
   trames SemanticKITTI de 10 sequences (dont 03, 04, 07, 09, absentes du cache local de trames), empreintes
   verifiees, au plus 6 par sequence.
Chaque nuage : `<nom>.u32le`, `<nom>.ids.u32le` (identifiants de la source : indice du retour brut pour les
coupes reelles), translation au minimum ; manifeste unique `manifest.json`.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np  # noqa: E402

from v12data import common  # noqa: E402
from v12data.splitmix import Rng  # noqa: E402

SIZES = (100, 300, 1000, 3000, 10000)  # MESURE.md § 2 de la v12
KNN_SIZES = SIZES
LATTICES = ((5, 5, 4), (10, 6, 5), (10, 10, 10), (20, 15, 10), (25, 20, 20))  # 100 ... 10 000 noeuds
SEED = 20261007
THING = {10: 'car', 11: 'bicycle', 13: 'bus', 15: 'motorcycle', 16: 'on-rails', 18: 'truck', 20: 'other-vehicle',
         30: 'person', 31: 'bicyclist', 32: 'motorcyclist', 252: 'car', 253: 'bicyclist', 254: 'person',
         255: 'motorcyclist', 256: 'on-rails', 257: 'bus', 258: 'truck', 259: 'other-vehicle'}
CONTEXT_MARGIN_MM = 1500
PER_CLASS = 4
BOUTS_PER_SEQUENCE = 6


def take_distinct(points: np.ndarray, n: int) -> np.ndarray:
    """Les n premieres positions distinctes dans l'ordre de generation (rejet des repetitions)."""
    seen, keep = set(), []
    for i, p in enumerate(map(tuple, points.tolist())):
        if p not in seen:
            seen.add(p)
            keep.append(i)
            if len(keep) == n:
                return points[keep]
    raise RuntimeError('pas assez de positions distinctes')


def synth(family: str, n: int, seed: int) -> tuple[np.ndarray, dict]:
    rng = Rng(seed)
    over = 2 * n + 64
    if family == 'uniform':
        pts = rng.integers(1 << 18, 3 * over).reshape(over, 3)
        params = dict(cube_mm=1 << 18)
    elif family == 'clusters8':
        centers = rng.integers(1 << 18, 24).reshape(8, 3)
        which = rng.integers(8, over)
        offsets = rng.gaussian_int(2000, 1, 3 * over).reshape(over, 3)
        pts = centers[which] + offsets
        params = dict(clusters=8, sigma_mm=2000, centers_in_cube_mm=1 << 18, gaussian='Irwin-Hall 12 (entier)')
    elif family == 'sphere':
        g = rng.gaussian_int(1 << 20, 1, 3 * over).reshape(over, 3).astype(np.float64)
        norm = np.sqrt((g * g).sum(axis=1))
        good = norm > 0
        r = 10000.0
        pts = np.floor(g[good] * (r / norm[good])[:, None] + 0.5).astype(np.int64)  # + - * / sqrt : IEEE exacts
        params = dict(radius_mm=10000, direction='Irwin-Hall 3D normalise (sqrt IEEE correctement arrondie)')
    elif family == 'slab':
        xy = rng.integers(50000, 2 * over).reshape(over, 2)
        z = rng.integers(50, over).reshape(over, 1)
        pts = np.hstack([xy, z])
        params = dict(size_mm=[50000, 50000, 50])
    elif family == 'line':
        t = np.sort(rng.integers(1 << 20, over))
        pts = np.stack([t, 2 * t, 3 * t], axis=1)
        params = dict(direction=[1, 2, 3], parameter_bound=1 << 20, degenerate='collinear')
    else:
        raise ValueError(family)
    return take_distinct(pts.astype(np.int64), n), params


def lattice(shape: tuple) -> tuple[np.ndarray, dict]:
    axes = [np.arange(k, dtype=np.int64) * 1000 for k in shape]
    pts = np.stack(np.meshgrid(*axes, indexing='ij'), axis=-1).reshape(-1, 3)
    return pts, dict(shape=list(shape), spacing_mm=1000, degenerate='integer lattice: massive cosphericity')


def load_case(prefix: Path):
    u = np.fromfile(str(prefix) + '.u32le', '<u4').reshape(-1, 3).astype(np.int64)
    ids = np.fromfile(str(prefix) + '.ids.u32le', '<u4').astype(np.int64)
    lab_path = Path(str(prefix) + '.labels.u32le')
    labels = np.fromfile(lab_path, '<u4') if lab_path.is_file() else None
    return u, ids, labels


def knn_ball(u: np.ndarray, seed_index: int, n: int) -> np.ndarray:
    """Indices des n sites les plus proches du site seed_index (distance carree entiere, egalites par indice)."""
    if int(u.max()).bit_length() > 30:
        raise ValueError('domaine > 30 bits : distance carree hors int64')
    d2 = ((u - u[seed_index]) ** 2).sum(axis=1)
    if n >= len(u):
        return np.arange(len(u))
    t = np.partition(d2, n - 1)[n - 1]
    inside = np.flatnonzero(d2 < t)
    tie = np.flatnonzero(d2 == t)[: n - len(inside)]
    return np.sort(np.concatenate([inside, tie]))


def record(out: Path, name: str, sub: str, q: np.ndarray, ids, params: dict, labels=None) -> dict:
    result = common.prepare_outputs(out, name, q, ids=ids, labels=labels, variants=('distinct',),
                                    primary='distinct')
    main = result['outputs'].get('distinct') or result['outputs']['raw']
    return dict(name=name, subfamily=sub, coordinates=main['coordinates'], point_ids=main['point_ids'],
                count=main['count'], duplicate_sites=0, sha256=main['sha256'], ids_sha256=main['ids_sha256'],
                coordinate_encoding='little_endian_u32_xyz', point_id_encoding='little_endian_u32',
                profile=result['measures']['minimal_profile'], bits_needed=result['measures']['bits_needed'],
                extent_mm=result['measures']['extent_mm'],
                duplicate_points_in_source=result['measures']['duplicate_points'],
                labels=main.get('labels', {}).get('file'), provenance=params)


def import_bouts(out: Path, folder: Path) -> list:
    """Bouts de scene de la v11 (Zoltan/demos, schema ehgp.zoltan.bouts.v1) : 2 ou 3 objets proches (voitures,
    velos, pietons) decoupes de trames SemanticKITTI de 10 sequences, sans sol ni fond. Empreintes verifiees ;
    selection stratifiee : par sequence, au plus BOUTS_PER_SEQUENCE bouts de 100 a 10 000 sites, rangs regulierement
    espaces par taille."""
    meta = json.loads((folder / 'bouts.json').read_text())
    if meta.get('schema') != 'ehgp.zoltan.bouts.v1':
        raise RuntimeError('schema de bouts inattendu')
    by_seq = {}
    for b in meta['bouts']:
        if 100 <= b['sites'] <= 10000:
            by_seq.setdefault(b['seq'], []).append(b)
    data_dir = folder / 'data' if (folder / 'data').is_dir() else folder  # dossier plat admis (paquet G4)
    out_cases = []
    for seq in sorted(by_seq):
        items = sorted(by_seq[seq], key=lambda b: (b['sites'], b['name']))
        k = min(BOUTS_PER_SEQUENCE, len(items))
        ranks = sorted({round(i * (len(items) - 1) / (k - 1)) for i in range(k)}) if k > 1 else [0]
        for r in ranks:
            b = items[r]
            sites_path = data_dir / (b['name'] + '_sites.u32le')
            labels_path = data_dir / (b['name'] + '_labels.u32le')
            if common.sha256_file(sites_path) != b['sites_sha256']:
                raise RuntimeError('empreinte de bout fausse : ' + b['name'])
            labels = None
            if labels_path.is_file() and common.sha256_file(labels_path) == b.get('crop_labels_sha256'):
                labels = np.fromfile(labels_path, '<u4')
            q = np.fromfile(sites_path, '<u4').reshape(-1, 3).astype(np.int64)
            params = dict(kind='real_cutout', dataset='SemanticKITTI', source='v11 bouts (Zoltan/demos, '
                          'ehgp.zoltan.bouts.v1)', sequence=b['seq'], frame=b['frame'],
                          velodyne_sha256=b['velodyne_sha256'], labels_sha256=b['labels_sha256'],
                          bout_kind=b['kind'], classes=b['classes'], points=b['points'], gaps=b.get('gaps'),
                          difficulty=b.get('difficulty'), source_sites_sha256=b['sites_sha256'],
                          labels_used=True, id_derivation='index of the site in the v11 bout file')
            out_cases.append(record(out, 'bout_' + b['name'], 'bout_v11', q, np.arange(len(q)), params, labels))
    return out_cases


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--kitti', type=Path, default=None, help='dossier des trames preparees (manifest_v12set)')
    parser.add_argument('--scene', action='append', default=[], help='NOM=prefixe (sans .u32le) d\'une grande scene')
    parser.add_argument('--bouts', type=Path, default=None,
                        help='dossier des bouts de scene de la v11 (bouts.json + data/<nom>_sites.u32le)')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    cases = []
    # 1. synthetiques
    for fi, family in enumerate(('uniform', 'clusters8', 'sphere', 'slab', 'line')):
        sizes = SIZES if family != 'line' else (100, 1000)  # aligne : temoin de refus, deux tailles
        for n in sizes:
            seed = SEED + 1000 * fi + n
            q, params = synth(family, n, seed)
            params.update(kind='synthetic', family=family, seed=seed, generator='SplitMix64 entier',
                          id_derivation='generation order 0..n-1', degenerate=params.get('degenerate', False))
            cases.append(record(args.out, 'synth_%s_n%d' % (family, n), 'synthetique', q, np.arange(n), params))
    for shape in LATTICES:
        q, params = lattice(shape)
        params.update(kind='synthetic', family='lattice', id_derivation='ij order')
        cases.append(record(args.out, 'synth_lattice_n%d' % len(q), 'synthetique', q, np.arange(len(q)), params))
    # 2. objets reels et 3. boules k-NN dans les trames KITTI
    if args.kitti is not None:
        selection = json.loads((args.kitti / 'manifest_v12set.json').read_text())['cases']
        by_class = {}
        for case in selection:
            prefix = args.kitti / case['name']
            u, ids, labels = load_case(prefix)
            if labels is None:
                continue
            sem, inst = labels & 0xFFFF, labels >> 16
            thing = np.isin(sem, list(THING)) & (inst > 0)
            values, counts = np.unique(labels[thing], return_counts=True)
            for value, count in zip(values.tolist(), counts.tolist()):
                if count >= 100:
                    cls = THING[value & 0xFFFF]
                    by_class.setdefault(cls, []).append((count, case['name'], value))
        for cls in sorted(by_class):
            items = sorted(by_class[cls])
            k = min(PER_CLASS, len(items))
            ranks = sorted({round(i * (len(items) - 1) / (k - 1)) for i in range(k)}) if k > 1 else [0]
            for r in ranks:
                count, frame_name, value = items[r]
                u, ids, labels = load_case(args.kitti / frame_name)
                member = np.flatnonzero(labels == value)
                base = dict(kind='real_cutout', dataset='SemanticKITTI', source_case=frame_name, label=int(value),
                            semantic=int(value & 0xFFFF), instance=int(value >> 16), class_name=cls,
                            labels_used=True, id_derivation='raw return index in the source frame')
                tag = '%s_%s_i%d' % (frame_name.replace('kitti_ng_', ''), cls.replace('-', ''), value >> 16)
                cases.append(record(args.out, 'kobj_' + tag, 'objet_reel', u[member], ids[member],
                                    dict(base, cut='instance'), labels[member]))
                lo = u[member].min(axis=0) - CONTEXT_MARGIN_MM
                hi = u[member].max(axis=0) + CONTEXT_MARGIN_MM
                box = np.flatnonzero(np.all((u >= lo) & (u <= hi), axis=1))
                if 100 <= len(box) <= 10000 and len(box) > len(member):  # contexte non vide, taille dans la plage
                    cases.append(record(args.out, 'kctx_' + tag, 'objet_reel_contexte', u[box], ids[box],
                                        dict(base, cut='bounding box + %d mm, all sites' % CONTEXT_MARGIN_MM),
                                        labels[box]))
        seen_seq = set()
        knn_sources = []
        for case in selection:
            seq = case['provenance']['sequence']
            if seq not in seen_seq:
                seen_seq.add(seq)
                knn_sources.append((case['name'], args.kitti / case['name'], 'SemanticKITTI'))
    else:
        knn_sources = []
    for spec in args.scene:
        name, prefix = spec.split('=', 1)
        knn_sources.append((name, Path(prefix), name))
    for si, (src_name, prefix, dataset) in enumerate(knn_sources):
        u, ids, labels = load_case(prefix)
        rng = Rng(SEED + 77 * (si + 1))
        seed_index = int(rng.integers(len(u), 1)[0])
        for n in KNN_SIZES:
            sel = knn_ball(u, seed_index, n)
            params = dict(kind='real_cutout', dataset=dataset, source_case=src_name, cut='k nearest sites (exact)',
                          seed_site_index=seed_index, seed_site_id=int(ids[seed_index]), k=n, labels_used=False,
                          id_derivation='ids of the source case')
            cases.append(record(args.out, 'knn_%s_n%d' % (src_name.replace('kitti_ng_', 'k'), n), 'boule_knn',
                                u[sel], ids[sel], params, labels[sel] if labels is not None else None))
    if args.bouts is not None:
        cases += import_bouts(args.out, args.bouts)
    sizes = sorted(c['count'] for c in cases)
    summary = dict(cases=len(cases), sites_min=sizes[0], sites_max=sizes[-1],
                   per_subfamily={s: sum(1 for c in cases if c['subfamily'] == s)
                                  for s in sorted({c['subfamily'] for c in cases})},
                   degenerate=[c['name'] for c in cases if c['provenance'].get('degenerate')])
    common.write_json(args.out / 'manifest.json', dict(schema=common.SCHEMA_SET, family='petits_nuages',
                                                      scope='100 to 10000 sites', summary=summary, cases=cases,
                                                      generator=common.generator_info(__file__),
                                                      created_utc=common.utc_now(), public_status='not_claimed'))
    common.log('%d petits nuages (%d a %d sites)' % (len(cases), sizes[0], sizes[-1]))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
