#!/usr/bin/env python3
"""Allowlist publication of a CLOSED local FULL G4 capture; no GCP/SSH call."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import tarfile

import readback as analysis

need, read, sha = analysis.need, analysis.read, analysis.sha


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + '\n')


def safe_bytes(path):
    need(path.is_file() and not path.is_symlink(), 'regular nonsymlink evidence required')
    raw = path.read_bytes()
    need(b'PRIVATE KEY' not in raw, 'private key marker in evidence')
    return raw


def redact(raw):
    need(b'PRIVATE KEY' not in raw, 'private key marker in guard log')
    text = raw.decode('utf-8')
    text = re.sub(r'[A-Za-z0-9_.+%-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}', '<account redacted>', text)
    text = re.sub(r'(?:ssh-ed25519|ssh-rsa|ecdsa-sha2-\S+) [A-Za-z0-9+/=]+(?: [^\r\n]*)?',
                  '<public SSH key redacted>', text)
    text = re.sub(r'(Instance (?:internal|external) IP is )[^\r\n]+', r'\1<address redacted>', text)
    return text.encode()


def vm_files(vm):
    receipt = read(vm / 'receipt.json')
    allowed = {'receipt.json', 'guard_evidence.json', 'sources_before.json', 'sources_after.json',
               'compiled_dependencies.json', 'meminfo.txt', 'os-release.txt', analysis.worker.PREFLIGHT_FILE}
    commands = []
    for path in sorted(vm.glob('*.command.json')):
        stem = path.name[:-len('.command.json')]
        need(re.fullmatch(r'[a-z0-9_]+', stem) is not None, 'unsafe command stem')
        commands.append(read(path))
        allowed.update(stem + suffix for suffix in ('.command.json', '.intent.json', '.stdout', '.stderr'))
        if re.fullmatch(r'probe_[0-9]+', stem):
            allowed.add(stem + '.summary.json')
    canonical = lambda rows: sorted(json.dumps(row, sort_keys=True) for row in rows)
    need(canonical(commands) == canonical(receipt['commands']), 'VM command inventory differs')
    paths = []
    for path in sorted(vm.iterdir()):
        need(path.name in allowed and path.is_file() and not path.is_symlink(), 'unexpected VM publication file: ' + path.name)
        safe_bytes(path)
        paths.append(path)
    # The ONLY binary allowed is the independently reproducible synthetic
    # preflight, not any of the transported SemanticKITTI input files.
    need((vm / analysis.worker.PREFLIGHT_FILE).read_bytes() == analysis.worker.preflight_cloud(),
         'preflight input is not the deterministic synthetic fixture')
    return paths


def projection(after):
    return {key: after[key] for key in ('name', 'selfLink', 'status', 'lastStartTimestamp', 'lastStopTimestamp',
            'zone', 'machineType', 'labels', 'scheduling') if key in after}


def host_files(host):
    paths = [path for path in sorted(host.iterdir()) if path.name.endswith(('.command.json', '.intent.json')) or
             path.name in ('receipt.json', 'handoff.json', 'newhandoff.json', 'lifecycle.txt')]
    for path in paths:
        safe_bytes(path)
    return paths


def copy_host(source, destination, state_paths):
    """Only command metadata, closed receipt, safe state projections and redacted guard/incident logs."""
    destination.mkdir()
    for path in host_files(source):
        shutil.copyfile(path, destination / path.name)
        need(sha(path) == sha(destination / path.name), 'raw host copy hash')
    for name in ('double_guard_verified', 'guest_guard_pending'):
        path = source / 'guardmarks' / name
        if path.exists():
            (destination / name).write_bytes(safe_bytes(path))
    redactions = []
    for stem in ('guarded_start', 'guarded_stop', 'pack_capture', 'worker', 'guest_schedule'):
        for suffix in ('stdout', 'stderr'):
            path = source / (stem + '.' + suffix)
            if path.exists():
                raw = safe_bytes(path)
                name = stem + '.redacted.' + suffix
                target = destination / name
                target.write_bytes(redact(raw))
                redactions.append(dict(source_name=path.name, source_sha256=hashlib.sha256(raw).hexdigest(),
                                       published_name=name, published_sha256=sha(target)))
    pins = {}
    for name, path in state_paths.items():
        save(destination / (name + '.json'), projection(read(path)))
        pins[name + '_source_sha256'] = sha(path)
    save(destination / 'PUBLICATION_REDACTIONS.json', dict(redactions=redactions, **pins,
         policy='Selected guard/incident stdout/stderr redacted; other host stdout/stderr, OS Login replies, session parent, keys and archives omitted'))


def readme(summary, worker_receipt):
    need(all(row['case']['frames'] == 1 for row in summary['rows']), 'README scope requires single-frame processes')
    gpu_cases = len(worker_receipt['GPU_completed_cases'])
    measured_allocation = summary['cost'].get('sessions', [summary['cost']])[0]
    lines = ["# FULL G4 — intérieurs q3 transportés jusqu'au catalogue", '',
             'Autorité : probes bruts dans `vm/`, reçus hôte fermés dans `host/`,',
             '`SUMMARY.json` entièrement recalculé par le lecteur. Sources : `' + summary['source_commit'] + '`.',
             'Capture : `' + summary['capture_status'] + '`, ' + str(summary['case_count']) + ' cas rejugés : ' +
             str(gpu_cases) + ' GPU et ' + str(summary['case_count'] - gpu_cases) + ' engine CPU ; ' +
             str(len(worker_receipt['cross_worker_comparisons'])) + ' comparaisons exactes et ' +
             str(sum(len(pair['paired_processes']) for pair in summary['paired_comparisons'])) + ' paires ON/OFF.', '',
             'Profil grille entière **1 mm/u18**, toute la tour **K=1..Kmax explicite**.',
             '00/01/02 désignent 08/000000, 08/000100 et 08/000200 sans sol :',
             '39 885 / 35 551 / 45 845 sites après les masques figés. b00 est la',
             'trame brute entière 08/000000, 123 389 sites. Ces trames appartiennent',
             'toutes à la même séquence 08.', '',
             'Les paires ON/OFF gardent trois digests et le travail producteur identiques.',
             'Les temps ci-dessous couvrent la chaîne vers la tour explicite sur entrée',
             'préparée en mémoire ; lecture fichier, segmentation et digests sont séparés.',
             'Tous les processus ont `frames=1` : aucune mesure chaude. Les valeurs sont',
             'les médianes de deux processus par bras pour 00/K5/s8 seulement, et la',
             'mesure unique par bras ailleurs ; ce ne sont pas les premiers essais chronologiques.', '',
             '| Scène | K | s | Médiane OFF (ms) | Médiane ON (ms) |',
             '|---|---:|---:|---:|---:|']
    for pair in summary['paired_comparisons']:
        cells = []
        for arm in ('off', 'on'):
            value = pair['timings'][arm]['first_frame_ms']['chain_total_ms']
            cells.append(format(value['median'], '.3f'))
        lines.append('| ' + ' | '.join([pair['scene'], str(pair['K']), str(pair['s'])] + cells) + ' |')
    lines += ['', 'Sur 00/K5/s8, les deux deltas appariés ON−OFF sont −11,474 ms puis',
              '+1,222 ms ; sur 02, +4,533 ms. Le census médian 00 diminue de 102,5415',
              'à 82,8245 ms, mais le gain FULL n\'est pas stable. Le défaut reste **OFF**.', '',
              'Ces observations ne qualifient ni les 100 ms, ni plusieurs séquences,',
              'ni une borne sous-quadratique globale. Le masque sans-sol figé ne remplace',
              'pas le contrat sur la trame brute. Les résultats des échecs antérieurs',
              'restent distincts dans `../g4_q3_payload_failed_capture_20260926/`.', '',
              'Allocation GCE de cette capture : ' + format(summary['cost']['vm_elapsed_seconds'], '.3f') +
              ' s ; avec les échecs liés : **737,423 s**. Aucun montant facturé estimé.',
              'Génération `' + measured_allocation['generation'] + '` ; arrêt `' + measured_allocation['stopped'] + '`.',
              'Le contrôle GCE après arrêt confirme `TERMINATED` sur cette même génération.', '',
              'Depuis la racine, lire normalement puis ajouter `-O` :', '', '```sh',
              'python3 -B morsehgp3D_v9/audits/b_q3_payload_g4_20260926/readback.py \\',
              '  morsehgp3D_v9/receipts/g4_q3_payload_20260926 \\',
              '  --snapshot /workspaces/E-HGP/build/v9-q3-payload-snapshot-f9f273bb0/snapshot.tar.gz', '```', '',
              'Le snapshot privé et le protocole épinglé sont nécessaires à cette relecture LIVE.',
              '`SHA256SUMS` couvre les fichiers publiés sauf lui-même. Aucun snapshot,',
              'nouveau nuage KITTI, archive ou clé SSH n\'est publié.']
    return '\n'.join(lines) + '\n'


def publish(host, package_path, after_stop, output, recovery_dir=None, failed_recovery_dir=None):
    need(host.name == 'tower_v9_host' and host.is_dir() and not host.is_symlink(), 'exact host directory only')
    need(not output.exists() and not output.is_symlink(), 'fresh publication required')
    receipt = read(host / 'receipt.json')
    analysis.closed_host(receipt, recovery_dir is not None)
    if recovery_dir is not None:
        need(recovery_dir.is_dir() and not recovery_dir.is_symlink() and recovery_dir != host and
             not (recovery_dir / 'received').is_symlink(), 'exact separate recovery directory only')
    if failed_recovery_dir is not None:
        need(recovery_dir is not None and failed_recovery_dir.is_dir() and not failed_recovery_dir.is_symlink() and
             failed_recovery_dir not in (host, recovery_dir), 'exact separate failed recovery directory only')
    snapshot = host / 'snapshot.tar.gz'
    need(snapshot.is_file() and not snapshot.is_symlink(), 'private local snapshot required')
    package = read(package_path)
    need(package['snapshot_sha256'] == receipt['snapshot_sha256'] and
         package['manifest_sha256'] == receipt['manifest_sha256'] and package['worker_sha256'] == receipt['worker_sha256'],
         'package pins differ from closed host')
    # Full replay BEFORE creating any publication. Never executes the cloud.
    summary = analysis.build(host, snapshot, after_stop, recovery_dir=recovery_dir, failed_recovery_dir=failed_recovery_dir)
    vm = (recovery_dir or host) / 'received/output'
    need(not ((recovery_dir or host) / 'received').is_symlink() and not vm.is_symlink(), 'received directory symlink')
    vm_paths = vm_files(vm)
    host_paths = host_files(host) + (host_files(recovery_dir) if recovery_dir else [])
    if failed_recovery_dir:
        host_paths += host_files(failed_recovery_dir)
    # Record raw hashes before copying, and verify unchanged sources after.
    sources = vm_paths + host_paths + [package_path, host / 'source_manifest.json', after_stop]
    if recovery_dir:
        sources.extend(recovery_dir / (name + '.json') for name in ('original_after_stop', 'after_stop'))
    if failed_recovery_dir:
        sources.append(failed_recovery_dir / 'original_after_stop.json')
    originals = {str(path): sha(path) for path in sources}
    output.mkdir(parents=True)
    (output / 'vm').mkdir()
    for source in vm_paths:
        shutil.copyfile(source, output / 'vm' / source.name)
        need(sha(source) == sha(output / 'vm' / source.name), 'raw VM copy hash')
    copy_host(host, output / 'host', {'after_stop': after_stop})
    if recovery_dir:
        copy_host(recovery_dir, output / 'recovery', {name: recovery_dir / (name + '.json')
                  for name in ('original_after_stop', 'after_stop')})
    if failed_recovery_dir:
        copy_host(failed_recovery_dir, output / 'failed_recovery',
                  {'original_after_stop': failed_recovery_dir / 'original_after_stop.json'})
    shutil.copyfile(package_path, output / 'PACKAGE.json')
    shutil.copyfile(host / 'source_manifest.json', output / 'source_manifest.json')
    with tarfile.open(snapshot, 'r:*') as archive:
        (output / 'plan.json').write_bytes(archive.extractfile(analysis.worker.PLAN).read())
    need(originals == {path: sha(Path(path)) for path in originals}, 'source evidence changed during publication')
    need(analysis.build(output, snapshot, verify_inventory=False) == summary, 'published analysis differs')
    save(output / 'SUMMARY.json', summary)
    (output / 'README.md').write_text(readme(summary, read(vm / 'receipt.json')))
    (output / 'SHA256SUMS').write_text(''.join(pin + '  ' + name + '\n' for name, pin in analysis.inventory(output).items()))
    analysis.build(output, snapshot)  # includes inventory and SUMMARY comparison
    return dict(status='published_recovered_capture' if recovery_dir else 'published_closed_capture',
                GCP_calls=False, files=len(analysis.inventory(output)),
                path=str(output), generation=receipt['generation'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('host', 'package', 'after-stop', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--recovery-dir', type=Path)
    parser.add_argument('--failed-recovery-dir', type=Path)
    args = parser.parse_args()
    print(json.dumps(publish(args.host, args.package, args.after_stop, args.output, args.recovery_dir, args.failed_recovery_dir), sort_keys=True))


if __name__ == '__main__':
    main()
