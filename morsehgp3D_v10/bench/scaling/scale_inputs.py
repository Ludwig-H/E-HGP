"""Entrees de mise a l'echelle de la v10 : doublement et quadruplement de la taille, dans deux regimes distincts.

Synthetique (n0 = 8 000, facteurs 1, 2, 4 : n = 8 000, 16 000, 32 000) :
  - `space`   : augmentation SPATIALE a densite constante. Le support grandit avec n : plus de groupes de meme
                taille (amas, coquilles, filaments), ou un domaine plus grand (uniforme, terrain) ;
  - `density` : augmentation de DENSITE a geometrie fixe. Meme support, n fois plus de points.
  Le pas de quantification h est FIXE dans chaque (famille, regime) : h = etendue de la scene de base / 2^16, comme
  la grille de 1 mm du LiDAR. En regime `density`, la resolution relative baisse donc avec n (plus de configurations
  degenerees), exactement comme sur une trame plus dense.

LiDAR reel : les secteurs des trois trames sans sol (recus v8), coupes par des plans paralleles aux axes passant
par le capteur (signes des coordonnees float32 d'origine) : 4 quarts, 2 moitiees (x < 0, x >= 0), trame entiere,
sur la grille commune de 1 mm. Quart -> moitie -> trame est le doublement et le quadruplement spatial reel.

  python3 scale_inputs.py --out <dossier>        # ecrit les .u32le et MANIFEST.json (sha256, n, h, regime)
Aucune etiquette : ces entrees ne servent qu'au cout.
"""
import argparse
import hashlib
import json
import math
import os
import shutil

import numpy as np

N0 = 8000
FACTORS = (1, 2, 4)
SEED = 3
FAMILIES = ('uniform', 'clusters', 'shells', 'filaments', 'terrain')
LIDAR = 'morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6'
SECTORS = ('full', 'half_x_neg', 'half_x_nonneg', 'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
           'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')


def unit_grid(m):
    """m centres sur une grille cubique d'espacement un (m <= 64), dans l'ordre d'un balayage en couches."""
    side = int(math.ceil(m ** (1.0 / 3.0) - 1e-9))
    pts = [(x, y, z) for z in range(side) for y in range(side) for x in range(side)][:m]
    return np.asarray(pts, dtype=np.float64)


def base_group(family, count, rng):
    """Un groupe centre en 0 : gaussienne (amas), coquille d'epaisseur fixe, filament."""
    if family == 'clusters':
        return rng.standard_normal((count, 3))
    if family == 'shells':
        d = rng.standard_normal((count, 3))
        d /= np.linalg.norm(d, axis=1, keepdims=True)
        return d * (2.0 + 0.05 * rng.standard_normal((count, 1)))
    if family == 'filaments':
        q, _ = np.linalg.qr(rng.standard_normal((3, 3)))
        t = (rng.random((count, 1)) - 0.5) * 6.0
        return t * q[:, 0] + 0.15 * rng.standard_normal((count, 3))
    raise ValueError(family)


def grouped(family, groups, per_group, spacing, rng, orient_seed):
    """`groups` groupes identiques en loi, centres sur une grille d'espacement `spacing`."""
    centres = spacing * unit_grid(groups)
    out = []
    for j in range(groups):
        out.append(centres[j] + base_group(family, per_group, np.random.default_rng(orient_seed + 7919 * j)
                                           if family == 'filaments' else rng))
    return np.vstack(out)


def terrain(n, side, rng):
    """Surface de type sol/facade : hauteur lisse z = f(x, y) + bruit mince, sur un carre de cote `side`."""
    xy = rng.random((n, 2)) * side
    x, y = xy[:, 0], xy[:, 1]
    z = 3.0 * np.sin(x / 7.0) * np.cos(y / 11.0) + 1.5 * np.sin((x + 2 * y) / 5.0) + 0.05 * rng.standard_normal(n)
    return np.column_stack([x, y, z])


def scene(family, regime, factor):
    """Nuage flottant d'une (famille, regime, facteur) ; la graine ne depend que de ces trois cles."""
    key = hashlib.sha256(('%s|%s|%d|%d' % (family, regime, factor, SEED)).encode()).hexdigest()
    rng = np.random.default_rng(int(key[:15], 16))
    n = N0 * factor
    if family == 'uniform':
        side = float(factor) ** (1.0 / 3.0) if regime == 'space' else 1.0
        return rng.random((n, 3)) * side
    if family == 'terrain':
        side = 40.0 * math.sqrt(factor) if regime == 'space' else 40.0
        return terrain(n, side, rng)
    if regime == 'space':  # 8, 16, 32 groupes de 1 000 points : densite locale constante
        return grouped(family, 8 * factor, N0 // 8, 8.0, rng, SEED)
    return grouped(family, 8, n // 8, 8.0, rng, SEED)  # 8 groupes, 1 000, 2 000, 4 000 points par groupe


def quantize(points, h, origin):
    q = np.floor((points - origin) / h + 0.5).astype(np.int64)
    if q.min() < 0 or q.max() >= (1 << 18):
        raise ValueError('hors du domaine 18 bits')
    uniq = np.unique(q, axis=0)
    return uniq.astype(np.uint32), len(q) - len(uniq)


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--repo', default=os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..')))
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    manifest = dict(schema='mhgp10.scale_inputs.v1', n0=N0, factors=list(FACTORS), seed=SEED, entries=[])
    for family in FAMILIES:
        for regime in ('space', 'density'):
            base = scene(family, regime, 1)
            lo, hi = base.min(axis=0), base.max(axis=0)
            h = float((hi - lo).max()) / float(1 << 16)
            clouds = {f: scene(family, regime, f) for f in FACTORS}
            origin = np.min([c.min(axis=0) for c in clouds.values()], axis=0)
            for f in FACTORS:
                grid, dups = quantize(clouds[f], h, origin)
                name = 'syn_%s_%s_x%d.u32le' % (family, regime, f)
                data = np.ascontiguousarray(grid, dtype='<u4').tobytes()
                with open(os.path.join(args.out, name), 'wb') as fh:
                    fh.write(data)
                manifest['entries'].append(dict(file=name, kind='synthetic', family=family, regime=regime,
                                                factor=f, points=N0 * f, sites=len(grid), duplicates_removed=dups,
                                                step=h, sha256=sha256_bytes(data)))
    for frame in ('00', '01', '02'):
        for sector in SECTORS:
            src = os.path.join(args.repo, LIDAR, 'scene_%s_grid' % frame, sector + '.u32le')
            name = 'lidar%s_%s.u32le' % (frame, sector)
            shutil.copyfile(src, os.path.join(args.out, name))
            data = open(src, 'rb').read()
            manifest['entries'].append(dict(file=name, kind='lidar', frame=frame, sector=sector,
                                            sites=len(data) // 12, source=os.path.relpath(src, args.repo),
                                            sha256=sha256_bytes(data)))
    with open(os.path.join(args.out, 'MANIFEST.json'), 'w') as fh:
        json.dump(manifest, fh, indent=1, sort_keys=True)
    for e in manifest['entries']:
        print(e['file'], e['sites'], e.get('duplicates_removed', ''))


if __name__ == '__main__':
    main()
