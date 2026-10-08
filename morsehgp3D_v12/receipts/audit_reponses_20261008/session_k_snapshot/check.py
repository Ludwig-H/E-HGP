#!/usr/bin/env python3
"""Relit les sources Git et, facultativement, le paquet LOCAL ; aucun controleur execute."""
import argparse
import hashlib
import json
import subprocess
import tarfile
from pathlib import Path


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', required=True, type=Path)
    parser.add_argument('--session-dir', type=Path)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())
    def git(pin, path):
        return subprocess.check_output(['git', 'show', pin + ':' + path], cwd=args.repo)
    def sources():
        for path, sha in cap['source_sha256'].items():
            need(digest(git(cap['snapshot_head'], path)) == sha, 'source de base: ' + path)
            changed = digest(git(cap['d6_fix_commit'], path)) != sha
            need(changed == path.endswith('pilote_d6.py'), 'delta e37: ' + path)
        for path, sha in cap.get('artifact_sha256', {}).items():
            need(digest((here / path).read_bytes()) == sha, 'artefact: ' + path)
        prior = json.loads(git(cap['admission_receipt_pin'], cap['admission_capture_path']))
        need(prior['pilot_sha256'] == cap['source_sha256'][prior['pilot_path']], 'ancien recu pilote FULL')
        for path, sha in prior['git_sources_sha256'].items():
            need(cap['source_sha256'][path] == sha, 'ancien recu sonde/porte')
    sources()
    if args.session_dir:
        folder = args.session_dir
        def package_hashes():
            for path, sha in cap['package_artifacts_sha256'].items():
                need(digest((folder / path).read_bytes()) == sha, 'paquet: ' + path)
        package_hashes()
        preflight = json.loads((folder / 'preflight.json').read_text())
        need(preflight['source']['head_commit'] == cap['snapshot_head'] and
             preflight['source_kind'] == cap['source_kind'] and
             preflight['evidence_grade'] == cap['evidence_grade'] and
             preflight['source']['files'] == cap['source_files'], 'provenance du snapshot')
        plan = json.loads((folder / 'package/plan.json').read_text())
        need([{'name': x['name'], 'timeout_seconds': x['timeout_seconds']} for x in plan['commands']]
             == cap['plan_steps'], 'ordre du plan')
        with tarfile.open(folder / 'package/package.tar.gz', 'r:gz') as archive:
            entries = archive.getmembers()
            for path, sha in cap['source_sha256'].items():
                matches = [x for x in entries if x.name.endswith(path)]
                need(len(matches) == 1 and matches[0].isfile(), 'source unique du paquet')
                need(digest(archive.extractfile(matches[0]).read()) == sha, 'source empaquetee: ' + path)
        package_hashes()
    sources()
    print(json.dumps({'status': 'ok', 'sources': 4, 'd6_old_reader_in_snapshot': True,
                      'full_pilot_matches_published_admission_capture': True,
                      'local_package_checked': args.session_dir is not None,
                      'historical_running_observation_not_replayed': True,
                      'performance_qualified': False, 'gcp_called': False}, sort_keys=True))


if __name__ == '__main__':
    main()
