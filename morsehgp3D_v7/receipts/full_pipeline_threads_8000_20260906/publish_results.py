#!/usr/bin/env python3
"""Publish four already closed direct runs; copy/hash only, no new test."""
import hashlib
import json
from pathlib import Path

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_full_pipeline_threads_20260906'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_pipeline_threads_8000_20260906'
PINS = {1: '5e3ccdb2880bd449ccc9bd99615300fc9caf609d2a0592dea41d39896cf308eb',
        2: 'ca12d24b1e1f98e4074193158fb6de716dbc06a2dcdbb9d1dbda158578f8ef32',
        4: '7cf81cb19342059fb320d9514b2004e3cca4e71c19bde47140a986737bb4d4a0',
        8: '2364a2817193b5a11c3ab5f82eab0741f6a6e486a65b03210d1349b0743040d8'}


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encode(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def main():
    if DEST.exists():
        raise ValueError('create-only destination exists')
    payload = {}
    def take(name, expected=None):
        raw = (BASE / name).read_bytes()
        if expected is not None and sha(raw) != expected:
            raise ValueError('hash: ' + name)
        payload[name] = raw
        return raw
    for threads, pin in PINS.items():
        prefix = f'n8000_t{threads}/'
        run = json.loads(take(prefix + 'run.json', pin))
        if run['status'] != 'completed' or run['exit_code'] != 0 or run['binary_sha256'] != run['binary_sha256_after']:
            raise ValueError('run not successfully closed')
        take(prefix + 'stdout.jsonl', run['stdout_sha256'])
        take(prefix + 'stderr.txt', run['stderr_sha256'])
    read_receipt = json.loads(take('analysis_r1/receipt.json', 'd3d5cfe7ef4c11c6b82b4b204830fcc58f75666dbde48dba09cef1523e800ef1'))
    if read_receipt['status'] != 'completed' or not read_receipt['normal_optimized_equal'] or not read_receipt['reader_stable']:
        raise ValueError('pure reads not closed')
    for name, command in zip(('normal', 'optimized'), read_receipt['commands']):
        take(f'analysis_r1/{name}.stdout', command['stdout_sha256'])
        take(f'analysis_r1/{name}.stderr', command['stderr_sha256'])
        take(f'analysis_r1/{name}.intent.json')
        take(f'analysis_r1/{name}.command.json')
    take('read_results.py', 'd318ac4204f20d67e4555c6d1fd080a080590471c3de82992f862e6fc6f25d9e')
    take('read_capture.py', '784f4c82620344514a1b66b9e14c74fd9d2ae8ff908f7d8268e8db7f5d96e9d0')
    take('publish_results.py')
    references = {
        '../full_pipeline_threads_micro_20260906/SHA256SUMS': '1f3dea9e9b96b0b43cabfa2a69e3bcc49fdb9f6f3006386232649a361062244c',
        '../full_pipeline_threads_micro_20260906/receipt.json': '6256d27c354c085bc52096061ec07dd2d2d3752fc94db9d920e1cddfb41e4122',
        '../full_pipeline_threads_micro_20260906/sources_before.json': '56425351d4c6a3f23872e3bf5785cbf1d833112c1d8a9831dd86beccacf7e9ca',
    }
    for name, expected in references.items():
        if sha((DEST / name).resolve().read_bytes()) != expected:
            raise ValueError('existing micro/source reference changed')
    payload['SOURCE_REFERENCE.json'] = encode(dict(
        references=references, project_dependencies=42, source_files_not_copied_again=True,
        source_directory='../full_pipeline_threads_micro_20260906/sources',
        runner='../full_pipeline_threads_micro_20260906/sources/morsehgp3D_v7/bench/run_full_probe.py',
        build_observation='../full_pipeline_threads_micro_20260906/build_observation.json',
        binary_reference='../full_pipeline_threads_micro_20260906/binary_reference.json',
        binary_sha256='4f5ba475ae5075cabab6c84222742b2dcdae226a0b3879e460b7fb8200b76aff'))
    payload['publication_notes.json'] = encode(dict(
        initial_copy_attempt_exit_code=1,
        initial_copy_attempt_failure='FileNotFoundError resolving a sibling reference through the not-yet-created destination',
        initial_copy_attempt_wrote_no_files=True,
        correction='resolve sibling reference lexically before read; final directory name follows ROOT documentation',
        benchmark_captures_and_reader_results_unchanged=True))
    payload['README.md'] = '''# FULL : pipeline 1/2/4/8 threads, n8000 — 6 septembre 2026

Quatre mesures directes closes, même ELF, n=8000/s=8/Kmax=10/lazy C=1 000 000,
P=unlimited. Les dix ordres, leurs digests, le digest final et tous les champs
d'ordre hors mesures sont identiques. La configuration ne diffère que par N.
Hors mesures et N, seuls les six nombres de workers diffèrent au terminal :
aucune différence des compteurs de travail, volumes ou pics logiques observés.

| Threads | CPU demandés | Total terminal (s) | Génération (s) | FULL (s) | GNU pic RSS (KiB) |
| --- | --- | ---: | ---: | ---: | ---: |
| 1 | 6 | 132,948 | 59,562 | 51,094 | 1 321 488 |
| 2 | 4,6 | 98,176 | 30,093 | 53,935 | 1 432 136 |
| 4 | 0,2,4,6 | 74,558 | 15,483 | 50,139 | 1 486 940 |
| 8 | 0-7 | 69,824 | 11,538 | 50,765 | 1 495 380 |

Les ratios du temps mural du runner sont 1 / 1,354 / 1,783 / 1,903.
WSPD passe de 29,821 à 6,012 s ; rectangles de 29,741 à 5,525 s ;
préfiltre de 9,930 à 2,129 s ; census de 6,921 à 1,482 s.
FULL et la boucle K restent séquentiels : à 8 threads, FULL prend encore
50,765 des 69,824 s. Le détail par phase et les différences classées sont
dans analysis_r1/normal.stdout (JSON complet).

Une seule observation par bras : aucun gain statistiquement robuste, aucune
complétude géométrique, ni aucun contrat 50k certifié ici. public_status=not_claimed.
GNU peak RSS est distinct des échantillons RSS/HWM ; les pics logiques ne sont
pas une mesure de la mémoire totale. Le protocole direct n'a pas de watchdog.

Les 12 fichiers bruts sont copiés octet pour octet. Le reader vérifie leurs
hashes, commandes, statuts, dix ordres et liaison du digest final. Les deux
lectures Python normal/-O sont closes, code 0, stdout identiques. La clôture
des runs est rapportée par wait du runner, GNU exit0 et terminal completed ;
aucun certificat de groupe de processus n'est inventé dans ces fichiers.
ROOT a en outre observé l'absence de full_probe actif après les quatre runs.

SOURCE_REFERENCE.json pointe vers les 42 dépendances déjà publiées une seule
fois avec les micros. Pas de nouvelle copie des headers ni de l'ELF. Le snapshot
reste explicitement post-compilation. Aucun moteur ni compilation n'est lancé
par le reader ou par cette publication. GCP non utilisé.

Lecture portable depuis ce dossier :

```bash
sha256sum -c SHA256SUMS
python3 -B read_results.py --directory . --run-pin 1=5e3ccdb2880bd449ccc9bd99615300fc9caf609d2a0592dea41d39896cf308eb --run-pin 2=ca12d24b1e1f98e4074193158fb6de716dbc06a2dcdbb9d1dbda158578f8ef32 --run-pin 4=7cf81cb19342059fb320d9514b2004e3cca4e71c19bde47140a986737bb4d4a0 --run-pin 8=2364a2817193b5a11c3ab5f82eab0741f6a6e486a65b03210d1349b0743040d8
```

Ajouter `-O` à Python reproduit la seconde lecture ; aucun moteur n'est appelé.
'''.encode()
    DEST.mkdir()
    for name, raw in sorted(payload.items()):
        path = DEST / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as stream:
            stream.write(raw)
        if path.read_bytes() != raw:
            raise ValueError('copy mismatch')
    manifest = ''.join(sha(raw) + '  ' + name + '\n' for name, raw in sorted(payload.items())).encode()
    with (DEST / 'SHA256SUMS').open('xb') as stream:
        stream.write(manifest)
    print(json.dumps(dict(files=len(payload)+1, SHA256SUMS=sha(manifest)), sort_keys=True))


if __name__ == '__main__':
    main()
