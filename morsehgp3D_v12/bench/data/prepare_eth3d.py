#!/usr/bin/env python3
"""ETH3D, scans laser terrestres de verite terrain (TLS, FARO Focus X 330) -> format v11 a 1 mm (famille b).

    python3 -I prepare_eth3d.py --raw DIR --out DIR [--scenes NOM ...]

Archive `multi_view_training_dslr_scan_eval.7z` (1,92 Go, telechargement direct sur www.eth3d.net, sans compte ;
licence CC BY-NC-SA 4.0) : 13 scenes, 1 a 4 stations par scene, chaque station en PLY binaire (x, y, z float32, metres,
repere du scanner) et `scan_alignment.mlp` (matrices 4x4 MeshLab vers le repere commun de la scene).
Deux sortes de scenes :
- station seule (`eth3d_<scene>_scanK`) : le nuage tel que le scanner l'a mesure, aucune transformation ; grille 1 mm
  exacte depuis le float32 ;
- scene assemblee (`eth3d_<scene>`) : toutes les stations, chacune transformee par sa matrice (coefficients lus en
  decimal dans le .mlp, produits et sommes float64 elementaires dans un ordre fixe : deterministe sur toute machine
  IEEE, sans BLAS ni FMA), puis grille 1 mm exacte ; identifiant = decalage de la station + indice dans la station.
Pas de variante sans sol : ETH3D ne fournit aucune classification (le sol n'est retire que la ou le producteur l'a
classe, jamais par une heuristique ajoutee ici). Doublons au mm : `<nom>` brut + `<nom>.distinct` (D8).
La lecture du 7z exige py7zr ou un binaire 7z (v12data/formats/sevenzip.py) ; tout le reste est numpy.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np  # noqa: E402

from v12data import common, lasconv  # noqa: E402
from v12data.formats import ply, sevenzip  # noqa: E402

URL = 'https://www.eth3d.net/data/multi_view_training_dslr_scan_eval.7z'
ARCHIVE = 'multi_view_training_dslr_scan_eval.7z'
SIZE = 1920731473
SHA256 = 'c47042bdecf29ca1d7e468182f0484869178c12cb2ec3c89e47b3c4e9189b962'  # epingle le 7 oct. 2026
DATASET = dict(name='ETH3D high-resolution multi-view benchmark, scans laser de verite terrain (dslr_scan_eval)',
               sensor='TLS, scanner laser terrestre a dephasage FARO Focus X 330, stations fixes',
               licence='CC BY-NC-SA 4.0', licence_url='https://www.eth3d.net/ ; https://www.eth3d.net/datasets',
               access='telechargement direct HTTPS www.eth3d.net/data/, sans compte',
               density='tres variable : sous-millimetrique pres de la station, centimetrique a 50 m')
# scenes retenues : stations seules (2-10 M et > 10 M de points) et scenes assemblees
SCENES = {
    'eth3d_meadow_scan1': ('meadow', ['scan1.ply'], False),
    'eth3d_courtyard_scan1': ('courtyard', ['scan1.ply'], False),
    'eth3d_courtyard': ('courtyard', ['scan1.ply', 'scan2.ply'], True),
    'eth3d_electro': ('electro', ['scan1.ply', 'scan2.ply', 'scan3.ply', 'scan4.ply'], True),
}


def parse_mlp(path: Path) -> dict:
    text = path.read_text()
    out = {}
    for m in re.finditer(r'<MLMesh[^>]*filename="([^"]+)"[^>]*>\s*<MLMatrix44>(.*?)</MLMatrix44>', text, re.S):
        values = m.group(2).split()
        if len(values) != 16:
            raise ValueError('matrice 4x4 attendue pour ' + m.group(1))
        out[m.group(1)] = values
    return out


def transform(xyz32: np.ndarray, matrix_text: list) -> np.ndarray:
    """x' = ((r0 x + r1 y) + r2 z) + t, float64 elementaire, ordre fixe ; derniere ligne verifiee = 0 0 0 1."""
    m = [float(v) for v in matrix_text]  # decimal -> double le plus proche (deterministe)
    if m[12:] != [0.0, 0.0, 0.0, 1.0]:
        raise ValueError('matrice non affine')
    x, y, z = (xyz32[:, i].astype(np.float64) for i in range(3))
    out = np.empty((len(xyz32), 3), dtype=np.float64)
    for row in range(3):
        r0, r1, r2, t = m[4 * row:4 * row + 4]
        acc = x * r0
        acc += y * r1
        acc += z * r2
        acc += t
        out[:, row] = acc
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--raw', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--scenes', nargs='*', default=list(SCENES))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    common.selftest_quantize()
    dl = common.download(URL, args.raw / ARCHIVE, expected_sha256=SHA256, expected_size=SIZE)
    common.log('%s : %s' % (ARCHIVE, 'telecharge' if dl['downloaded'] else 'present'))
    needed = sorted({'%s/dslr_scan_eval/%s' % (SCENES[s][0], f) for s in args.scenes for f in SCENES[s][1]}
                    | {'%s/dslr_scan_eval/scan_alignment.mlp' % SCENES[s][0] for s in args.scenes})
    extracted = args.raw / 'extracted'
    missing = [m for m in needed if not (extracted / m).is_file()]
    if missing:
        sevenzip.extract(args.raw / ARCHIVE, missing, extracted)
    entries = []
    for name in args.scenes:
        scene, scans, merged = SCENES[name]
        folder = extracted / scene / 'dslr_scan_eval'
        mlp = parse_mlp(folder / 'scan_alignment.mlp') if merged else None
        parts, ids, offsets, files = [], [], [], []
        offset = 0
        for scan in scans:
            data, header = ply.read_vertices(folder / scan)
            xyz = np.stack([data['x'], data['y'], data['z']], axis=1)
            q = common.quantize_float_mm(transform(xyz, mlp[scan]) if merged else xyz)
            parts.append(q)
            ids.append(np.arange(offset, offset + len(q), dtype=np.int64))
            offsets.append(dict(scan=scan, first_id=offset, points=int(len(q)),
                                matrix=mlp[scan] if merged else None))
            files.append(dict(member='%s/dslr_scan_eval/%s' % (scene, scan),
                              sha256=common.sha256_file(folder / scan), bytes=(folder / scan).stat().st_size))
            offset += len(q)
        q = np.concatenate(parts)
        result = common.prepare_outputs(args.out, name, q, ids=np.concatenate(ids))
        source = dict(url=URL, archive=dl, members=files,
                      alignment=dict(member='%s/dslr_scan_eval/scan_alignment.mlp' % scene,
                                     sha256=common.sha256_file(folder / 'scan_alignment.mlp')) if merged else None)
        conversion = dict(source_type='PLY binary little endian, float32 x y z (metres)',
                          stations=offsets, merged=merged,
                          transform=('4x4 MeshLab matrices read as decimal, applied in float64 elementwise '
                                     '((r0 x + r1 y) + r2 z) + t, then exact 1 mm grid') if merged else
                          'none (scanner frame)',
                          ids='station offset + index in the station PLY', ground='not removed (no classification)')
        manifest = common.scene_manifest(name, 'multi_millions', DATASET, source, conversion, result, __file__)
        common.write_json(args.out / (name + '.manifest.json'), manifest)
        entries.append(lasconv.set_entry(manifest))
        me = result['measures']
        common.log('%s : %d points, %d distincts, %d bits' % (name, me['n_points'], me['n_distinct'],
                                                               me['bits_needed']))
    manifest_path = args.out / 'manifest.json'
    old = json.loads(manifest_path.read_text()) if manifest_path.is_file() else {}
    merged_cases = {c['name']: c for c in old.get('cases', [])}
    merged_cases.update({e['name']: e for e in entries})
    out = dict(schema=common.SCHEMA_SET, family='multi_millions', dataset=DATASET,
               cases=[merged_cases[k] for k in sorted(merged_cases)], generator=common.generator_info(__file__),
               created_utc=common.utc_now(), public_status='not_claimed')
    if old.get('crops'):
        out['crops'] = old['crops']
    common.write_json(manifest_path, out)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
