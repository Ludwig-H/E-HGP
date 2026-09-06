#!/usr/bin/env python3
"""Copy the three closed direct runs and recalculate their descriptive results."""
import hashlib
import json
from pathlib import Path
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_direct_scaling_20260906'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_direct_scaling_20260906'
READER_SHA = 'db4e47035e3d75c464f4b9fd8f4beb23651b76e77772a78d98578e558eab2877'
RUNNER_SHA = 'd381fc81c378382e610c61722863f6ae5764e4c3ef34ee7f94ac18f53a6c5fe7'
README = """# Trois mesures directes FULL — 6 septembre 2026

Nuages uniformes quantifiés u16, seed 3, n=8 000/16 000/32 000, s=8,
P=`unlimited`, cache lazy de 1 000 000 entrées, CPU 6 mono-thread.
Les trois processus ont rendu le code 0 et un terminal `complete_relative`
après dix ordres horizontaux K=1..10. Une seule observation par taille.

Les neuf fichiers bruts sont copiés à l'identique. `run.json` porte les
commandes, les hashes des flux et le même hash ELF avant/après.
`bench/run_full_probe.py` conserve le runner ; celui-ci n'appliquait ni quota
d'opérations ni timeout. Il enregistre HEAD, pas une preuve hermétique du worktree.
Le build et les sources de cet ELF sont référencés dans `BUILD_REFERENCE.json`
et dans le paquet voisin [full_probe_no_quotas_20260906](../full_probe_no_quotas_20260906/README.md).
L'échec de raccord des auto-tests de ce paquet voisin n'est pas réétiqueté.

`python3 read_results.py` recalcule `results.json` : hashes, commandes, P/s/n,
dix ordres et terminal, puis temps, phases, volumes et ratios d'échelle.
Ce petit lecteur n'est **pas un juge de géométrie ou de complétude**.
Les volumes additionnés sur K ne sont ni une RAM simultanée ni une archive
conservée. Le pic GNU RSS est distingué des échantillons RSS/HWM de la sonde.
Les ratios comparent des tailles de nuages, pas deux algorithmes.

`public_status=not_claimed`. Ni tour inter-K intégrée, ni contrat 50k acquis,
ni mesure GPU. Aucun futur P0/s10/s12 n'est inclus ici. Vérification des octets :
`sha256sum -c SHA256SUMS` depuis ce dossier.
"""


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def main():
    if DEST.exists():
        raise ValueError('destination already exists; no overwrite')
    reader_raw = (BASE / 'read_results.py').read_bytes()
    runner_raw = (ROOT / 'morsehgp3D_v7/bench/run_full_probe.py').read_bytes()
    if sha(reader_raw) != READER_SHA or sha(runner_raw) != RUNNER_SHA:
        raise ValueError('source hash changed')
    reader = types.ModuleType('direct_measurements_only')
    reader.__file__ = str(BASE / 'read_results.py')
    exec(compile(reader_raw, reader.__file__, 'exec'), reader.__dict__)
    reference = ROOT / 'morsehgp3D_v7/receipts/full_probe_no_quotas_20260906/build_r1/receipt.json'
    report = reader.results(BASE, reference)
    payload = {}
    for n in reader.RUN_PINS:
        name = f'n{n}_s8_punlimited'
        folder = BASE / name
        if {p.name for p in folder.iterdir()} != {'run.json', 'stdout.jsonl', 'stderr.txt'}:
            raise ValueError('unexpected input file')
        for path in folder.iterdir():
            if path.is_symlink():
                raise ValueError('symlink input')
            payload[name + '/' + path.name] = path.read_bytes()
    payload.update({'read_results.py': reader_raw, 'bench/run_full_probe.py': runner_raw,
        'results.json': encoded(report), 'README.md': README.encode(),
        'BUILD_REFERENCE.json': encoded(dict(
            build_receipt='../full_probe_no_quotas_20260906/build_r1/receipt.json',
            build_receipt_sha256=reader.BUILD_SHA, binary_sha256=reader.BINARY_SHA,
            source_snapshot='../full_probe_no_quotas_20260906/source_snapshot',
            source_map_sha256='72a147fec063382013b4c61a1990ad163cd99dbe9051a8c170c417797b519fbe')),
        'publish.py': Path(__file__).read_bytes()})
    DEST.mkdir()
    for name, raw in sorted(payload.items()):
        path = DEST / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as out:
            out.write(raw)
        if path.read_bytes() != raw:
            raise ValueError('copy mismatch: ' + name)
    if reader.results(DEST, reference) != report:
        raise ValueError('copied results mismatch')
    manifest = ''.join(sha(raw) + '  ' + name + '\n' for name, raw in sorted(payload.items())).encode()
    with (DEST / 'SHA256SUMS').open('xb') as out:
        out.write(manifest)
    print(json.dumps(dict(status='copied', files=len(payload) + 1, directory=str(DEST),
        SHA256SUMS=sha(manifest), results_sha256=sha(payload['results.json']), engine_invoked=False), sort_keys=True))


if __name__ == '__main__':
    main()
