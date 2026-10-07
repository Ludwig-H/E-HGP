#!/usr/bin/env python3
"""Assemble un dossier PLAT de donnees pour une session G4 gardee (contraintes du lanceur v11 :
noms [A-Za-z0-9][A-Za-z0-9._-]*, au plus 512 fichiers, au plus 8 Gio, aucun fichier vide, .u32le multiple de 4).

    python3 -S make_g4_bundle.py --out DOSSIER MANIFESTE.json [MANIFESTE.json ...] [--only NOM ...]
        [--with-labels] [--with-distinct] [--with-inverse] [--prefer-distinct] [--with-crops] [--copy]

Liens physiques par defaut (meme systeme de fichiers), sinon copie. Ecrit `bundle_manifest.json` (les cas retenus,
au format du manifeste d'ensemble, chemins relatifs au dossier) et `SHA256SUMS`. Bibliotheque standard seule.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
from pathlib import Path

NAME = re.compile(r'[A-Za-z0-9][A-Za-z0-9._-]*')
MAX_FILES, MAX_BYTES = 512, 8 * 2 ** 30


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 22), b''):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('manifests', type=Path, nargs='+')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--only', nargs='*', default=None)
    parser.add_argument('--with-labels', action='store_true')
    parser.add_argument('--with-distinct', action='store_true')
    parser.add_argument('--with-inverse', action='store_true')
    parser.add_argument('--copy', action='store_true')
    parser.add_argument('--prefer-distinct', action='store_true',
                        help='scene avec doublons : seulement la variante .distinct (+ .mult), pas le fichier brut')
    parser.add_argument('--with-crops', action='store_true', help='ajoute les decoupes (cle crops du manifeste)')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    chosen, files = [], []
    for manifest_path in args.manifests:
        manifest = json.loads(manifest_path.read_text())
        root = manifest_path.parent
        cases = list(manifest['cases']) + (list(manifest.get('crops', [])) if args.with_crops else [])
        for case in cases:
            if args.only and case['name'] not in args.only:
                continue
            if args.prefer_distinct and case.get('distinct'):
                d = case['distinct']
                for name in (d['coordinates'], d['point_ids'], d['mult']):
                    files.append((root / name, name))
                chosen.append(dict(case, bundled='distinct'))
                continue
            names = [case['coordinates'], case['point_ids']]
            if case.get('mult') and case['mult'] not in names:
                names.append(case['mult'])
            stem = case['coordinates'][:-len('.u32le')]
            if args.with_labels and (root / (stem + '.labels.u32le')).is_file():
                names.append(stem + '.labels.u32le')
            if (root / (stem + '.mult.u32le')).is_file() and stem + '.mult.u32le' not in names:
                names.append(stem + '.mult.u32le')
            if args.with_distinct and case.get('distinct'):
                d = case['distinct']
                names += [d['coordinates'], d['point_ids'], d['mult']]
                if args.with_inverse:
                    names.append(d['coordinates'][:-len('.u32le')] + '.inverse.u32le')
            for name in names:
                if not NAME.fullmatch(name) or name == 'SHA256SUMS':
                    raise SystemExit('nom refuse par le lanceur : ' + name)
                files.append((root / name, name))
            chosen.append(case)
    total = sum(src.stat().st_size for src, _ in files)
    if len(files) + 2 > MAX_FILES or total > MAX_BYTES:
        raise SystemExit('paquet hors limites : %d fichiers, %d octets' % (len(files) + 2, total))
    sums = []
    for src, name in files:
        dst = args.out / name
        if dst.exists():
            dst.unlink()
        if args.copy:
            shutil.copyfile(src, dst)
        else:
            try:
                os.link(src, dst)
            except OSError:
                shutil.copyfile(src, dst)
        sums.append('%s  %s\n' % (sha256_file(dst), name))
    bundle = dict(schema='mhgp12.benchmark_inputs.v1', scope='flat G4 bundle', cases=chosen,
                  sources=[str(p) for p in args.manifests], files=len(files), bytes=total)
    (args.out / 'bundle_manifest.json').write_text(json.dumps(bundle, indent=1, sort_keys=True) + '\n')
    (args.out / 'SHA256SUMS.txt').write_text(''.join(sums))
    print('bundle %s : %d cas, %d fichiers, %.1f Mo' % (args.out, len(chosen), len(files) + 2, total / 1e6))
    return 0


if __name__ == '__main__':
    sys.exit(main())
