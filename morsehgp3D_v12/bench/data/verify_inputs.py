#!/usr/bin/env python3
"""Verifie un dossier d'entrees prepare (sur la VM G4 ou en local) contre son manifeste d'ensemble.

    python3 -S verify_inputs.py DOSSIER/manifest.json [--measure]

Admission du manifeste AVANT toute lecture de fichier (CST-0217) : objet JSON au schema mhgp12.benchmark_inputs.v1,
liste `cases` non vide (et `crops`, facultative, en liste) ; pour chaque fichier liste (coordonnees, identifiants,
multiplicites ; variante `distinct` comprise), un nom simple [A-Za-z0-9][A-Za-z0-9._-]* (jamais un chemin), une
empreinte SHA-256 de 64 chiffres hexadecimaux minuscules et un compte de sites entier strictement positif ; une variante
`distinct` porte toujours ses multiplicites et leur empreinte, une multiplicite listee porte toujours son empreinte ;
`bundled` vaut `distinct` ou est absent ; un meme nom liste deux fois a la meme empreinte et la meme taille. Un
manifeste vide, un fichier sans empreinte, une empreinte nulle ou mal formee rendent le code 2 sans qu'aucun fichier
soit lu : jamais un code 0 sans preuve complete.
Puis, bibliotheque standard seule (Python 3.10 nu de la VM) : existence, tailles (12 octets par site, 4 par
identifiant ou multiplicite) et SHA-256 de chaque fichier liste. Avec --measure (numpy requis) : recompte n, positions
distinctes, doublons, etendue et bits, et les compare au manifeste.
Codes : 0 conforme (au moins un fichier verifie) ; 1 ecart (fichier absent, taille, empreinte, mesure) ; 2 manifeste
illisible ou non conforme, usage.
"""
from __future__ import annotations

import hashlib
import json
import re
import sys
from pathlib import Path

SCHEMA = 'mhgp12.benchmark_inputs.v1'
NAME = re.compile(r'[A-Za-z0-9][A-Za-z0-9._-]*')
SHA256 = re.compile(r'[0-9a-f]{64}')


class Refus(Exception):
    """Manifeste non conforme : refus avant toute lecture de fichier (code 2)."""


def need(condition, message):
    if not condition:
        raise Refus(message)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 22), b''):
            digest.update(block)
    return digest.hexdigest()


def file_entry(record: dict, name_key: str, digest_key: str, per_site: int, count: int, where: str):
    name, digest = record.get(name_key), record.get(digest_key)
    need(isinstance(name, str) and NAME.fullmatch(name) is not None,
         '%s : %s doit etre un nom de fichier simple, pas %r' % (where, name_key, name))
    need(isinstance(digest, str) and SHA256.fullmatch(digest) is not None,
         '%s : %s doit etre une empreinte SHA-256 (64 chiffres hexadecimaux minuscules), pas %r'
         % (where, digest_key, digest))
    return name, digest, per_site * count


def variant_files(record: dict, where: str, mult_required: bool) -> list:
    """Fichiers d'une variante (cas, decoupe ou variante distincte) : coordonnees, identifiants, multiplicites."""
    need(isinstance(record, dict), '%s : objet attendu' % where)
    count = record.get('count')
    need(type(count) is int and count > 0, '%s : count doit etre un entier strictement positif, pas %r'
         % (where, count))
    files = [file_entry(record, 'coordinates', 'sha256', 12, count, where),
             file_entry(record, 'point_ids', 'ids_sha256', 4, count, where)]
    if mult_required or 'mult' in record or 'mult_sha256' in record:
        files.append(file_entry(record, 'mult', 'mult_sha256', 4, count, where))
    return files


def files_of(case, where: str) -> list:
    need(isinstance(case, dict), '%s : objet attendu' % where)
    name = case.get('name')
    need(isinstance(name, str) and name != '', '%s : nom de cas absent' % where)
    where = '%s %s' % (where, name)
    bundled = case.get('bundled')
    need(bundled in (None, 'distinct'), '%s : bundled inconnu %r' % (where, bundled))
    if bundled == 'distinct':  # paquet G4 : seule la variante distincte a ete envoyee
        need('distinct' in case, '%s : variante distincte annoncee mais absente' % where)
        return variant_files(case['distinct'], where + ' (distinct)', True)
    files = variant_files(case, where, False)
    if 'distinct' in case:
        files += variant_files(case['distinct'], where + ' (distinct)', True)
    return files


def admit(manifest) -> list:
    """Admission : liste des (cas, fichiers) ; leve Refus au premier ecart de forme, avant toute lecture."""
    need(isinstance(manifest, dict), 'manifeste : objet JSON attendu')
    need(manifest.get('schema') == SCHEMA, 'manifeste : schema %r, attendu %s' % (manifest.get('schema'), SCHEMA))
    cases = manifest.get('cases')
    need(isinstance(cases, list) and len(cases) > 0, 'manifeste : liste cases absente ou vide')
    crops = manifest.get('crops', [])
    need(isinstance(crops, list), 'manifeste : crops doit etre une liste')
    admitted, seen = [], {}
    for where, items in (('cas', cases), ('decoupe', crops)):
        for index, case in enumerate(items):
            files = files_of(case, '%s %d' % (where, index))
            for name, digest, size in files:
                if name in seen:
                    need(seen[name] == (digest, size), 'fichier %s liste deux fois avec deux empreintes ou tailles'
                         % name)
                seen[name] = (digest, size)
            admitted.append((case, files))
    need(len(seen) > 0, 'manifeste : aucun fichier a verifier')
    return admitted


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
    options = [a for a in argv if a.startswith('-')]
    paths = [a for a in argv if not a.startswith('-')]
    if len(paths) != 1 or any(o != '--measure' for o in options):
        print(__doc__)
        return 2
    manifest_path = Path(paths[0])
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, ValueError) as error:
        print('manifeste illisible : %s' % error)
        return 2
    try:
        admitted = admit(manifest)
    except Refus as error:
        print('REFUS %s : %s (aucun fichier lu)' % (manifest_path, error))
        return 2
    if '--measure' in options:
        try:
            import numpy  # noqa: F401  (mesure seulement)
        except ImportError:
            print('--measure exige numpy')
            return 2
    root = manifest_path.parent
    bad = 0
    checked = set()
    for case, files in admitted:
        for name, digest, size in files:
            if name in checked:
                continue
            checked.add(name)
            path = root / name
            if not path.is_file():
                print('ABSENT %s' % name)
                bad += 1
                continue
            if path.stat().st_size != size:
                print('TAILLE %s : %d au lieu de %d' % (name, path.stat().st_size, size))
                bad += 1
            elif sha256_file(path) != digest:
                print('EMPREINTE %s' % name)
                bad += 1
        if '--measure' in options:
            if case.get('bundled') == 'distinct':
                d = case['distinct']
                probe = dict(name=case['name'], count=d['count'], duplicate_sites=0)
                target = root / d['coordinates']
            else:
                probe, target = case, root / case['coordinates']
            gaps = measure(target, probe) if target.is_file() else ['coordonnees absentes']
            if gaps:
                print('MESURE %s : %s' % (case['name'], ', '.join(gaps)))
                bad += 1
    print('verify_inputs %s : %d cas, %d fichiers, %d ecarts' % (manifest_path, len(admitted), len(checked), bad))
    return 1 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))
