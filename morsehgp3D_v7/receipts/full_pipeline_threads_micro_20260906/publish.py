#!/usr/bin/env python3
"""Copy the closed micro capture once; no engine or compiler invocation."""
import hashlib
import json
from pathlib import Path

ROOT = Path('/workspaces/E-HGP')
SOURCE = ROOT / 'build/v7_full_pipeline_threads_micro_20260906/micro_r1'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_pipeline_threads_micro_20260906'
RECEIPT = '6256d27c354c085bc52096061ec07dd2d2d3752fc94db9d920e1cddfb41e4122'


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def encode(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def main():
    if DEST.exists():
        raise ValueError('create-only destination exists')
    raw = (SOURCE / 'receipt.json').read_bytes()
    receipt = json.loads(raw)
    if digest(raw) != RECEIPT or receipt['status'] != 'completed' or not receipt['sources_stable'] or not receipt['all_groups_closed']:
        raise ValueError('closed receipt identity')
    payload = {}
    for path in sorted(SOURCE.rglob('*')):
        if path.is_symlink():
            raise ValueError('unexpected symlink')
        if path.is_file():
            data = path.read_bytes()
            if data.startswith(b'\x7fELF'):
                raise ValueError('no ELF expected in micro capture')
            payload[str(path.relative_to(SOURCE))] = data
    payload['publish.py'] = Path(__file__).read_bytes()
    payload['binary_reference.json'] = encode(dict(
        original_path=str(ROOT / 'build/v7_full_pipeline_threads_20260906/full_probe'),
        sha256=receipt['binary_sha256'], sha256_after=receipt['binary_sha256_after'],
        binary_not_copied=True, original_preserved=True))
    payload['build_observation.json'] = encode(dict(
        authority='ROOT_reported_compile_exit0_no_stderr_not_a_recorded_build_command',
        argv=['c++', '-O3', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
              '-MMD', '-MF', 'build/v7_full_pipeline_threads_20260906/full_probe.d',
              'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp', '-o', 'build/v7_full_pipeline_threads_20260906/full_probe'],
        source_timing='42_actual_project_dependencies_copied_after_compilation_before_micros',
        hermetic_build_claimed=False))
    payload['README.md'] = '''# Pipeline multi-CPU : micros — 6 septembre 2026

Cinq appels n=8, s=8, Kmax=10, lazy C=1 000 000, P=unlimited emploient
le même ELF `4f5ba475…` : sans option threads (v5), puis threads=1/2/4/8
(v6 explicite). Chacun termine code 0, huit ordres K=1..8. L'entrée,
les huit digests d'ordre et le digest final sont identiques ; tous les champs
non mesurés des ordres sont identiques. FULL et la boucle K restent séquentiels.

Sur ces petites fixtures, les workers observés rects/préfiltre/census/expand
atteignent 2/4/8 ; WSPD et tri restent à 1. Ce ne sont pas des chronométrages
de performance, ni des preuves de complétude géométrique.

Neuf rejets de la sonde rendent 2 avec terminal invalid_input ; sept rejets
du runner rendent 2 avant création du répertoire demandé. La porte de schéma
pure du lecteur mono inchangé admet v5 et refuse les quatre v6 avec la cause
exacte `schema` ; ce contrôle n'est pas une exécution du juge mono complet.
La comparaison Python normal/-O rend deux fois 0 avec des stdout identiques.
Les 23 commandes et leurs groupes sont clos ; leurs bruts sont conservés.

Les limites locales 60 s murales / 45 s CPU du recorder sont les paramètres
réellement exécutés pour ces seuls micros ; elles ne sont pas appliquées aux
mesures directes de taille utile. Aucune capture n'est réécrite après clôture.

Les 42 dépendances projet du depfile sont copiées une seule fois sous sources/,
avec le runner et le lecteur mono (44 fichiers au total). Ce snapshot est
post-compilation, avant les micros : aucune provenance pré-build ou hermétique
n'est revendiquée. build_observation.json distingue la compilation rapportée
par ROOT des commandes réellement capturées. L'ELF n'est pas distribué ; son
identité avant/après est conservée. public_status=not_claimed, CPU/u16.

Lecture portable, sans moteur, depuis ce dossier :

```bash
sha256sum -c SHA256SUMS
python3 -B record.py --check .
python3 -B -O record.py --check .
```

Aucun résultat n=8000 n'est inclus ici. GCP non utilisé.
'''.encode()
    DEST.mkdir()
    for name, data in sorted(payload.items()):
        path = DEST / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as stream:
            stream.write(data)
        if path.read_bytes() != data:
            raise ValueError('copy mismatch')
    manifest = ''.join(digest(data) + '  ' + name + '\n' for name, data in sorted(payload.items())).encode()
    with (DEST / 'SHA256SUMS').open('xb') as stream:
        stream.write(manifest)
    print(json.dumps(dict(files=len(payload)+1, SHA256SUMS=digest(manifest), receipt_sha256=RECEIPT), sort_keys=True))


if __name__ == '__main__':
    main()
