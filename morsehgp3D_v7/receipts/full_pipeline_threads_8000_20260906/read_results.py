#!/usr/bin/env python3
"""Read four closed, externally pinned v6 runs. No subprocess or engine."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re

BINARY = '/workspaces/E-HGP/build/v7_full_pipeline_threads_20260906/full_probe'
BINARY_SHA = '4f5ba475ae5075cabab6c84222742b2dcdae226a0b3879e460b7fb8200b76aff'
ORDER_MEASURES = {'build_ms', 'digest_ms', 'expand_ms', 'read_ms', 'release_ms', 'rss_mib_sample', 'hwm_mib_sample'}
TERMINAL_MEASURES = {'stage_ms', 'generation_wspd_ms', 'generation_rects_ms', 'elapsed_before_terminal_ms',
    'provisional_output_ms', 'digest_ms', 'compute_read_release_ms_subtracted_diagnostic', 'rss_mib_sample', 'hwm_mib_sample'}
WORKERS = {'workers_wspd', 'workers_rects', 'workers_sort', 'workers_prefilter', 'workers_census', 'workers_expand'}
PEAKS = {'candidate_capacity_observed', 'ball_capacity_observed', 'wave_peak_tasks', 'alive_peak_rects', 'census_merge_peak_bytes'}
WORK = {'wspd_witness_nodes', 'wspd_corner_evals', 'q4_core_site_tests', 'q4_completions',
        'prefilter_query_nodes', 'census_query_nodes', 'last_order_work'}
VOLUMES = {'raw_candidates', 'unique_candidates', 'census_balls', 'pair_mass_expected_per_lane',
           'ledger_q2_emitted', 'ledger_q3_emitted', 'ledger_q4_emitted',
           'ledger_q2_killed', 'ledger_q3_killed', 'ledger_q4_killed'}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def checked(path, expected):
    raw = path.read_bytes()
    need(type(expected) is str and re.fullmatch('[0-9a-f]{64}', expected) and digest(raw) == expected,
         'hash:' + str(path))
    return raw


def unique(pairs):
    value = {}
    for key, item in pairs:
        need(key not in value, 'duplicate JSON key')
        value[key] = item
    return value


def loads(raw):
    return json.loads(raw, object_pairs_hook=unique, parse_constant=lambda _: need(False, 'nonfinite JSON'))


def same(a, b):
    return json.dumps(a, sort_keys=True, allow_nan=False) == json.dumps(b, sort_keys=True, allow_nan=False)


def without(row, removed):
    return {key: value for key, value in row.items() if key not in removed}


def aggregate(input_hash, order_hashes):
    value = hashlib.sha256()
    def number(item):
        value.update(item.to_bytes(8, 'little'))
    def tag(item):
        raw = item.encode('ascii')
        number(len(raw))
        value.update(raw)
    tag('mhgp7-full-semantic-v1:horizontal-orders')
    tag(input_hash)
    number(len(order_hashes))
    for index, item in enumerate(order_hashes, 1):
        number(index)
        tag(item)
    return value.hexdigest()


def load(directory, threads, pin):
    run = loads(checked(directory / 'run.json', pin))
    rows = [loads(line) for line in checked(directory / 'stdout.jsonl', run['stdout_sha256']).decode().splitlines()]
    stderr = checked(directory / 'stderr.txt', run['stderr_sha256']).decode()
    for key, value in dict(status='completed', exit_code=0, public_status='not_claimed', binary_sha256=BINARY_SHA,
                           binary_sha256_after=BINARY_SHA, pipeline_threads_requested=threads).items():
        need(same(run[key], value), 'run:' + key)
    cpu_list = run['cpu_list_requested']
    need(type(cpu_list) is str and re.fullmatch(r'[0-9]+(?:-[0-9]+)?(?:,[0-9]+(?:-[0-9]+)?)*', cpu_list), 'affinity')
    argv = ['taskset', '-c', cpu_list, '/usr/bin/time', '-v', BINARY, '--n=8000', '--s=8', '--kmax=10',
            '--alias-policy=lazy', '--cache-entries=1000000', '--meb-proposal-supports=unlimited', f'--threads={threads}']
    need(same(run['command'], argv) and type(run['pid']) is int and run['pid'] > 0, 'command')
    need([row['type'] for row in rows] == ['configuration'] + ['order'] * 10 + ['terminal'], 'ten_order_sequence')
    config, orders, terminal = rows[0], rows[1:-1], rows[-1]
    for row in rows:
        for key, value in dict(schema='mhgp7-full-gabriel-probe-v6', limits_profile='memory_guarded_no_operation_quotas_v1',
            parallel_profile='pipeline_workers_full_order_serial_v1', pipeline_threads=threads, full_order_builder_threads=1,
            order_schedule='sequential_k1_to_kmax', meb_proposal_budget_kind='unlimited',
            max_meb_proposal_supports_per_order=(1 << 64)-1, cache_entries=1000000,
            alias_policy='lazy_first_c_strict_resolutions_v1').items():
            need(same(row[key], value), 'profile:' + key)
    for row in (config, terminal):
        for key, value in dict(n=8000, s=8, kmax_requested=10, kmax_effective=10, public_status='not_claimed').items():
            need(same(row[key], value), 'domain:' + key)
    need(same(config['threads'], threads), 'requested_threads')
    for k, row in enumerate(orders, 1):
        need(same(row['k'], k) and row['outcome'] == 'complete_relative' and row['whole_tower_authority'] is False,
             'order_completion')
    for key, value in dict(terminal_status='completed', outcome='complete_relative', exit_code=0,
        completed_orders_diagnostic=10, complete_requested_horizontal_orders=True, integrated_inter_k_tower=False,
        certificate_retained=False, digest_proves_catalogue_completeness=False).items():
        need(same(terminal[key], value), 'terminal:' + key)
    need(all(type(terminal[key]) is int and 0 <= terminal[key] <= threads for key in WORKERS), 'worker_range')
    order_hashes = [row['certificate_digest'] for row in orders]
    need(all(type(value) is str and re.fullmatch('[0-9a-f]{64}', value)
             for value in [terminal['input_digest'], terminal['certificate_digest'], *order_hashes]), 'digest_format')
    need(terminal['certificate_digest'] == aggregate(terminal['input_digest'], order_hashes), 'final_digest_binding')
    peak = re.findall(r'^\s*Maximum resident set size \(kbytes\): (\d+)\s*$', stderr, re.MULTILINE)
    exits = re.findall(r'^\s*Exit status: (\d+)\s*$', stderr, re.MULTILINE)
    need(len(peak) == 1 and exits == ['0'], 'GNU_peak_and_exit')
    times = dict(terminal['stage_ms'], generation_wspd=terminal['generation_wspd_ms'],
                 generation_rectangles=terminal['generation_rects_ms'], total=terminal['elapsed_before_terminal_ms'])
    need(all(type(value) in (int, float) and math.isfinite(value) and value >= 0 for value in times.values()) and
         type(run['elapsed_seconds']) in (int, float) and math.isfinite(run['elapsed_seconds']) and run['elapsed_seconds'] > 0,
         'nonnegative_timings')
    measures = dict(threads=threads, cpu_list_requested=cpu_list, run_sha256=pin,
        recorder_wall_seconds=run['elapsed_seconds'], seconds={key: value/1000 for key, value in times.items()},
        GNU_peak_RSS_KiB=int(peak[0]), terminal_rss_mib_sample=terminal['rss_mib_sample'],
        terminal_hwm_mib_sample=terminal['hwm_mib_sample'], workers={key: terminal[key] for key in sorted(WORKERS)},
        parallel_storage_peaks={key: terminal[key] for key in sorted(PEAKS)},
        front_work={key: terminal[key] for key in sorted(WORK - {'last_order_work'})},
        front_volumes={key: terminal[key] for key in sorted(VOLUMES)},
        input_digest=terminal['input_digest'], final_digest=terminal['certificate_digest'], order_digests=order_hashes)
    return config, orders, terminal, measures


def compare(directory, pins):
    arms = {threads: load(directory / f'n8000_t{threads}', threads, pins[threads]) for threads in (1, 2, 4, 8)}
    base_config, base_orders, base_terminal, base_measures = arms[1]
    comparisons = {}
    for threads, (config, orders, terminal, measures) in arms.items():
        need(same(without(config, {'threads', 'pipeline_threads'}), without(base_config, {'threads', 'pipeline_threads'})),
             'configuration_except_N:' + str(threads))
        need(same([without(row, ORDER_MEASURES | {'pipeline_threads'}) for row in orders],
                  [without(row, ORDER_MEASURES | {'pipeline_threads'}) for row in base_orders]),
             'all_order_nonmeasurements:' + str(threads))
        need(same(terminal['input_digest'], base_terminal['input_digest']) and
             same(terminal['certificate_digest'], base_terminal['certificate_digest']), 'input_final_pair')
        changes = {name: {} for name in ('workers', 'parallel_storage_peaks', 'front_work', 'front_volumes', 'other_nonmeasurement')}
        for key in sorted(set(terminal) | set(base_terminal)):
            if key in TERMINAL_MEASURES | {'pipeline_threads'} or same(terminal.get(key), base_terminal.get(key)):
                continue
            category = ('workers' if key in WORKERS else 'parallel_storage_peaks' if key in PEAKS else
                        'front_work' if key in WORK else 'front_volumes' if key in VOLUMES else 'other_nonmeasurement')
            changes[category][key] = dict(t1=base_terminal.get(key), current=terminal.get(key))
        times = {key: dict(t1_seconds=base_measures['seconds'][key], current_seconds=value,
                          t1_over_current=base_measures['seconds'][key]/value if value else None,
                          current_minus_t1_seconds=value-base_measures['seconds'][key]) for key, value in measures['seconds'].items()}
        comparisons[str(threads)] = dict(terminal_nonmeasurement_changes=changes, phase_comparison=times,
            recorder_wall_t1_over_current=base_measures['recorder_wall_seconds']/measures['recorder_wall_seconds'])
    return dict(status='closed_runs_compared', public_status='not_claimed', engine_invoked=False,
        same_binary_sha256=BINARY_SHA, configuration_equal_except_N=True, ten_order_nonmeasurement_fields_equal=True,
        input_ten_order_and_final_digests_equal=True, measurements=[arms[n][3] for n in (1, 2, 4, 8)],
        comparisons_to_t1=comparisons, whole_tower_completeness_judged=False, robust_speedup_claim=False,
        closure_evidence='runner wait exit0 + GNU Exit status0 + terminal completed; no independent process-group certificate',
        note='Single observations; FULL builders remain sequential. GNU peak RSS and sampled RSS/HWM are separate measurements.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--run-pin', action='append', required=True, help='N=SHA256, once for each of 1,2,4,8')
    args = parser.parse_args()
    pins = {}
    for value in args.run_pin:
        key, separator, pin = value.partition('=')
        need(separator == '=' and key in ('1', '2', '4', '8') and int(key) not in pins and
             re.fullmatch('[0-9a-f]{64}', pin), 'explicit_unique_run_pin')
        pins[int(key)] = pin
    need(set(pins) == {1, 2, 4, 8}, 'four_closed_external_pins_required')
    print(json.dumps(compare(args.directory.resolve(), pins), indent=2, sort_keys=True))
