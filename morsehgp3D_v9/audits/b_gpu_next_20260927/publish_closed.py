#!/usr/bin/env python3
"""Publish a closed core/warm capture. No cloud or shell calls."""
import argparse
import json
from pathlib import Path
import shutil
import tarfile

import readback as analysis

# Import only the old safe-copy/allowlist helpers. Neither its payload
# analysis nor its hardcoded payload README/publication entry point runs.
copy_helpers = analysis.load_pinned('mhgp9_core_safe_copy', 'publish_closed.py',
    'eb4f56273691405c569cbe236531a4f0a385ed01284ac6e3b4118d058230e992')
need, read, sha = analysis.need, analysis.read, analysis.sha


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + '\n')


def readme(summary):
    lines = ['# FULL G4 : noyau diamétral ON/OFF et répétitions résidentes', '',
        'Capture close et rejugée. Source `' + summary['source_commit'] + '`.',
        'Une seule trame : 08/000000 sans sol, 39 885 sites, grille 1 mm,',
        'tour K1..5 explicite, s8, 48 workers. Quatre processus GPU et deux',
        'témoins moteur CPU. Deux processus indépendants par bras GPU ;',
        'quatre calculs de la même trame dans chacun, pas quatre scènes.', '',
        '| Bras | Premier passage, médiane de deux processus (ms) | Médiane chaude par processus, puis médiane (ms) |',
        '| --- | ---: | ---: |']
    for name, label in (('core_on', 'noyau ON'), ('core_off', 'noyau OFF')):
        value = summary['timings'][name]
        lines.append('| ' + label + ' | ' + format(value['first_frame_ms']['chain_total_ms']['median'], '.3f') +
                     ' | ' + format(value['warm_process_median_ms']['chain_total_ms']['median'], '.3f') + ' |')
    lines += ['', 'Les trois digests et tous les objets jugés sont égaux dans les 18',
        'passages. Le registre des certificats de chaque bras GPU est aussi',
        'comparé directement au témoin moteur de même option, y compris OFF.',
        'Les sous-chronos détaillés décrivent seulement le premier passage.',
        'Lecture, segmentation et digests restent hors chaîne ; le mur des',
        'processus est publié séparément. Aucun nouveau contrat multis-scènes,',
        '100 ms ou sous-quadratique global n’est certifié par ces répétitions.', '',
        'Allocation GCE observée : ' + format(summary['cost']['vm_elapsed_seconds'], '.3f') + ' s.',
        'Arrêt de la génération `' + summary['cost']['generation'] + '` relu',
        '`TERMINATED` ; ce temps n’est pas une facture.', '',
        'Lecteur : `morsehgp3D_v9/audits/b_gpu_next_20260927/readback.py`,',
        'normal puis `python3 -O`, avec le snapshot privé indiqué par son hash',
        'dans `SUMMARY.json`. Le snapshot se reconstruit depuis le commit',
        'et `plan.json`, vers un répertoire neuf. Aucun KITTI, snapshot,',
        'archive ou clé SSH n’est ajouté à la v9.', '']
    return '\n'.join(lines)


def publish(host, package_path, after_stop, output):
    need(host.name == 'tower_v9_host' and host.is_dir() and not host.is_symlink(), 'exact host directory')
    need(not output.exists() and not output.is_symlink(), 'fresh output required')
    receipt = read(host / 'receipt.json')
    analysis.evidence.closed_host(receipt)
    snapshot = host / 'snapshot.tar.gz'
    package = read(package_path)
    need(all(package[name] == receipt[name] for name in
             ('snapshot_sha256', 'manifest_sha256', 'worker_sha256')), 'package pins differ')
    summary = analysis.build(host, snapshot, after_stop)
    vm = host / 'received/output'
    need(not (host / 'received').is_symlink() and not vm.is_symlink(), 'received symlink')
    vm_paths = copy_helpers.vm_files(vm)
    host_paths = copy_helpers.host_files(host)
    sources = vm_paths + host_paths + [package_path, host / 'source_manifest.json', after_stop]
    originals = {str(path): sha(path) for path in sources}
    output.mkdir(parents=True)
    (output / 'vm').mkdir()
    for path in vm_paths:
        shutil.copyfile(path, output / 'vm' / path.name)
        need(sha(path) == sha(output / 'vm' / path.name), 'raw VM copy hash')
    copy_helpers.copy_host(host, output / 'host', {'after_stop': after_stop})
    shutil.copyfile(package_path, output / 'PACKAGE.json')
    shutil.copyfile(host / 'source_manifest.json', output / 'source_manifest.json')
    with tarfile.open(snapshot, 'r:*') as archive:
        (output / 'plan.json').write_bytes(archive.extractfile(analysis.worker.PLAN).read())
    need(originals == {name: sha(Path(name)) for name in originals}, 'source evidence changed')
    need(analysis.build(output, snapshot, verify_inventory=False) == summary, 'published replay differs')
    save(output / 'SUMMARY.json', summary)
    (output / 'README.md').write_text(readme(summary))
    (output / 'SHA256SUMS').write_text(''.join(pin + '  ' + name + '\n'
        for name, pin in analysis.inventory(output).items()))
    analysis.build(output, snapshot)
    return dict(status='published_closed_capture', GCP_calls=False, path=str(output),
                files=len(analysis.inventory(output)), generation=receipt['generation'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('host', 'package', 'after-stop', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(publish(args.host, args.package, args.after_stop, args.output), sort_keys=True))


if __name__ == '__main__':
    main()
