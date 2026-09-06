#!/usr/bin/env python3
"""Copy the closed positive-core supplement; no test is executed here."""
import hashlib
import json
from pathlib import Path

ROOT = Path('/workspaces/E-HGP')
SOURCE = ROOT / 'build/v7_wspd_q2_positive_core_20260906_r1'
DEST = ROOT / 'morsehgp3D_v7/receipts/wspd_q2_positive_core_20260906'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def main():
    if DEST.exists():
        raise ValueError('destination exists; no overwrite')
    receipt_raw = (SOURCE / 'receipt.json').read_bytes()
    receipt = json.loads(receipt_raw)
    if receipt['status'] != 'completed' or not receipt['cpp_closed']:
        raise ValueError('capture not successfully closed; keep private failure unchanged')
    payload, excluded = {}, {}
    for path in sorted(SOURCE.rglob('*')):
        if path.is_symlink():
            raise ValueError('source symlink')
        if not path.is_file():
            continue
        name = str(path.relative_to(SOURCE))
        raw = path.read_bytes()
        if name.startswith('bin/'):
            mode = name.removeprefix('bin/')
            if mode not in receipt['binaries'] or sha(raw) != receipt['binaries'][mode]:
                raise ValueError('unexpected binary identity')
            excluded[name] = dict(sha256=sha(raw), bytes=len(raw), original_preserved=True)
        else:
            if raw.startswith(b'\x7fELF'):
                raise ValueError('unlisted ELF')
            payload[name] = raw
    if len(excluded) != 3:
        raise ValueError('three exact binary omissions required')
    payload['excluded_binaries.json'] = encoded(excluded)
    payload['publish.py'] = Path(__file__).read_bytes()
    payload['README.md'] = ('''# Cœur q2 positif : supplément permanent — 6 septembre 2026

Le gate `tests/wspd_terminal_reuse_gate.cpp` ajoute une exigence locale dans
la fixture existante scène 1, s=8, masque q2 seul, threshold=1 (h₂=10) :
le rectangle ordonné (-1,-3) est trouvé exactement une fois et son cœur q2
vaut 1. Le compteur explicite `q2_positive_core_checks` doit valoir 1.

O2 et ASan/UBSan terminent chacun avec 174 appels, six refus intentionnels
et un contrôle positif ; leurs stdout sont identiques. `--unknown` rend 2
sans sortie dans les deux modes. LeakSanitizer est activé dans l'environnement
capturé ; ce supplément ne prétend pas requalifier un CTest global.

Le mutant physique privé enlève seulement `ff.c[0]=fc.c[0]` dans sa copie
de generate.hpp. Il compile, puis le gate rend 1 avec la première cause exacte
`wspd q2 front rejected: line.q2_positive_core_value`. Son stdout partiel est
conservé tel quel. Aucun switch mutant n'est ajouté au produit.

Neuf commandes fermées sont conservées, dont trois compilations et l'identité
du compilateur. Les 27 dépendances projet sont copiées pour chaque bras ; les
depfiles et hashes avant/après les relient aux compilations. Le fichier nominal
generate.hpp reste inchangé (`345129a7…`). Trois ELF sont omis avec leurs
hashes dans `excluded_binaries.json` ; leurs originaux privés sont conservés.

Ce supplément concerne le nouveau gate `35d28f2c…`. Les premiers CTests et
anciens reçus restent attachés au gate antérieur `81a8657a…` : ils ne sont pas
réétiquetés comme qualification de ce delta. C'est un renforcement de test,
pas la correction d'un défaut nominal constaté, ni une mesure de performance.

`public_status=not_claimed`, CPU/u16. Aucune complétude géométrique ni aucun
contrat de temps nouvellement certifié. Aucune VM GCP utilisée.
Vérification : `sha256sum -c SHA256SUMS` depuis ce dossier.
''').encode()
    payload['publication.json'] = encoded(dict(original_directory=str(SOURCE), receipt_sha256=sha(receipt_raw),
        copied_bytes_unchanged=True, public_status='not_claimed', engine_invoked_by_publication=False))
    DEST.mkdir()
    for name, raw in sorted(payload.items()):
        path = DEST / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as out:
            out.write(raw)
        if path.read_bytes() != raw:
            raise ValueError('copy mismatch')
    manifest = ''.join(sha(raw) + '  ' + name + '\n' for name, raw in sorted(payload.items())).encode()
    with (DEST / 'SHA256SUMS').open('xb') as out:
        out.write(manifest)
    print(json.dumps(dict(status='copied', files=len(payload) + 1, SHA256SUMS=sha(manifest),
                         receipt_sha256=sha(receipt_raw)), sort_keys=True))


if __name__ == '__main__':
    main()
