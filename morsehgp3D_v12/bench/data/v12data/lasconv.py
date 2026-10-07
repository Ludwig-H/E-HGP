"""Conversion commune des nuages LAS / LAZ (aeriens et autres) au format v11 a 1 mm.

Entiers bruts X, Y, Z -> millimetres exacts (echelle et decalage de l'en-tete lus en decimal), variantes :
- `tout` : tous les retours (classes de bruit comprises) ;
- `sans_sol` : classe ASPRS 2 (sol) retiree ; c'est le pendant aerien du regime « LiDAR sans sol » ;
- options : `sans_bruit` (classes 7 et 18 retirees), recadrage par rectangle entier en mm.
Chaque variante : tous les retours (`<nom>`), et s'il y a des doublons au mm `<nom>.distinct` (+ multiplicites).
"""
from __future__ import annotations

from pathlib import Path

import numpy as np

from . import common
from .formats import las, laz

ASPRS = {0: 'never classified', 1: 'unassigned', 2: 'ground', 3: 'low vegetation', 4: 'medium vegetation',
         5: 'high vegetation', 6: 'building', 7: 'low noise', 8: 'model key / reserved', 9: 'water', 10: 'rail',
         11: 'road surface', 12: 'overlap / reserved', 13: 'wire guard', 14: 'wire conductor',
         15: 'transmission tower', 16: 'wire connector', 17: 'bridge deck', 18: 'high noise'}


def read_any(path: Path) -> dict:
    header = las.read_header(path)
    if header['compressed'] or str(path).lower().endswith('.laz'):
        return laz.read_points(path)
    return las.read_points(path)


def quantize(points: dict) -> tuple[np.ndarray, dict]:
    sx, sy, sz = points['header']['scale']
    ox, oy, oz = points['header']['offset']
    qx, ix = common.quantize_scaled_int_mm(points['X'], sx, ox)
    qy, iy = common.quantize_scaled_int_mm(points['Y'], sy, oy)
    qz, iz = common.quantize_scaled_int_mm(points['Z'], sz, oz)
    return np.stack([qx, qy, qz], axis=1), dict(x=ix, y=iy, z=iz)


def class_histogram(classification: np.ndarray, names: dict | None = None) -> dict:
    names = ASPRS if names is None else names
    values, counts = np.unique(classification, return_counts=True)
    return {'%d %s' % (v, names.get(int(v), '?')): int(c) for v, c in zip(values.tolist(), counts.tolist())}


def convert(path: Path, out_dir: Path, name: str, dataset: dict, source: dict, script: str,
            variants=('tout', 'sans_sol'), crop_mm: tuple | None = None, family: str = 'multi_millions',
            write_variants=('raw', 'distinct'), exclude_always: dict | None = None,
            class_names: dict | None = None) -> list:
    """Rend la liste des manifestes de scene ecrits (un par variante).

    exclude_always : {classe: raison} retirees de toutes les variantes (ex. IGN 66 = points virtuels, qui ne sont
    pas des retours LiDAR). class_names : noms des classes du producteur pour les histogrammes (ASPRS par defaut) ;
    la variante `sans_sol` retire toujours la classe 2, qui est le sol chez l'IGN, l'ASPRS et FOR-instance.
    """
    pts = read_any(path)
    n_file = len(pts['X'])
    q, scale_info = quantize(pts)
    keep = np.ones(n_file, dtype=bool)
    for value in (exclude_always or {}):
        keep &= pts['classification'] != value
    crop_info = None
    if crop_mm is not None:
        (x0, y0), (x1, y1) = crop_mm
        keep &= (q[:, 0] >= x0) & (q[:, 0] < x1) & (q[:, 1] >= y0) & (q[:, 1] < y1)
        crop_info = dict(x_mm=[x0, x1], y_mm=[y0, y1], rule='x0 <= x < x1 and y0 <= y < y1 (absolute mm)')
    cls = pts['classification']
    manifests = []
    for variant in variants:
        mask = keep.copy()
        if 'sans_sol' in variant:
            mask &= cls != 2
        if 'sans_bruit' in variant:
            mask &= (cls != 7) & (cls != 18)
        idx = np.flatnonzero(mask)
        case = name if variant == 'tout' else '%s_%s' % (name, variant)
        result = common.prepare_outputs(out_dir, case, q[idx], ids=idx, variants=write_variants, primary='raw')
        conversion = dict(source_type='LAS integers with header scale/offset (%s, point format %s)'
                          % (pts['header'].get('version'), pts['header'].get('point_format')),
                          las_scale_offset=scale_info, variant=variant,
                          variant_rule={'tout': 'all returns of the file (crop and exclusions applied)',
                                        'sans_sol': 'ASPRS class 2 (ground) removed',
                                        'sans_sol_sans_bruit': 'ASPRS classes 2, 7 and 18 removed'}.get(variant),
                          crop=crop_info, ids='index of the point in the source file (0-based)',
                          excluded_classes_all_variants={str(k): v for k, v in (exclude_always or {}).items()},
                          labels_used='classification of the producer (not ours) for ground/noise removal'
                          if variant != 'tout' else False,
                          class_histogram_written=class_histogram(cls[idx], class_names),
                          class_names='producer' if class_names is not None else 'ASPRS')
        manifest = common.scene_manifest(case, family, dataset, source, conversion, result, script,
                                         extra=dict(file_points=int(n_file),
                                                    file_class_histogram=class_histogram(cls, class_names),
                                                    file_header=dict((k, v) for k, v in pts['header'].items()
                                                                     if k in ('version', 'point_format', 'count',
                                                                              'scale', 'offset'))))
        common.write_json(out_dir / (case + '.manifest.json'), manifest)
        manifests.append(manifest)
        common.log('%s : %d points, %d distincts, %d bits' % (case, result['measures']['n_points'],
                                                               result['measures']['n_distinct'],
                                                               result['measures']['bits_needed']))
    return manifests


def set_entry(manifest: dict) -> dict:
    """Ligne de manifeste d'ensemble (format proche de build/v11-full-data-20261002/manifest.json)."""
    outs = manifest['outputs']
    main = outs.get('raw') or outs.get('distinct')
    entry = dict(name=manifest['name'], coordinates=main['coordinates'], point_ids=main['point_ids'],
                 count=main['count'], duplicate_sites=main['duplicate_sites'], sha256=main['sha256'],
                 ids_sha256=main['ids_sha256'], coordinate_encoding=main['coordinate_encoding'],
                 point_id_encoding=main['point_id_encoding'], profile=manifest['measures']['minimal_profile'],
                 bits_needed=manifest['measures']['bits_needed'], extent_mm=manifest['measures']['extent_mm'],
                 n_distinct=manifest['measures']['n_distinct'], dataset=manifest['dataset'].get('name'),
                 sensor=manifest['dataset'].get('sensor'), licence=manifest['dataset'].get('licence'))
    if 'distinct' in outs:
        d = outs['distinct']
        entry['distinct'] = dict(coordinates=d['coordinates'], point_ids=d['point_ids'], count=d['count'],
                                 sha256=d['sha256'], ids_sha256=d['ids_sha256'], mult=d['mult']['file'],
                                 mult_sha256=d['mult']['sha256'])
    return entry
