#!/usr/bin/env python3
"""IGN LiDAR HD (France, aerien, Licence Ouverte Etalab 2.0) -> format v11 a 1 mm (famille b, multi-millions).

    python3 -I prepare_ign_lidarhd.py --raw DIR --out DIR [--tiles NOM ...] [--variants tout sans_sol]

Dalles COPC LAZ 1.4 de 1 km x 1 km (Lambert-93 / IGN69), classees (sol = 2, batiment = 6, vegetation 3-5, bruit
65/66 chez l'IGN), telechargees directement sur data.geopf.fr (aucun formulaire), URL et nombre de points lus dans
l'index WFS IGNF_LIDAR-HD_TA:nuage-dalle. Empreintes SHA-256 epinglees au premier telechargement (7 oct. 2026).
La lecture LAZ exige laspy + lazrs (v12data/formats/laz.py) ; tout le reste est numpy.
Variantes : `tout` (tous les retours LiDAR de la dalle : seule la classe 66, points virtuels ajoutes par l'IGN sous
les ponts, est retiree) et `sans_sol` (classe 2 retiree en plus). Doublons au mm : fichier brut
`<nom>` + `<nom>.distinct` (positions distinctes, multiplicites), decision D8.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from v12data import common, lasconv  # noqa: E402

BASE = 'https://data.geopf.fr/telechargement/download/LiDARHD-NUALID/'
DATASET = dict(name='IGN LiDAR HD (nuages classes, NUALHD 1-0)', sensor='ALS, avion (lidar aerien)',
               licence='Licence Ouverte / Open Licence Etalab 2.0 (attribution : IGN)',
               licence_url='https://geoservices.ign.fr/lidarhd ; https://www.etalab.gouv.fr/licence-ouverte-open-licence/',
               access='telechargement direct HTTPS data.geopf.fr, sans compte ; index WFS data.geopf.fr/wfs '
                      '(couche IGNF_LIDAR-HD_TA:nuage-dalle)',
               density='nominale >= 10 impulsions/m2 ; 6 a 45 M de retours par dalle de 1 km2 selon le lot')
# classes propres a l'IGN : 64 sursol perenne, 65 artefacts (non utilise), 66 points virtuels (ajoutes sous les ponts,
# pas des retours LiDAR : retires de toutes les variantes), 67 divers batis (DC_LiDAR_HD_1-0_PTS, geoservices.ign.fr)
EXCLUDE = {66: 'IGN virtual points (added under bridges, not LiDAR returns)'}
CLASSES = {1: 'non classe', 2: 'sol', 3: 'vegetation basse', 4: 'vegetation moyenne', 5: 'vegetation haute',
           6: 'batiment', 9: 'eau', 17: 'tablier de pont', 64: 'sursol perenne', 65: 'artefacts',
           66: 'points virtuels', 67: 'divers batis'}  # DC_LiDAR_HD_1-0_PTS (geoservices.ign.fr)
TILES = {
    # nom : (chemin relatif, octets, sha256 epingle, points annonces par le WFS, metadonnees)
    'ign_marseille_0891_6248': ('NUALHD_1-0__LAZ_LAMB93_PQ_2025-03-14/LHD_FXX_0891_6248_PTS_LAMB93_IGN69.copc.laz',
                                32023506, '153603760509f3159c736be5167cd80cd5ac55b3200b353218b2b12de6576af5', 6712268,
                                dict(city='Marseille (littoral)', mission='21LHD7PQ', sensor='Leica TerrainMapper',
                                     acquisition='2021-06-26')),
    'ign_paris_0651_6863': ('NUALHD_1-0__LAZ_LAMB93_KE_2025-06-06/LHD_FXX_0651_6863_PTS_LAMB93_IGN69.copc.laz',
                            94075634, 'd6deddeea7bd065cdefb51bc77e44544884fa3abc60f151f73e4c5a16261260d', 14553289,
                            dict(city='Paris (centre)', mission='22LHDKE', sensor='Leica TerrainMapper',
                                 acquisition='2023-03-03')),
    'ign_lyon_0842_6521': ('NUALHD_1-0__LAZ_LAMB93_OL_2025-02-20/LHD_FXX_0842_6521_PTS_LAMB93_IGN69.copc.laz',
                           171404492, '9e27e8a9a52713fc89a412fd188eac9e487391a4299ed404ea60cf563d780569', 32429318,
                           dict(city='Lyon (centre)', mission='21LHD6OL', sensor='RIEGL VQ-780 II-S',
                                acquisition='2021-04-14..2021-09-24')),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--raw', type=Path, required=True, help='dossier des fichiers telecharges (non fiables)')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--tiles', nargs='*', default=list(TILES))
    parser.add_argument('--variants', nargs='*', default=['tout', 'sans_sol'])
    args = parser.parse_args()
    args.raw.mkdir(parents=True, exist_ok=True)
    args.out.mkdir(parents=True, exist_ok=True)
    common.selftest_quantize()
    entries = []
    for name in args.tiles:
        rel, size, digest, announced, meta = TILES[name]
        url = BASE + rel
        dl = common.download(url, args.raw / Path(rel).name, expected_sha256=digest, expected_size=size)
        common.log('%s : %s (%d octets, sha256 %s)' % (name, 'telecharge' if dl['downloaded'] else 'present',
                                                      dl['bytes'], dl['sha256']))
        source = dict(url=url, file=dl, announced_points=announced, tile=meta,
                      pin='sha256 pinned on first download (7 Oct 2026)' if digest else 'NOT PINNED (first use)')
        for m in lasconv.convert(args.raw / Path(rel).name, args.out, name, DATASET, source, __file__,
                                 variants=tuple(args.variants), exclude_always=EXCLUDE, class_names=CLASSES):
            if m['file_points'] != announced and m['conversion']['variant'] == 'tout':  # index WFS : autre edition
                common.log('%s : %d points lus, %d annonces par le WFS' % (name, m['file_points'], announced))
            entry = lasconv.set_entry(m)
            entry['announced_points_wfs'] = announced
            entries.append(entry)
    manifest_path = args.out / 'manifest.json'
    old = json.loads(manifest_path.read_text())['cases'] if manifest_path.is_file() else []
    merged = {c['name']: c for c in old}
    merged.update({e['name']: e for e in entries})
    common.write_json(manifest_path, dict(schema=common.SCHEMA_SET, family='multi_millions', dataset=DATASET,
                                          cases=[merged[k] for k in sorted(merged)],
                                          generator=common.generator_info(__file__), created_utc=common.utc_now(),
                                          public_status='not_claimed'))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
