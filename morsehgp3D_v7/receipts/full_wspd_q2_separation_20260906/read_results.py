#!/usr/bin/env python3
"""Read closed q2/separation measurements; no engine, oracle or speedup claim."""
import argparse
import hashlib
import json
from pathlib import Path
import re

BUILD_SHA = '1d5f0b319192274cf0b231a2bcb0c444a9239d7dd52208dfcf09b9eb0063deaa'
BINARY_SHA = '23646a320b4dee52c626cb3fb5a0fcc54822a2ff4b093e661b125f7a581ffe07'
BINARY = '/workspaces/E-HGP/build/v7_wspd_probe_20260906/full_probe'
OLD_BINARY = '/workspaces/E-HGP/build/v7_no_work_quotas_20260906_controller/build_r1/full_gabriel_lazy_probe'
OLD_BINARY_SHA = '4938b94b3166e8c13d02b0fd9687168130d5c528702d7efcf7b7379b3adeb360'
OLD_RUN_SHA = 'f7c23062b380523ecc6990dbf67c59240455d03e801d513ff028fd96a8613358'
RUN_PINS = {
    'micro_n8': '23d003f1b465a1f28409b3146c4e7747c4939361a0c44054dbb18a5e56f221ec',
    'n8000_s8_punlimited': 'e4ccecc539ee7fc95484144493fb6f8522b90720f453af73e23659cfb3ea2eb5',
    'n8000_s10_punlimited': '42b97553557d81e6d75d3860f4fdaedacb4b03be3ff03ac359f24e0ef3055671',
    'n8000_s12_punlimited': '6d97e33fc80932a45fff1727925f4bcb4e227f98826f87f7a2e5b840c3caa280',
}
ORDER_MEASURES = {'build_ms', 'digest_ms', 'expand_ms', 'read_ms', 'release_ms', 'rss_mib_sample', 'hwm_mib_sample'}
TERMINAL_MEASURES = {'stage_ms', 'generation_wspd_ms', 'generation_rects_ms', 'elapsed_before_terminal_ms',
    'provisional_output_ms', 'digest_ms', 'compute_read_release_ms_subtracted_diagnostic', 'rss_mib_sample', 'hwm_mib_sample'}


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def same(a, b):
    return json.dumps(a, sort_keys=True, allow_nan=False) == json.dumps(b, sort_keys=True, allow_nan=False)


def checked(path, expected):
    raw = path.read_bytes()
    require(type(expected) is str and sha(raw) == expected, 'hash:' + str(path))
    return raw


def nonmeasures(row, excluded):
    return {key: value for key, value in row.items() if key not in excluded}


def deltas(before, after):
    def times(value):
        return dict(value['stage_seconds'], recorder_wall=value['recorder_wall_seconds'],
            terminal_elapsed=value['elapsed_before_terminal_seconds'],
            WSPD=value['generation_wspd_seconds'], rectangles=value['generation_rects_seconds'])
    a, b = times(before), times(after)
    return {key: dict(after_minus_before_seconds=b[key] - value,
                     after_minus_before_percent=100 * (b[key] / value - 1)) for key, value in a.items()}


def load(directory, expected, n, s, binary, binary_pin):
    run = json.loads(checked(directory / 'run.json', expected))
    rows = [json.loads(line) for line in checked(directory / 'stdout.jsonl', run['stdout_sha256']).decode().splitlines()]
    stderr = checked(directory / 'stderr.txt', run['stderr_sha256']).decode()
    for key, value in dict(status='completed', exit_code=0, public_status='not_claimed',
                           binary_sha256=binary_pin, binary_sha256_after=binary_pin).items():
        require(same(run[key], value), 'run:' + key)
    argv = ['taskset', '-c', '6', '/usr/bin/time', '-v', binary, f'--n={n}', f'--s={s}',
        '--kmax=10', '--alias-policy=lazy', '--cache-entries=1000000', '--meb-proposal-supports=unlimited']
    require(same(run['command'], argv), 'command')
    count = min(n, 10)
    require([row['type'] for row in rows] == ['configuration'] + ['order'] * count + ['terminal'], 'row sequence')
    config, orders, terminal = rows[0], rows[1:-1], rows[-1]
    for row in rows:
        for key, value in dict(schema='mhgp7-full-gabriel-probe-v5', meb_proposal_budget_kind='unlimited',
            max_meb_proposal_supports_per_order=(1 << 64) - 1).items():
            require(same(row[key], value), 'profile:' + key)
    for row in (config, terminal):
        for key, value in dict(n=n, s=s, kmax_requested=10, kmax_effective=count).items():
            require(same(row[key], value), 'domain:' + key)
    for k, row in enumerate(orders, 1):
        require(same(row['k'], k) and row['outcome'] == 'complete_relative', 'order completion')
    for key, value in dict(terminal_status='completed', outcome='complete_relative', exit_code=0,
        completed_orders_diagnostic=count, complete_requested_horizontal_orders=True,
        integrated_inter_k_tower=False, public_status='not_claimed').items():
        require(same(terminal[key], value), 'terminal:' + key)
    peak = re.findall(r'Maximum resident set size \(kbytes\): (\d+)', stderr)
    require(len(peak) == 1, 'GNU RSS')
    result = dict(n=n, s=s, orders=count, run_sha256=expected,
        recorder_wall_seconds=run['elapsed_seconds'], elapsed_before_terminal_seconds=terminal['elapsed_before_terminal_ms'] / 1000,
        generation_wspd_seconds=terminal['generation_wspd_ms'] / 1000,
        generation_rects_seconds=terminal['generation_rects_ms'] / 1000,
        stage_seconds={k: v / 1000 for k, v in terminal['stage_ms'].items()}, GNU_peak_RSS_KiB=int(peak[0]),
        counters={key: terminal[key] for key in ('raw_candidates', 'census_balls', 'wspd_witness_nodes',
            'wspd_corner_evals', 'q4_core_site_tests', 'q4_completions', 'prefilter_query_nodes', 'census_query_nodes')},
        input_digest=terminal['input_digest'], final_digest=terminal['certificate_digest'],
        order_digests=[row['certificate_digest'] for row in orders])
    return result, config, orders, terminal


def results(directory, old_s8):
    build = json.loads(checked(directory / 'build.json', BUILD_SHA))
    require(build['status'] == 'completed' and same(build['exit_code'], 0)
            and build['binary_sha256'] == BINARY_SHA and build['dependencies'] == 42, 'build metadata')
    sources = build['project_sources']
    require(len(sources) == 42, 'project dependencies')
    for name, expected in sources.items():
        checked(directory / 'source_snapshot' / name, expected)
    require(all(type(value) is str for value in RUN_PINS.values()), 'all three runs must be closed and pinned')
    micro = load(directory / 'micro_n8', RUN_PINS['micro_n8'], 8, 8, BINARY, BINARY_SHA)[0]
    current = [load(directory / f'n8000_s{s}_punlimited', RUN_PINS[f'n8000_s{s}_punlimited'], 8000, s, BINARY, BINARY_SHA)
               for s in (8, 10, 12)]
    old = load(old_s8, OLD_RUN_SHA, 8000, 8, OLD_BINARY, OLD_BINARY_SHA)
    old_value, old_config, old_orders, old_terminal = old
    value, config, orders, terminal = current[0]
    old_order_equal = all(same(nonmeasures(a, ORDER_MEASURES), nonmeasures(b, ORDER_MEASURES))
                          for a, b in zip(old_orders, orders))
    return dict(public_status='not_claimed', engine_invoked=False, micro=micro,
        measurements=[item[0] for item in current], old_s8=old_value,
        old_vs_new_s8=dict(configuration_equal=same(old_config, config), order_nonmeasures_equal=old_order_equal,
            terminal_nonmeasures_equal=same(nonmeasures(old_terminal, TERMINAL_MEASURES), nonmeasures(terminal, TERMINAL_MEASURES)),
            elapsed_old_over_new=old_value['elapsed_before_terminal_seconds'] / value['elapsed_before_terminal_seconds'],
            elapsed_reduction_percent=100 * (1 - value['elapsed_before_terminal_seconds'] / old_value['elapsed_before_terminal_seconds']),
            timing_changes_by_phase=deltas(old_value, value),
            wspd_witness_nodes_equal=old_terminal['wspd_witness_nodes'] == terminal['wspd_witness_nodes'],
            wspd_corner_evals_equal=old_terminal['wspd_corner_evals'] == terminal['wspd_corner_evals']),
        across_s=dict(input_digests_equal=len({v[0]['input_digest'] for v in current}) == 1,
            final_digests_equal=len({v[0]['final_digest'] for v in current}) == 1,
            ten_order_digests_equal=all(same(current[0][0]['order_digests'], v[0]['order_digests']) for v in current),
            order_nonmeasures_equal=all(all(same(nonmeasures(a, ORDER_MEASURES), nonmeasures(b, ORDER_MEASURES))
                for a, b in zip(current[0][2], v[2])) for v in current),
            changes_from_s8={str(v[0]['s']): deltas(current[0][0], v[0]) for v in current[1:]}),
        geometry_or_completeness_judged=False, integrated_inter_k_tower=False, robust_speedup_claim=False,
        note='single observations; identical old/new s8 visit counters do not demonstrate a visit reduction on this cloud')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--old-s8', type=Path)
    args = parser.parse_args()
    old = args.old_s8 or args.directory.parent / 'full_direct_scaling_20260906/n8000_s8_punlimited'
    print(json.dumps(results(args.directory, old), indent=2, sort_keys=True))
