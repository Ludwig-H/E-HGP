#!/usr/bin/env python3
"""Relecture de sources et traces publiees ; aucun test natif/GPU."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


def need(ok, message):
    if not ok:
        raise SystemExit(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True, type=Path)
    repo = ap.parse_args().repo.resolve()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())
    root = repo / 'morsehgp3D_v12'

    def verify():
        for rel, h in cap['artifact_hashes'].items():
            need(sha((root / rel).read_bytes()) == h, 'hash: ' + rel)

    verify()
    receipt = json.loads((root / cap['session_receipt']).read_text())
    manifest = {row['path']: row for row in receipt['source']['manifest']}
    for rel in cap['sources']:
        path = 'morsehgp3D_v12/' + rel
        need(manifest[path]['sha256'] == cap['artifact_hashes'][rel], 'source/session: ' + rel)
        for pin in (cap['main_pin'], cap['session_source_pin']):
            data = subprocess.check_output(['git', 'show', pin + ':' + path], cwd=repo)
            need(sha(data) == cap['artifact_hashes'][rel], 'source/Git: ' + rel)
    old = subprocess.check_output(['git', 'show', cap['v11_pin'] + ':morsehgp3D_v11/src/catalogue/single_pass_batch.cpp'], cwd=repo)
    need(sha(old) == cap['v11_source_sha256'], 'v11')
    log = (root / cap['device_log']).read_text()
    need('device_open : voie appareil jouee sur 9 temoins\n' in log, 'voie GPU absente')
    need('test device_open controles=217 echecs=0 plancher=1\n' in log, 'controles GPU')
    need('mhgp12_test_ok tests=1 controles=217\n' in log, 'terminal GPU')
    ctest = (root / cap['ctest_log']).read_text()
    for gate in ('pipeline_witnesses', 'device_open'):
        pattern = r'^.*Test\s+#\d+: mhgp12_catalogue_device_unit_' + gate + r'\s+\.+\s+Passed\s+'
        need(len(re.findall(pattern, ctest, re.M)) == 1, 'porte: ' + gate)
    verify()
    print(json.dumps({'status': 'ok', 'finding': 'CST-0009', 'scope': 'unresolved_leaf_parallel_dispatch',
        'source_files_matched_to_session_and_git': len(cap['sources']), 'artifact_hashes_before_after': len(cap['artifact_hashes']),
        'actual_device_witnesses': 9, 'actual_device_checks': 217,
        'native_executed_by_auditor': False, 'speedup_measured': False, 'global_stage_preadmission_proved': False}, sort_keys=True))


if __name__ == '__main__':
    main()
