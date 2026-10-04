#!/usr/bin/env python3
"""Read frozen JSON metadata only; no product/validator/archive/data/cloud call."""
import hashlib
import json
from pathlib import Path
from statistics import median

ROOT = Path(__file__).resolve().parent


def ms(value):
    return round(value / 1_000_000, 6)


def read():
    before = json.loads((ROOT / 'BEFORE.json').read_text())
    values = {}
    for item in before['files']:
        raw = (ROOT / 'metadata' / item['path']).read_bytes()
        if hashlib.sha256(raw).hexdigest() != item['sha256'] or len(raw) != item['bytes']:
            raise ValueError('snapshot hash: ' + item['path'])
        values[item['path']] = json.loads(raw)
    sessions = []
    for name, receipt in sorted(values.items()):
        if not name.endswith('/receipt.json'):
            continue
        plan_name = name.rsplit('/', 1)[0] + '/plan.json'
        plan_raw = (ROOT / 'metadata' / plan_name).read_bytes()
        sessions.append({
            'session': name.split('/')[1], 'commit': receipt['commit'],
            'worker_source': receipt.get('worker_source'), 'source_kind': receipt.get('source_kind'),
            'status': receipt['status'], 'closure': receipt.get('closure'),
            'reported_after_state': (receipt.get('observed_after') or {}).get('status'),
            'targeted_shutdown_certified': receipt.get('targeted_shutdown_certified'),
            'results_verified_by_controller': receipt.get('results_verified'),
            'results_sha256_declared': receipt.get('results_sha256'),
            'package_sha256_declared': receipt.get('package_sha256'),
            'plan_sha256_declared': receipt.get('plan_sha256'),
            'local_plan_sha256': hashlib.sha256(plan_raw).hexdigest(),
            'plan_hash_matches': hashlib.sha256(plan_raw).hexdigest() == receipt.get('plan_sha256'),
            'failed_commands': (receipt.get('remote_summary') or {}).get('failed_commands'),
            'archive_not_reread': True,
        })
    reports = []
    all_complete_cold = all_complete_warm_dumps = all_complete_warm_passes = 0
    for name, report in sorted(values.items()):
        if not name.endswith('_report.json'):
            continue
        cold, warm = report['cold'], report['warm']
        allrows = cold + warm
        identity_bad = [i for i, row in enumerate(allrows)
                        if row.get('dump_sha256') is None or
                        row.get('dump_sha256') != report['identity'].get(row.get('frame'))]
        final_bad = [i for i, row in enumerate(allrows)
                     if row.get('code') != 0 or (row.get('summary') or {}).get('status') != 'ok']
        warm_bad = [i for i, row in enumerate(warm)
                    if [p.get('pass') for p in row.get('passes', [])] != list(range(1, report['warm_passes'] + 1))
                    or any(p.get('status') != 'ok' for p in row.get('passes', []))]
        batch_examples = []
        for row in cold:
            if row.get('rep') == 0 and row.get('mode') == 'gpu':
                batch = row['summary'].get('batch') or {}
                frame = row['frame']
                ledger = report['ledger'].get(frame) or {}
                batch_examples.append({
                    'frame': frame,
                    **{k: batch.get(k) for k in ('jobs', 'fill_jobs', 'unresolved', 'records', 'population', 'device_bytes')},
                    'records_equal_emitted': batch.get('records') == ledger.get('emitted'),
                    'population_equal_incidences': batch.get('population') == ledger.get('incidences'),
                    'scratch_nonoverflow_records_not_exported': 'scratch_records' not in batch,
                })
        complete = report['verdict'] == 'conforme' and not report['refusals'] and not final_bad and not identity_bad and not warm_bad
        if complete:
            all_complete_cold += len(cold)
            all_complete_warm_dumps += len(warm)
            all_complete_warm_passes += sum(len(row['passes']) for row in warm)
        reports.append({
            'path': name, 'bench_sha256': report['bench_sha256'],
            'build_binary_hashes': [x.get('sha256') for x in report.get('build', []) if x.get('step') in ('built', 'reuse')],
            'kmax': report['kmax'], 'leaf': report['leaf'], 'modes': report['modes'],
            'reps': report['reps'], 'warm_passes_requested': report['warm_passes'],
            'verdict': report['verdict'], 'refusals': len(report['refusals']),
            'cold_rows': len(cold), 'warm_rows_final_dumps': len(warm),
            'warm_pass_events': sum(len(row['passes']) for row in warm),
            'bad_final_rows': len(final_bad), 'bad_final_dump_rows': len(identity_bad),
            'bad_warm_sequences_or_status': len(warm_bad), 'observed_complete': complete,
            'batch_first_gpu_take': batch_examples,
        })
    latest = []
    for filename in ('gpu_k5_report.json', 'gpu_k10_report.json'):
        report = values['sessions/claudegpu5/' + filename]
        for frame in ('lidar_ng00', 'lidar_ng01', 'lidar_ng02'):
            modes = {}
            for mode in ('cpu', 'gpu', 'lot'):
                cold = [r for r in report['cold'] if r['frame'] == frame and r['mode'] == mode]
                warm = [r for r in report['warm'] if r['frame'] == frame and r['mode'] == mode]
                if len(warm) != 1:
                    raise ValueError('unique warm process expected')
                events = warm[0]['passes']
                later = events[1:]
                fields = ('wall_ns', 'domain_ns', 'forest_ns', 'single_pass_ns', 'prefix_ns', 'sort_ns',
                          'batch_executor_ns', 'batch_count_ns', 'batch_fill_ns', 'batch_levels_ns')
                phases = {k.removesuffix('_ns') + '_ms': ms(median([e[k] for e in later]))
                          for k in fields if all(k in e for e in later)}
                modes[mode] = {
                    'cold_processes': len(cold),
                    'cold_wall_ms': median([r['summary']['wall_ms'] for r in cold]),
                    'cold_domain_ms': median([r['summary']['domain_ms'] for r in cold]),
                    'cold_forest_ms': median([r['summary']['forest_ms'] for r in cold]),
                    'warm_processes': 1, 'warm_later_passes': len(later),
                    'warm_first_wall_ms': ms(events[0]['wall_ns']),
                    'warm_first_device_init_ms': ms(events[0]['batch_device_init_ns']),
                    **phases,
                }
            latest.append({'kmax': report['kmax'], 'leaf': report['leaf'], 'frame': frame,
                           'modes': modes,
                           'warm_gpu_cpu_ratio': round(modes['gpu']['wall_ms'] / modes['cpu']['wall_ms'], 6),
                           'cold_gpu_cpu_ratio': round(modes['gpu']['cold_wall_ms'] / modes['cpu']['cold_wall_ms'], 6)})
    return {
        'scope': 'Frozen unversioned JSON metadata, not live session state or native qualification',
        'snapshot_utc': before['captured_utc'], 'actor_HEAD_observed': before['live_head'],
        'metadata_files': len(values), 'README_present_at_snapshot': before['README_present'],
        'sessions': sessions, 'reports': reports, 'latest_gpu5': latest,
        'complete_reports_totals': {'cold_dumps': all_complete_cold,
                                   'warm_final_dumps': all_complete_warm_dumps,
                                   'warm_pass_events': all_complete_warm_passes,
                                   'dump_identity_scope': 'one dump per cold process; last pass only per warm process'},
        'limitations': ['archives/package/binaries not reopened or rehashed',
                        'no native unit/mutant qualification commands in these measurement plans',
                        'aggregate fill_jobs demonstrates overflow/replay but not count of records copied from nonoverflow scratch',
                        'source pins008/b74/16; neither source22 nor later scratch compression is qualified here',
                        'warm passes within one process are not independent cold repetitions',
                        'phase medians overlap and must not be summed or attributed individually'],
    }


if __name__ == '__main__':
    print(json.dumps(read(), indent=2, sort_keys=True))
