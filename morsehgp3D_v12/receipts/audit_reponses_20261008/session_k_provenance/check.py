#!/usr/bin/env python3
"""Relit les seules preuves locales K ; aucun contrôleur, moteur ou accès distant."""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(body):
    return hashlib.sha256(body).hexdigest()


def kv(body):
    return dict(line.split('=', 1) for line in body.decode().splitlines() if '=' in line)


def archive(path):
    with tarfile.open(path, 'r:gz') as tar:
        members = tar.getmembers()
        require(len({m.name for m in members}) == len(members), 'membre répété')
        require(all(m.isfile() or m.isdir() for m in members), 'type de membre inattendu')
        require(all(not m.name.startswith('/') and '..' not in Path(m.name).parts for m in members), 'chemin')
        return {m.name: tar.extractfile(m).read() for m in members if m.isfile()}, len(members)


def manifest(body, files, prefix=''):
    seen = set()
    for line in body.decode().splitlines():
        digest, name = line.split(maxsplit=1)
        name = name.lstrip('*')
        if name.startswith('./'):
            name = name[2:]
        if prefix and not name.startswith(prefix):
            name = prefix + name
        require(name not in seen and name in files, 'manifeste : nom')
        require(sha(files[name]) == digest, 'manifeste : empreinte')
        seen.add(name)
    return seen


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session-dir', type=Path, required=True)
    args = parser.parse_args()
    here = Path(__file__).parent
    cap = json.loads((here / 'capture.json').read_bytes())
    root = args.session_dir
    before = {}
    for name, digest in cap['local_artifacts_sha256'].items():
        body = (root / name).read_bytes()
        require(sha(body) == digest, 'artefact local différent : ' + name)
        before[name] = digest
    for name, digest in cap['receipt_artifacts_sha256'].items():
        require(sha((here / name).read_bytes()) == digest, 'reçu modifié : ' + name)

    result_files, members = archive(root / 'results/results.tar.gz')
    require(members == 221 and len(result_files) == 207, 'inventaire résultats')
    mf = result_files['results/MANIFEST.sha256']
    require(sha(mf) == cap['results_manifest_sha256'], 'manifeste résultats')
    covered = manifest(mf, result_files, 'results/')
    require(covered == set(result_files) - {'results/MANIFEST.sha256'}, 'couverture résultats')
    declared = (root / 'results/SHA256SUMS').read_text().split()
    require(declared == [cap['local_artifacts_sha256']['results/results.tar.gz'], 'results.tar.gz'], 'somme extérieure')
    require(result_files['results/plan.sh'] == (root / 'package/plan.sh').read_bytes(), 'plan rapatrié')

    source_files, _ = archive(root / 'package/package.tar.gz')
    source_covered = manifest((root / 'package/SNAPSHOT_MANIFEST.sha256').read_bytes(), source_files)
    require(len(source_covered) == 2262 and source_covered == set(source_files), 'couverture paquet source')
    for name, digest in cap['selected_source_sha256'].items():
        require(sha(source_files[name]) == digest, 'source sélectionnée')

    receipt = json.loads((root / 'receipt.json').read_bytes())
    preflight = json.loads((root / 'preflight.json').read_bytes())
    require(receipt['target'] == preflight['target'], 'cible différente')
    require(receipt['generation'] == receipt['closing_generation'], 'génération différente')
    require(receipt['observed_after']['name'] == receipt['target']['instance'], 'cible de clôture')
    require(receipt['observed_after']['status'] == 'TERMINATED', 'non arrêté')
    require(receipt['targeted_shutdown_certified'] is True and receipt['closure'] == 'stopped'
            and receipt['stop_exit_code'] == 0 and receipt['stop_attempts'] == 1, 'arrêt non certifié')
    require(receipt['observed_after']['lastStopTimestamp'] == cap['last_stop_timestamp'], 'date arrêt')
    require(receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1
            and receipt['worker_outcome'] == 'exited', 'issue de campagne')
    require(receipt['results_verified'] is True and receipt['data_verified_remote'] is True
            and receipt['results_sha256'] == cap['local_artifacts_sha256']['results/results.tar.gz'], 'résultats non liés')
    require(receipt['source_kind'] == 'worktree_snapshot' and receipt['evidence_grade'] == 'dev_snapshot'
            and receipt['public_status'] == 'not_claimed' and receipt['commit'] is None, 'grade de provenance')
    require(receipt['source']['head_commit'] == cap['snapshot_head']
            and receipt['source']['files'] == 2262 and receipt['source']['status_entries'] == 34, 'snapshot')
    require(receipt['package_sha256'] == cap['local_artifacts_sha256']['package/package.tar.gz']
            and receipt['plan_sha256'] == cap['local_artifacts_sha256']['package/plan.json']
            and receipt['source']['manifest_sha256'] == cap['local_artifacts_sha256']['package/SNAPSHOT_MANIFEST.sha256'], 'liaison paquet')
    require(receipt['provenance'] == {'binaries_sha256': {}, 'cmakecache': [], 'compiler': []}, 'provenance build différente')
    require(receipt['errors'] == [] and receipt['warnings'] == [] and receipt['results_skipped_members'] == []
            and receipt['overflow'] == {'evicted': [], 'truncated_streams': []}, 'perte ou alerte')

    for folder, expected in cap['commands'].items():
        meta = kv(result_files['results/cmd/' + folder + '/meta.txt'])
        require(all(meta[k] == str(v) for k, v in expected.items()), 'méta commande : ' + folder)
        require(result_files['results/cmd/' + folder + '/stderr'] == b'', 'stderr externe')
    for prefix in ('024_describe_before_stop_1', '025_guarded_stop', '026_describe_after_stop1_1', '027_oslogin_remove'):
        log = json.loads((root / 'host/logs' / (prefix + '.json')).read_bytes())
        require(type(log['exit_code']) is int and log['exit_code'] == 0, 'code clôture')
    observed = json.loads((root / 'host/logs/026_describe_after_stop1_1.stdout').read_bytes())
    require(all(observed.get(k) == v for k, v in receipt['observed_after'].items()), 'description clôture différente')
    require(not any(name.startswith(('results/build/', 'results/provenance/')) for name in result_files), 'nouvelle preuve build')
    for name, digest in before.items():
        require(sha((root / name).read_bytes()) == digest, 'artefact modifié pendant lecture')
    print(json.dumps(dict(ok=True, result_members=members, result_files=len(result_files),
                         result_manifest=len(covered), source_manifest=len(source_covered),
                         targeted_stop_certified=True, source_grade='dev_snapshot',
                         archived_binary_hashes=0, command_codes=[0, 1], native_executed=False), sort_keys=True))


if __name__ == '__main__':
    main()
