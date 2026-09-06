#!/usr/bin/env python3
"""Copy only the closed v5 R1 captures; preserve their failure without replay."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_no_work_quotas_20260906_controller'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_probe_no_quotas_20260906'
PINS = {
    'build_r1': '25ccae8eb8466280568090963314c9a67a579bc5d81aa43871830a13a6fd7e9d',
    'micro_r1': 'd5d95a5036f89532ce91aeddda4cec5d5d36ba41909e9198b7649b449f6ae279',
}
SOURCE_SHA = '72a147fec063382013b4c61a1990ad163cd99dbe9051a8c170c417797b519fbe'
EXCLUDED = {
    'build_r1/full_gabriel_lazy_probe': '4938b94b3166e8c13d02b0fd9687168130d5c528702d7efcf7b7379b3adeb360',
    'micro_r1/cmake/CMakeFiles/3.28.3/CMakeDetermineCompilerABI_CXX.bin': '879b5d31eac14d7b0ca6cd3b96320041ad8d87b538fea24a17e8707f0656bc96',
    'micro_r1/cmake/CMakeFiles/3.28.3/CompilerIdCXX/a.out': 'a3d366f5f634a9cc0d9340115704a2d978e930f75ce0aceac02d7177849a8a1a',
    'micro_r1/cmake/CMakeFiles/mhgp7_full_gabriel_probe_limits_gate.dir/tests/full_gabriel_probe_limits_gate.cpp.o': 'bd5cd38705ab9ec60ec3fbc6e7911954aa8601c06418c18df5878600dfbacc0d',
    'micro_r1/cmake/mhgp7_full_gabriel_probe_limits_gate': 'edad9391a6063c8864980072e9aeeaec41b339a97a356620e215b14e3b6c98cb',
    'micro_r1/limits_O2': '4443936361da1e394920098478eb02fa019d73a5a0cb2fbfa874d9ff0f37a7ac',
    'micro_r1/limits_san': 'df01c44f3d5368fab6913a0aedd18c12f53db381d14bd62d6f67b6e039c95cac',
    'micro_r1/local_O2': '1e3fc91a97732d16f7258bfcff6ff782d5efdc05e4a51b392c131c2460bedb21',
    'micro_r1/local_san': 'e26f311de08d0f778b18833cfa11f96531e654ae15601f5ff30b1b896d4afb0f',
    'micro_r1/q4_before_q3/local_mutant': '5727aaa1b1b5b4bbb598a64e54e667bbe8742eb60850026fb133bb5e4ede12d0',
}
README = """# Sonde FULL v5 sans quotas de travail — captures R1 conservées

`public_status=not_claimed`, CPU de référence, entrée quantifiée u16. Ce paquet
conserve les octets des captures closes ; il ne rejoue aucun test et ne répare
aucun ancien reçu. Les chemins absolus capturés restent des identifiants du
poste d'origine. Les sources effectivement consommées sont dans `source_snapshot/`.

- `build_r1/` : compilation terminée, code 0 ; quatre commandes capturées,
  sans exécution scientifique dans cette étape.
- `micro_r1/` : campagne **failed**, 225 commandes et 37 tentatives scientifiques.
  Les portes des limites ont passé 52 contrôles en O2 et en ASan/UBSan ; les
  six CTests ciblés ont passé. Les autres portes locales, leurs rejets et le
  mutant privé sont également conservés avec leurs sorties.
- 36 micros Kmax=5 sont validés par le contrôleur (180 ordres horizontaux).
  La première tentative Kmax=10, `n8_s8_k10_eager_c0_p0`, a produit son brut
  et un code moteur 0, mais son verdict de capture est **failed** :
  `KeyError: 'successor_accounting'`. L'auto-test first-C v5 omettait ce champ
  de version exigé par le contrôleur. Ce résultat n'est pas promu en micro validé.

Il ne s'agit donc **ni de 77 micros/503 ordres qualifiés, ni d'une campagne
complète**, ni d'un contrat 50k ou d'une tour inter-K intégrée. Les extensions
de domaine prévues n'ont pas été exécutées. Les benchmarks directs ultérieurs
ne figurent pas dans ce paquet.

Après la clôture, une seule ligne a ajouté ce champ au lecteur first-C v5,
SHA `d51ee4e4ad8cdcb33d86f721dd7bf6e48a11cd730d1373d0845656fa69a1ee5f`.
Un petit modèle Python séparé a passé en normal et sous `-O` (codes 0, sorties
identiques, 40/25 mutants, six champs de métadonnées par lecteur). Ce contrôle
ponctuel n'a pas relancé la campagne R1 et ne change aucun de ses verdicts.
La source first-C R1 demeure dans le snapshot avec son hash `a57e54c4…`.

`publication.json` associe les deux reçus d'origine à ce paquet.
`excluded_binaries.json` énumère exactement dix exécutables/objets absents,
avec leurs hashes vérifiés lors de la copie. Les originaux privés n'ont pas
été supprimés ou modifiés. Toutes les autres captures, y compris les fichiers
CMake de provenance et les sources du mutant privé, sont copiées à l'identique.
`SHA256SUMS` couvre tous les fichiers publiés sauf lui-même ; vérification :
`sha256sum -c SHA256SUMS` depuis ce dossier. Aucun GCP utilisé pour cette copie.
"""


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def files(directory):
    paths = list(directory.rglob('*'))
    require(not directory.is_symlink() and all(not p.is_symlink() for p in paths), 'symlink')
    return {str(p.relative_to(directory)): p for p in paths if p.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--publish', action='store_true')
    args = parser.parse_args()
    require(args.publish, 'explicit --publish required; no files written')
    require(not DEST.exists(), 'destination_must_be_new')
    copies, watched, omissions, receipts = {}, {}, {}, {}
    for run, receipt_pin in PINS.items():
        directory = BASE / run
        inventory = files(directory)
        receipt_raw = inventory['receipt.json'].read_bytes()
        require(sha(receipt_raw) == receipt_pin, 'receipt_pin:' + run)
        receipt = json.loads(receipt_raw)
        receipts[run] = receipt
        expected = dict(receipt['artifacts'], **{'receipt.json': receipt_pin})
        require(set(inventory) == set(expected), 'closed_inventory:' + run)
        for name, path in inventory.items():
            raw, target = path.read_bytes(), run + '/' + name
            require(sha(raw) == expected[name], 'capture_hash:' + target)
            watched[path] = sha(raw)
            if target in EXCLUDED:
                require(sha(raw) == EXCLUDED[target] and raw.startswith(b'\x7fELF'), 'excluded_identity:' + target)
                omissions[target] = dict(sha256=sha(raw), bytes=len(raw), original_preserved=True)
            else:
                require(not raw.startswith(b'\x7fELF'), 'unlisted_binary:' + target)
                copies[target] = raw
    require(set(omissions) == set(EXCLUDED), 'exclusion_inventory')
    require(receipts['build_r1']['status'] == 'completed' and receipts['micro_r1']['status'] == 'failed', 'preserve_status')
    snapshot = json.loads(copies['build_r1/sources_before.json'])
    require(snapshot['source_map_sha256'] == SOURCE_SHA and sha(encoded(snapshot['files'])) == SOURCE_SHA, 'source_map')
    source_files = files(BASE / 'build_r1_sources/source_snapshot')
    require(set(source_files) == set(snapshot['files']), 'source_inventory')
    for name, path in source_files.items():
        raw = path.read_bytes()
        require(sha(raw) == snapshot['files'][name], 'source_hash:' + name)
        copies['source_snapshot/' + name] = raw
        watched[path] = sha(raw)
    copies['README.md'] = README.encode()
    copies['excluded_binaries.json'] = encoded(omissions)
    copies['publication.json'] = encoded(dict(schema='mhgp7-v5-R1-preserved-captures-v1',
        original_directory=str(BASE), receipt_sha256=PINS, source_map_sha256=SOURCE_SHA,
        build_status='completed', micro_status='failed', copied_capture_bytes_unchanged=True,
        engine_invoked=False, public_status='not_claimed', excluded_files=len(omissions)))
    copies['publish_r1.py'] = Path(__file__).read_bytes()
    DEST.mkdir()
    for name, raw in sorted(copies.items()):
        target = DEST / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as output:
            output.write(raw)
        require(sha(target.read_bytes()) == sha(raw), 'copied_hash:' + name)
    for path, expected in watched.items():
        require(sha(path.read_bytes()) == expected, 'source_changed_during_copy')
    manifest = ''.join(sha(raw) + '  ' + name + '\n' for name, raw in sorted(copies.items())).encode()
    with (DEST / 'SHA256SUMS').open('xb') as output:
        output.write(manifest)
    print(json.dumps(dict(status='copied', destination=str(DEST), files=len(copies) + 1,
        excluded=len(omissions), manifest_sha256=sha(manifest), engine_invoked=False), sort_keys=True))


if __name__ == '__main__':
    main()
