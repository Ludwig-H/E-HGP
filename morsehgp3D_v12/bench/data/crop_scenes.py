#!/usr/bin/env python3
"""Decoupes spatiales emboitees d'une grande scene (jamais de sous-echantillonnage) : MESURE.md § 2 de la v12
(« tailles 1, 2, 4 et 8 millions par decoupe spatiale, puis la scene entiere ; avec et sans sol »).

    python3 -I crop_scenes.py --manifest DOSSIER/manifest.json [--sizes 1000000 2000000 4000000 8000000]
        [--only NOM ...]

Pour chaque scene du manifeste : on part des POSITIONS DISTINCTES de la scene (variante `.distinct` s'il y a des
doublons, sinon le fichier unique) et de la distance de Tchebychev horizontale d(x) = max(|x - cx|, |y - cy|) de chaque
site au centre entier (cx, cy) = plancher du milieu de la boite xy. Pour une taille visee N, le rayon r est la distance
du N-ieme site dans l'ordre croissant des distances, et la decoupe garde TOUS les sites de distance au plus r : le
carre horizontal ferme de rayon r (cote 2r), sur toute la hauteur (CST-0218 : une colonne de memes x, y n'est jamais
tranchee). Son nombre de sites (`count`) est donc au moins N, et le depasse quand plusieurs sites sont a la distance r ;
N est publie a part (`crop.target_sites`). Les rayons croissent avec N : decoupes emboitees par construction
(1 M c 2 M c 4 M c 8 M). Une taille >= au nombre de sites de la scene, ou dont le carre couvre toute la scene, est
sautee. Chaque decoupe : `<scene>_c<N>.u32le` (nom selon la taille visee), `.ids.u32le` (les identifiants de la
source), `.mult.u32le` (multiplicites de la source, si la scene a des doublons), translation au minimum, ordre
lexicographique ; les decoupes s'ajoutent au manifeste d'ensemble du dossier (`crops`).
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np  # noqa: E402

from v12data import common  # noqa: E402

SIZES = (1_000_000, 2_000_000, 4_000_000, 8_000_000)
RULE = ('all distinct sites whose horizontal Chebyshev distance to the integer xy box centre is at most r, '
        'r = distance of the N-th site in increasing distance order: closed horizontal square of radius r, '
        'entire height, no column split (count >= N); nested')


def label(n: int) -> str:
    return '%dM' % (n // 1_000_000) if n % 1_000_000 == 0 else ('%dk' % (n // 1000) if n % 1000 == 0 else str(n))


def crop_distances(u: np.ndarray):
    """Distance de Tchebychev horizontale (entiere, en mm) de chaque site au centre entier de la boite xy."""
    lo = u[:, :2].min(axis=0).astype(np.int64)
    hi = u[:, :2].max(axis=0).astype(np.int64)
    center = (lo + hi) // 2
    d = u[:, :2].astype(np.int64) - center
    return np.abs(d).max(axis=1), [int(c) for c in center.tolist()]


def crop_selection(dinf: np.ndarray, n: int):
    """(rang des sites gardes en ordre croissant, rayon r) : carre ferme de rayon r = distance du n-ieme site."""
    radius = int(np.partition(dinf, n - 1)[n - 1])
    return np.flatnonzero(dinf <= radius), radius


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--sizes', type=int, nargs='*', default=list(SIZES))
    parser.add_argument('--only', nargs='*', default=None)
    args = parser.parse_args()
    if any(n < 1 for n in args.sizes):
        parser.error('--sizes : tailles entieres strictement positives')
    root = args.manifest.parent
    manifest = json.loads(args.manifest.read_text())
    crops = {c['name']: c for c in manifest.get('crops', [])}
    for case in manifest['cases']:
        if args.only and case['name'] not in args.only:
            continue
        src = case.get('distinct') or case
        u = np.fromfile(root / src['coordinates'], '<u4').reshape(-1, 3)
        ids = np.fromfile(root / src['point_ids'], '<u4')
        mult = np.fromfile(root / src['mult'], '<u4') if case.get('distinct') else None
        dinf, center = crop_distances(u)
        for n in sorted(set(args.sizes)):
            name = '%s_c%s' % (case['name'], label(n))
            if n >= len(u):
                crops.pop(name, None)  # jamais une entree d'un rejeu anterieur laissee pour une decoupe sautee
                continue
            sel, radius = crop_selection(dinf, n)  # rangs croissants : ordre lexicographique conserve
            if len(sel) == len(u):
                crops.pop(name, None)
                common.log('%s : le carre de rayon %d mm couvre toute la scene, decoupe sautee' % (name, radius))
                continue
            result = common.prepare_outputs(root, name, u[sel].astype(np.int64), ids=ids[sel],
                                            variants=('distinct',), primary='distinct')
            main_out = result['outputs'].get('distinct') or result['outputs']['raw']
            entry = dict(name=name, parent=case['name'], coordinates=main_out['coordinates'],
                         point_ids=main_out['point_ids'], count=main_out['count'], duplicate_sites=0,
                         sha256=main_out['sha256'], ids_sha256=main_out['ids_sha256'],
                         coordinate_encoding='little_endian_u32_xyz', point_id_encoding='little_endian_u32',
                         profile=result['measures']['minimal_profile'],
                         bits_needed=result['measures']['bits_needed'], extent_mm=result['measures']['extent_mm'],
                         crop=dict(rule=RULE, target_sites=int(n), centre_xy_parent_frame_mm=center,
                                   parent_sites=int(len(u)), parent_coordinates_sha256=src['sha256'],
                                   radius_chebyshev_mm=radius))
            if mult is not None:
                path = root / (name + '.mult.u32le')
                entry['mult'] = path.name
                entry['mult_sha256'] = common.write_u32(path, mult[sel])
                entry['returns'] = int(mult[sel].sum())
            crops[name] = entry
            common.log('%s : %d sites (taille visee %d), %d bits, rayon %d mm' % (
                name, entry['count'], n, entry['bits_needed'], radius))
    manifest['crops'] = [crops[k] for k in sorted(crops)]
    common.write_json(args.manifest, manifest)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
