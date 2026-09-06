#!/usr/bin/env python3
"""Create-only publication of closed terminal-once evidence; no engine."""
import difflib
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'build/v7_wspd_terminal_once_20260906'
DEST = ROOT / 'morsehgp3D_v7/receipts/wspd_terminal_once_negative_20260906'
PINS = {
    'receipt.json': '82d3655eb44b78c382a36c69a2081deeab5a20fdcd6108965cf68494f640a1cc',
    'sources_before.json': 'feb4036ced14d6bc4972d910b35ae95fed54fc14d25167e123c8a3cf0365837c',
    'sources_after.json': 'feb4036ced14d6bc4972d910b35ae95fed54fc14d25167e123c8a3cf0365837c',
    'measure_logs/result.json': '73ceddbb2ce27be5453d7c2fcc14cfd86ffe5502cffd914d5cec8ba3e9ff4829',
    'RESULTATS_8K.md': '4741e4002b3523ef19349eb785346660abefeb0cdd96dcd9ae3ba96f62f1c604',
    'measure.py': '2294ebba5f2fbbfc84c9fab147863a5c70e3b0b0c2757c2b846208d38040ca2e',
}
COMPRESSED = {'measure_logs/reference_measure.stdout', 'measure_logs/candidate_measure.stdout'}


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def require(ok, why):
    if not ok:
        raise ValueError(why)


def write(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(raw)


def main():
    require(sys.argv[1:] == ['--execute'], 'inert: --execute required')
    require(not DEST.exists(), 'destination_exists')
    for name, pin in PINS.items():
        require(digest((SOURCE / name).read_bytes()) == pin, 'pin:' + name)
    for name, pin in json.loads((SOURCE / 'sources_before.json').read_text()).items():
        require(digest((SOURCE / name).read_bytes()) == pin, 'closed_qualification_source:' + name)
    require(json.loads((SOURCE / 'receipt.json').read_text())['status'] == 'completed', 'qualification_not_closed')
    result = json.loads((SOURCE / 'measure_logs/result.json').read_text())
    require(result['status'] == 'completed' and result['literal_semantic_equality'] is True, 'pair_not_closed')
    for arm in ('reference', 'candidate'):
        command = json.loads((SOURCE / 'measure_logs' / (arm + '_measure.command.json')).read_text())
        require(command['exit_code'] == 0 and command['closed'] is True
                and command['timeout'] is None and command['imposed_rlimits'] is None, 'measurement_closure')
        for suffix, binding in command['streams'].items():
            raw = (SOURCE / 'measure_logs' / (arm + '_measure.' + suffix)).read_bytes()
            require(len(raw) == binding['bytes'] and digest(raw) == binding['sha256'], 'measurement_stream')
    DEST.mkdir()
    entries = {}
    for path in sorted(SOURCE.rglob('*')):
        rel = path.relative_to(SOURCE)
        if not path.is_file() or rel.parts[0] == 'bin' or '__pycache__' in rel.parts:
            continue
        raw = path.read_bytes()
        require(not raw.startswith(b'\x7fELF'), 'unexpected_ELF:' + str(rel))
        name = rel.as_posix()
        packed = gzip.compress(raw, compresslevel=6, mtime=0) if name in COMPRESSED else raw
        target = 'payload/' + name + ('.gz' if name in COMPRESSED else '')
        write(DEST / target, packed)
        if name in COMPRESSED:
            require(gzip.decompress((DEST / target).read_bytes()) == raw, 'gzip_roundtrip')
        else:
            require((DEST / target).read_bytes() == raw, 'copy_changed')
        entries[name] = dict(path=target, original_sha256=digest(raw), original_bytes=len(raw),
                             stored_sha256=digest(packed), stored_bytes=len(packed),
                             encoding='gzip' if name in COMPRESSED else 'identity')
    relative = 'src/pipeline/generate.hpp'
    old = (SOURCE / 'reference' / relative).read_text().splitlines(keepends=True)
    new = (SOURCE / 'candidate' / relative).read_text().splitlines(keepends=True)
    patch = ''.join(difflib.unified_diff(old, new, fromfile='a/' + relative, tofile='b/' + relative))
    write(DEST / 'terminal_once.patch', patch.encode())
    write(DEST / 'publish.py', Path(__file__).read_bytes())
    readme = """# Terminal-once WSPD : piste privée non retenue

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La suppression du premier comptage cœur aux rectangles terminaux conserve les sorties sur les [174 petits fronts](payload/RESULTATS_R1.md) et sur [une paire front8k](payload/RESULTATS_8K.md), mais n'apporte aucun gain de temps observé sur cette paire : **37,767 s → 38,287 s**. Les tests de coins passent de **167 115 088 à 335 509 837**. Ce doublement du travail est observé exactement sur cette entrée ; la différence de temps de +1,375 % n'est pas une conclusion statistique ni un théorème de régression. **Pas d'intégration produit.**

Les 754 686 rectangles et tous les champs sémantiques sérialisés sont identiques. Le temps couvre seulement le front, sans construction de l'index ni sérialisation ; ce n'est ni la génération complète ni la tour FULL. Mesure unique par bras, référence puis candidat, CPU6, un thread, n=8000/s=8, uniforme/graine3/coord65536, h=10/9/8. Aucun timeout ou nouveau rlimit pour les mesures. Les petits tests O2 ne transmettent pas de qualification SAN.

[Preuve et limite](payload/README.md), [diff minimal](terminal_once.patch), [reçu174](payload/receipt.json), [résultat8k](payload/measure_logs/result.json). Les snapshots source, depfiles, commandes, PID/codes et flux sont conservés ; leurs chemins absolus sont des enregistrements historiques, pas des chemins portables à exécuter. Les scripts de capture sont archivés, aucun nouveau run n'a servi à cette publication. Aucun ELF n'est publié et aucun fichier auditeur n'est copié.

Les deux grands JSON stdout sont stockés en gzip déterministe, sans altération des octets décompressés. `publication.json` donne pour chaque fichier le chemin public, l'encodage et les SHA-256/taille avant et après compression. Le SHA original du flux reste celui de sa commande historique.

Depuis la racine de ce paquet, `sha256sum -c SHA256SUMS` vérifie les copies. Pour relire un grand JSON sans moteur, utiliser `gzip -cd payload/measure_logs/reference_measure.stdout.gz` (ou `candidate_measure.stdout.gz`). La vérification de décompression exacte a été faite lors de la copie. Les chemins du manifeste sont canoniques, sans préfixe `./`.

GCP non utilisé. Aucun contrat 50k/1s ou massif/GPU acquis.
"""
    write(DEST / 'README.md', readme.encode())
    publication = dict(schema='mhgp7-private-terminal-once-publication-v1', status='completed',
                       public_status='not_claimed', verdict='not_integrated_no_observed_gain',
                       engine_invoked=False, source_directory=str(SOURCE), closed_pins=PINS,
                       files=entries, omitted=['bin/', '**/__pycache__/'], exact_gzip_roundtrip=True)
    write(DEST / 'publication.json', (json.dumps(publication, indent=2, sort_keys=True) + '\n').encode())
    files = sorted(p for p in DEST.rglob('*') if p.is_file())
    sums = ''.join(digest(p.read_bytes()) + '  ' + p.relative_to(DEST).as_posix() + '\n' for p in files)
    write(DEST / 'SHA256SUMS', sums.encode())
    for line in sums.splitlines():
        pin, name = line.split('  ', 1)
        require(not name.startswith('./') and digest((DEST / name).read_bytes()) == pin, 'manifest')
    print(json.dumps(dict(status='completed', files=len(files) + 1, entries=len(files),
                         sha256sums=digest(sums.encode()), destination=str(DEST)), sort_keys=True))


if __name__ == '__main__':
    main()
