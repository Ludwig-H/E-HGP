#!/usr/bin/env python3
"""Recalculate three recorded measurements; not a completeness or geometry judge."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re

BINARY_SHA = '4938b94b3166e8c13d02b0fd9687168130d5c528702d7efcf7b7379b3adeb360'
BINARY = '/workspaces/E-HGP/build/v7_no_work_quotas_20260906_controller/build_r1/full_gabriel_lazy_probe'
BUILD_SHA = '25ccae8eb8466280568090963314c9a67a579bc5d81aa43871830a13a6fd7e9d'
RUN_PINS = {
    8000: 'f7c23062b380523ecc6990dbf67c59240455d03e801d513ff028fd96a8613358',
    16000: '153a801ccde6080db04cc76bff6d15a69cf7d21d2de80b9e6f67ac57607d217f',
    32000: '1da95059e10b808c96359e5d1b73c9dba2cf6240a63a249bfe1310df44fba553',
}


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def pinned(path, expected):
    raw = path.read_bytes()
    require(digest(raw) == expected, 'hash mismatch: ' + str(path))
    return raw


def same(actual, expected, reason):
    require(type(actual) is type(expected) and actual == expected, reason)


def results(directory, build_receipt):
    build = json.loads(pinned(build_receipt, BUILD_SHA))
    same(build['binary_sha256'], BINARY_SHA, 'referenced build binary')
    measurements = []
    for n, expected in RUN_PINS.items():
        folder = directory / f'n{n}_s8_punlimited'
        run = json.loads(pinned(folder / 'run.json', expected))
        raw = pinned(folder / 'stdout.jsonl', run['stdout_sha256'])
        stderr = pinned(folder / 'stderr.txt', run['stderr_sha256']).decode()
        command = ['taskset', '-c', '6', '/usr/bin/time', '-v', BINARY, f'--n={n}',
            '--s=8', '--kmax=10', '--alias-policy=lazy', '--cache-entries=1000000', '--meb-proposal-supports=unlimited']
        same(run['command'], command, 'recorded command')
        for key, value in dict(status='completed', exit_code=0, public_status='not_claimed',
                               binary_sha256=BINARY_SHA, binary_sha256_after=BINARY_SHA).items():
            same(run[key], value, 'run: ' + key)
        rows = [json.loads(line) for line in raw.decode().splitlines()]
        require(len(rows) == 12 and rows[0]['type'] == 'configuration' and rows[-1]['type'] == 'terminal', '12 rows')
        config, orders, terminal = rows[0], rows[1:-1], rows[-1]
        for row in rows:
            for key, value in dict(schema='mhgp7-full-gabriel-probe-v5',
                limits_profile='memory_guarded_no_operation_quotas_v1', meb_proposal_budget_kind='unlimited',
                max_meb_proposal_supports_per_order=(1 << 64) - 1, cache_entries=1000000,
                alias_policy='lazy_first_c_strict_resolutions_v1').items():
                same(row[key], value, 'row: ' + key)
        for row in (config, terminal):
            for key, value in dict(n=n, s=8, kmax_requested=10, kmax_effective=10).items():
                same(row[key], value, 'configuration: ' + key)
        same(config['threads'], 1, 'mono-thread configuration')
        for index, row in enumerate(orders, 1):
            same(row['type'], 'order', 'order type')
            same(row['k'], index, 'ten ordered K values')
            same(row['outcome'], 'complete_relative', 'order outcome')
        for key, value in dict(terminal_status='completed', outcome='complete_relative', exit_code=0,
            completed_orders_diagnostic=10, last_order=10, complete_requested_horizontal_orders=True,
            integrated_inter_k_tower=False, certificate_retained=False, public_status='not_claimed').items():
            same(terminal[key], value, 'terminal: ' + key)
        peak = re.findall(r'^\s*Maximum resident set size \(kbytes\): (\d+)$', stderr, re.M)
        require(len(peak) == 1 and re.findall(r'^\s*Exit status: (\d+)$', stderr, re.M) == ['0'], 'GNU time tail')
        elapsed = terminal['elapsed_before_terminal_ms'] / 1000
        require(math.isfinite(elapsed) and elapsed > 0 and math.isfinite(run['elapsed_seconds']), 'elapsed')
        stages = {key: value / 1000 for key, value in terminal['stage_ms'].items()}
        require(all(math.isfinite(v) and v >= 0 for v in stages.values()), 'stage times')
        volumes = {key: terminal[key] for key in ('raw_candidates', 'unique_candidates', 'census_balls',
            'census_merge_peak_bytes', 'wspd_witness_nodes', 'q4_core_site_tests', 'q4_completions')}
        for field in ('minimum_catalogue_records', 'connection_catalogue_records', 'certificate_nodes',
                      'certificate_minima', 'certificate_parent_refs', 'meb_calls', 'meb_proposal_supports',
                      'meb_proposal_certified', 'meb_proposal_fallback', 'meb_reference_supports'):
            require(all(type(row[field]) is int and row[field] >= 0 for row in orders), 'volume: ' + field)
            volumes['sum_K1_to_K10_' + field] = sum(row[field] for row in orders)
        measurements.append(dict(n=n, s=8, proposal='unlimited', kmax=10, orders=10,
            recorder_wall_seconds=run['elapsed_seconds'], elapsed_before_terminal_seconds=elapsed,
            stage_seconds=stages, stage_percent_of_terminal_elapsed={k: v * 100 / elapsed for k, v in stages.items()},
            generation_wspd_seconds=terminal['generation_wspd_ms'] / 1000,
            generation_rects_seconds=terminal['generation_rects_ms'] / 1000,
            GNU_peak_RSS_KiB=int(peak[0]), terminal_sample_RSS_MiB=terminal['rss_mib_sample'],
            terminal_sample_HWM_MiB=terminal['hwm_mib_sample'], volumes=volumes,
            input_digest=terminal['input_digest'], certificate_digest=terminal['certificate_digest']))
    ratios = []
    for old, new in ((measurements[0], measurements[1]), (measurements[1], measurements[2]), (measurements[0], measurements[2])):
        ratios.append(dict(n_from=old['n'], n_to=new['n'], n_ratio=new['n'] / old['n'],
            recorder_wall_ratio=new['recorder_wall_seconds'] / old['recorder_wall_seconds'],
            terminal_elapsed_ratio=new['elapsed_before_terminal_seconds'] / old['elapsed_before_terminal_seconds'],
            stage_ratios={k: new['stage_seconds'][k] / value for k, value in old['stage_seconds'].items() if value > 0},
            volume_ratios={k: new['volumes'][k] / value for k, value in old['volumes'].items() if value > 0},
            GNU_peak_RSS_ratio=new['GNU_peak_RSS_KiB'] / old['GNU_peak_RSS_KiB']))
    return dict(status='recorded_measurements_read', public_status='not_claimed', measurements=measurements,
        scaling_ratios=ratios, geometry_or_catalogue_completeness_judged=False,
        integrated_inter_k_tower=False, slo_claim=False, engine_invoked=False,
        volume_scope='per_order_counts_summed_not_simultaneous_RAM_or_retained_archive',
        ratio_scope='single_observations_of_different_cloud_sizes_not_an_algorithm_speedup')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--build-receipt', type=Path)
    args = parser.parse_args()
    build = args.build_receipt or args.directory.parent / 'full_probe_no_quotas_20260906/build_r1/receipt.json'
    print(json.dumps(results(args.directory, build), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
