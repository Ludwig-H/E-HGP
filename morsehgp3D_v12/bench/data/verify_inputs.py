#!/usr/bin/env python3
"""Verifie un dossier d'entrees prepare (sur la VM G4 ou en local) contre son manifeste d'ensemble.

    python3 -S verify_inputs.py DOSSIER/manifest.json [--measure]

Sans --measure : bibliotheque standard seule (Python 3.10 nu de la VM) : existence, tailles (12 octets par site,
4 par identifiant) et SHA-256 de chaque fichier liste. Avec --measure (numpy requis) : recompte n, positions
distinctes, doublons, etendue et bits, et les compare au manifeste.
Codes : 0 conforme ; 1 ecart ; 2 manifeste illisible.
"""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 22), b''):
            digest.update(block)
    return digest.hexdigest()


def files_of(case: dict):
    if case.get('bundled') == 'distinct':  # paquet G4 : seule la variante distincte a ete envoyee
        d = case['distinct']
        yield d['coordinates'], d['sha256'], 12 * d['count']
        yield d['point_ids'], d['ids_sha256'], 4 * d['count']
        yield d['mult'], d.get('mult_sha256'), 4 * d['count']
        return
    yield case['coordinates'], case['sha256'], 12 * case['count']
    yield case['point_ids'], case['ids_sha256'], 4 * case['count']
    if case.get('mult') and case.get('mult_sha256'):  # decoupes : multiplicites de la source
        yield case['mult'], case['mult_sha256'], 4 * case['count']
    d = case.get('distinct')
    if d:
        yield d['coordinates'], d['sha256'], 12 * d['count']
        yield d['point_ids'], d['ids_sha256'], 4 * d['count']
        if d.get('mult'):
            yield d['mult'], d.get('mult_sha256'), 4 * d['count']


def measure(path: Path, case: dict) -> list:
    import numpy as np
    u = np.fromfile(path, '<u4').reshape(-1, 3).astype(np.uint64)
    gaps = []
    if len(u) != case['count']:
        gaps.append('count')
    ext = (u.max(axis=0) - u.min(axis=0)).astype(np.int64).tolist()
    if 'extent_mm' in case and ext != case['extent_mm']:
        gaps.append('extent')
    if u.min(axis=0).tolist() != [0, 0, 0]:
        gaps.append('translation (min != 0)')
    bits = max(int(e).bit_length() for e in ext)
    if 'bits_needed' in case and bits != case['bits_needed']:
        gaps.append('bits')
    key = (u[:, 0] << np.uint64(42)) | (u[:, 1] << np.uint64(21)) | u[:, 2] if bits <= 21 else None
    if key is not None:
        distinct = len(np.unique(key))
    else:
        distinct = len(np.unique(u, axis=0))
    if distinct != case['count'] - case.get('duplicate_sites', 0):
        gaps.append('duplicates (%d distinct)' % distinct)
    return gaps


def main(argv) -> int:
    if not argv:
        print(__doc__)
        return 2
    manifest_path = Path(argv[0])
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, ValueError) as error:
        print('manifeste illisible : %s' % error)
        return 2
    root = manifest_path.parent
    bad = 0
    checked = 0
    cases = list(manifest['cases']) + list(manifest.get('crops', []))
    for case in cases:
        for name, digest, size in files_of(case):
            path = root / name
            checked += 1
            if not path.is_file():
                print('ABSENT %s' % name)
                bad += 1
                continue
            if path.stat().st_size != size:
                print('TAILLE %s : %d au lieu de %d' % (name, path.stat().st_size, size))
                bad += 1
            elif digest is not None and sha256_file(path) != digest:
                print('EMPREINTE %s' % name)
                bad += 1
        if '--measure' in argv:
            if case.get('bundled') == 'distinct':
                d = case['distinct']
                probe = dict(name=case['name'], count=d['count'], duplicate_sites=0)
                gaps = measure(root / d['coordinates'], probe)
            else:
                gaps = measure(root / case['coordinates'], case)
            if gaps:
                print('MESURE %s : %s' % (case['name'], ', '.join(gaps)))
                bad += 1
    print('verify_inputs %s : %d cas, %d fichiers, %d ecarts' % (manifest_path, len(cases), checked, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))
