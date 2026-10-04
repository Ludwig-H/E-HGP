#!/usr/bin/env python3
"""Déballe sur la VM une archive tar de scènes : trames LiDAR au format de points_campaign.py (mode lidar) ou
scènes synthétiques gelées de points_scenes_freeze.py.

    python3 points_unpack.py --archive DATA/scenes.tar --out BUILD/scenes

La session gardée limite le nombre de fichiers de données (512) : un lot de scènes nombreuses et petites (bouts de
scène) voyage donc en une seule archive. Seuls des fichiers réguliers à nom simple sont acceptés (ni chemin absolu, ni
sous-dossier, ni lien) ; les empreintes sha256 des sites et des étiquettes de chaque scène sont ensuite vérifiées contre
le manifeste points_manifest.json de l'archive (LiDAR) ou contre les manifestes <ensemble>_<taille>.manifest.json
(scènes gelées : taille et sha256 de chaque fichier). Code 0 si tout concorde, 2 sinon.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tarfile


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    count = 0
    with tarfile.open(args.archive) as tar:
        for member in tar.getmembers():
            name = member.name
            if not member.isfile() or '/' in name or '\\' in name or name in ('', '.', '..') or name.startswith('.'):
                print('refus : membre %r' % name, file=sys.stderr)
                return 2
            data = tar.extractfile(member).read()
            (args.out / name).write_bytes(data)
            count += 1
    bad = []
    if (args.out / 'points_manifest.json').is_file():  # trames LiDAR
        manifest = json.loads((args.out / 'points_manifest.json').read_text())
        for scene in manifest['scenes']:
            for suffix, key in (('_sites.u32le', 'sites_sha256'), ('_labels.u32le', 'labels_sha256')):
                path = args.out / (scene['name'] + suffix)
                if not path.is_file() or sha(path) != scene.get(key):
                    bad.append(scene['name'] + suffix)
        scenes = len(manifest['scenes'])
    else:  # scenes synthetiques gelees (bench/points_scenes_freeze.py) : <ensemble>_<taille>.manifest.json
        manifests = sorted(args.out.glob('*.manifest.json'))
        if not manifests:
            print('refus : aucun manifeste', file=sys.stderr)
            return 2
        scenes = 0
        for path in manifests:
            for scene in json.loads(path.read_text())['scenes']:
                scenes += 1
                for entry in scene['files'].values():
                    target = args.out / entry['name']
                    if not target.is_file() or target.stat().st_size != entry['bytes'] or \
                            sha(target) != entry['sha256']:
                        bad.append(entry['name'])
    print('points_unpack fichiers %d scenes %d ecarts %d' % (count, scenes, len(bad)))
    return 2 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main())
