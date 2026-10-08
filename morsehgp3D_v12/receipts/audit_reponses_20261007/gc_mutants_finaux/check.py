#!/usr/bin/env python3
"""Relecture des 18 mutants Gc finaux ; aucune execution native."""
import argparse
import hashlib
import io
import json
import runpy
import subprocess
import tarfile
from pathlib import Path


def need(ok, why):
    if not ok:
        raise SystemExit(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--scratch', required=True, type=Path)
    root = ap.parse_args().scratch.resolve()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())
    helper = here.parent / 'gc_rebase_portes/check.py'
    need(sha(helper.read_bytes()) == cap['tree_reader_sha256'], 'lecteur anterieur modifie')
    tree_reader = runpy.run_path(str(helper))
    source = root / 'repo3/morsehgp3D_v12'

    def verify():
        for rel, want in cap['artifact_hashes'].items():
            need(sha((root / rel).read_bytes()) == want, 'hash: ' + rel)
        prefix = '\n'.join((root / 'runs/final_checks.log').read_text().splitlines()[:8]) + '\n'
        need(sha(prefix.encode()) == cap['driver_prefix_sha256'], 'code externe modifie')
        need(tree_reader['current_tree'](source) == (359, cap['source_tree_sha256']), 'arbre source')

    verify()
    archive = subprocess.check_output(['git', 'archive', cap['prototype_pin'], 'morsehgp3D_v12'], cwd=root / 'git3')
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        for item in tar:
            rel = item.name.removeprefix('morsehgp3D_v12/')
            if item.isfile() and rel.split('/')[0] in tree_reader['TOPS']:
                need((source / rel).read_bytes() == tar.extractfile(item).read(), 'archive: ' + rel)
    report = json.loads((root / 'runs/mutants_tower_final.json').read_text())
    manifest = json.loads((source / 'tests/mutants/tower.json').read_text())
    need(report['schema'] == 'mhgp12.mutants.v1' and report['module'] == 'tower', 'schema')
    need(type(report['code']) is int and report['code'] == 0 and report['temoin'] == 'vert', 'statut')
    need(type(report['plancher']) is int and report['plancher'] == manifest['plancher'] == 18, 'plancher')
    need(report['sources_sha256'] == cap['source_tree_sha256'], 'rapport sur autre arbre')
    need(report['manifeste_sha256'] == sha((source / 'tests/mutants/tower.json').read_bytes()), 'manifeste')
    expected = [{'id': x['id'], 'verdict': 'TUE', 'detail': 'code', 'juge': x['porte']} for x in manifest['mutants']]
    need(len(expected) == len({x['id'] for x in expected}) == 18 and report['mutants'] == expected, 'cohorte/causes')
    log = (root / 'runs/mutants_tower_final.log').read_text()
    need(log.splitlines()[-1] == cap['terminal_line'], 'terminal')
    for row in expected:
        need(any(line.split() == [row['id'], 'TUE', 'code'] for line in log.splitlines()), 'ligne mutant')
    verify()
    print(json.dumps({'status': 'ok', 'prototype_pin': cap['prototype_pin'], 'source_files': 359,
        'source_tree_sha256': cap['source_tree_sha256'], 'mutants': 18, 'killed_by_code': 18,
        'signals': 0, 'timeouts': 0, 'build_failures': 0, 'external_code': 0,
        'artifact_hashes_before_after': len(cap['artifact_hashes']), 'precise_failing_check_archived': False,
        'old_campaign_transferred': False, 'native_executed_by_auditor': False}, sort_keys=True))


if __name__ == '__main__':
    main()
