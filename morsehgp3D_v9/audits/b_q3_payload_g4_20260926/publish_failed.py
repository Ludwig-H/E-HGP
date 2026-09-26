#!/usr/bin/env python3
"""Publish closed capture failures only. No cloud calls and no claimed FULL result."""
import argparse
import json
from pathlib import Path
import shutil
import tarfile

import publish_closed as pub
import read_failed as reader


def readme(summary):
    return ("# Capture G4 non récupérée — preuve d'échec close\n\n"
            "Autorité : `SUMMARY.json` est recalculé depuis les trois reçus hôte,\n"
            "leurs commandes et les confirmations GCE arrêtées. Aucun probe brut\n"
            "n'a été récupéré : zéro cas FULL/GPU qualifié, aucun chrono exploitable.\n"
            "Les 26 cas sont ceux du plan, pas 26 résultats localement validés.\n\n"
            "L'échec initial de capture est conservé (`host/`). La récupération R1\n"
            "a été refusée avant démarrage pour expiration insuffisante de la clé\n"
            "OS Login (`failed_recovery/`). R2 a été arrêtée après un échec silencieux\n"
            "du pack (`recovery_allocated/`) ; sa précondition bloquante exacte n'est\n"
            "pas établie. Aucune disparition de données n'est déduite.\n\n"
            "Budget : original 309,542 s + R2 121,113 s = " + format(summary['cost']['vm_elapsed_seconds'], '.3f').replace('.', ',') +
            " s d'allocation GCE ;\nR1 ne crée aucune génération. Les deux générations allouées sont certifiées\n"
            "TERMINATED. Aucun montant facturé n'est estimé.\n\n"
            "Relecture hors ligne depuis la racine, normal puis avec `-O` :\n\n"
            "```sh\npython3 -B morsehgp3D_v9/audits/b_q3_payload_g4_20260926/read_failed.py \\\n"
            "  morsehgp3D_v9/receipts/g4_q3_payload_failed_capture_20260926 \\\n"
            "  --snapshot /workspaces/E-HGP/build/v9-q3-payload-snapshot-f9f273bb0/snapshot.tar.gz\n```\n\n"
            "Lecteur LIVE : snapshot privé épinglé nécessaire. `SHA256SUMS` couvre\n"
            "tous les fichiers publiés sauf lui-même. Ni archive, ni nouvelle donnée\n"
            "KITTI, ni clé SSH, ni réponse OS Login brute n'est publiée. Les reçus\n"
            "restent inchangés ; seuls les logs sélectionnés et états GCE sont expurgés.\n")


def publish(host, prestart, allocated, package, output):
    pub.need(host.name == 'tower_v9_host' and all(p.is_dir() and not p.is_symlink() for p in (host, prestart, allocated)),
             'exact evidence directories required')
    pub.need(not output.exists() and not output.is_symlink(), 'fresh publication required')
    snapshot = host / 'snapshot.tar.gz'
    summary = reader.build(host, prestart, allocated, snapshot, package)
    paths = [package, host / 'source_manifest.json']
    for directory in (host, prestart, allocated):
        paths += pub.host_files(directory)
    paths += [prestart / 'original_after_stop.json', allocated / 'original_after_stop.json', allocated / 'after_stop.json']
    pins = {str(path): pub.sha(path) for path in paths}
    output.mkdir(parents=True)
    pub.copy_host(host, output / 'host', {'after_stop': allocated / 'original_after_stop.json'})
    pub.copy_host(prestart, output / 'failed_recovery', {'original_after_stop': prestart / 'original_after_stop.json'})
    pub.copy_host(allocated, output / 'recovery_allocated', {name: allocated / (name + '.json')
                  for name in ('original_after_stop', 'after_stop')})
    shutil.copyfile(package, output / 'PACKAGE.json')
    shutil.copyfile(host / 'source_manifest.json', output / 'source_manifest.json')
    with tarfile.open(snapshot, 'r:*') as archive:
        (output / 'plan.json').write_bytes(archive.extractfile(pub.analysis.worker.PLAN).read())
    pub.need(pins == {path: pub.sha(Path(path)) for path in pins}, 'failure sources changed during copy')
    replay = reader.build(output / 'host', output / 'failed_recovery', output / 'recovery_allocated', snapshot,
                          output / 'PACKAGE.json', published_root=output, verify_inventory=False)
    pub.need(replay == summary, 'failure publication differs')
    pub.save(output / 'SUMMARY.json', summary)
    (output / 'README.md').write_text(readme(summary))
    (output / 'SHA256SUMS').write_text(''.join(pin + '  ' + name + '\n' for name, pin in pub.analysis.inventory(output).items()))
    reader.build(output / 'host', output / 'failed_recovery', output / 'recovery_allocated', snapshot,
                 output / 'PACKAGE.json', published_root=output)
    return dict(status='published_capture_failures_only', GCP_calls=False, files=len(pub.analysis.inventory(output)),
                vm_elapsed_seconds=summary['cost']['vm_elapsed_seconds'], locally_validated_case_count=0, path=str(output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('host', 'prestart', 'allocated', 'package', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(publish(args.host, args.prestart, args.allocated, args.package, args.output), sort_keys=True))


if __name__ == '__main__':
    main()
