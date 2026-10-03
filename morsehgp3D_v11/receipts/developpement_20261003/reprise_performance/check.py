"""Check pinned audit artifacts and standalone models; no native or cloud calls."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
PIN = 'ff7c79dea1e35bd97fae4b36a44cfb7737898e9eea6dee3a83a415f1f2cf506d'


def need(value, message):
    if not value:
        raise ValueError(message)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    raw = (HERE / 'proof.json').read_bytes()
    need(digest(raw) == PIN, 'proof pin')
    proof = json.loads(raw)
    for name, pin in proof['artifacts'].items():
        raw = (HERE / name).read_bytes()
        need(len(raw) == pin['bytes'] and digest(raw) == pin['sha256'], 'artifact ' + name)
    for pin in proof['product_sources']:
        result = subprocess.run(['git', 'show', proof['source'] + ':' + pin['path']],
                                cwd=REPO, capture_output=True, timeout=15)
        need(result.returncode == 0, 'pinned source unavailable')
        need(len(result.stdout) == pin['bytes'] and digest(result.stdout) == pin['sha256'], 'source pin')
    metrics = json.loads((HERE / proof['closed_receipt'] / 'metrics.json').read_text())
    perf = json.loads((HERE / 'performance.json').read_text())
    need(perf['qualified_latest_receipt_source'] == proof['closed_v11_source'], 'qualified source')
    need(perf['source_package_available_at_declared_path'] is False and
         perf['source_package_rehashed_sha256'] is None, 'missing LIVE package must remain explicit')
    selected = {row['case']: row for row in metrics['attempts'] if row['mode'] == 2047 and
                row['coord_bits'] == 21 and row['workers'] == 48}
    for row in perf['comparative_table']:
        current = selected[row['case']]
        need(abs(row['v11_full_ms'] - current['full_ms']) < 1e-9, 'FULL time')
        need(row['v11_phases_ms'] == current['stage_ms'], 'phase times')
        need(row['same_large_forest_canonical_bytes_checked'] is False, 'whole differential remains open')
    for name, model in proof['models'].items():
        for flags in ([], ['-O']):
            result = subprocess.run([sys.executable, '-B', *flags, str(HERE / name)],
                                    capture_output=True, timeout=60)
            need(result.returncode == 0 and not result.stderr, 'model execution ' + name)
            need(digest(result.stdout) == model['stdout_sha256'], 'model output ' + name)
            need(json.loads(result.stdout) == model['result'], 'model result ' + name)
    recovery = json.loads((HERE / 'graph4_recovery.json').read_text())
    absence = json.loads((HERE / 'graph4_oslogin_absence.json').read_text())
    need(recovery['targeted_shutdown_certified'] is True and recovery['closure'] == 'already_terminated',
         'targeted closure')
    need(recovery['oslogin_key_removed'] is False, 'retain failed removal verdict')
    need(absence['gcloud_exit_code'] == 0 and absence['session_key_matches'] == 0 and
         absence['private_key_exists'] is False, 'independent key absence')
    print(json.dumps({'status': 'ok', 'source': proof['source'], 'product_sources': len(proof['product_sources']),
                      'models': len(proof['models']), 'threshold_checks': 829, 'root_union_cases': 900,
                      'native_executions': 0, 'cloud_calls_by_reader': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
