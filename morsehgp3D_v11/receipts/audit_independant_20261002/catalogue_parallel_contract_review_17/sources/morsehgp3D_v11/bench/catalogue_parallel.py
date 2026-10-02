#!/usr/bin/env python3
"""Bounded W8/W48 catalogue campaign, u21/u24; no FULL or GPU timing claim."""
import argparse
from pathlib import Path
import sys
import time

import catalogue_profiles as profiles

base, semantic = profiles.base, profiles.semantic
SCHEMA = 'ehgp.v11.catalogue_parallel.v1'
TIMEOUT = 15


def schedule():
    """36 declared attempts; whole LiDAR first, growth, repeated W48, W8, then K10."""
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    synthetic = sorted((name for name in profiles.COUNTS if not name.startswith('lidar')),
                       key=profiles.COUNTS.get)
    result = []

    def add(names, kmax, workers, repetitions):
        for name in names:
            for bits in (21, 24):
                for repetition in repetitions:
                    result.append(dict(case=name, coord_bits=bits, kmax=kmax,
                                       workers=workers, repetition=repetition))

    add(lidar + synthetic, 5, 48, (0,))
    add(lidar, 5, 48, (1, 2))
    add(lidar, 5, 8, (0,))
    add(lidar, 10, 48, (0,))
    return result


def identity(row):
    return tuple(row[key] for key in ('case', 'coord_bits', 'kmax', 'workers', 'repetition'))


def check_parallel(row, requested):
    semantic.need(identity(row) == identity(requested), 'attempt identity changed')
    if row['status'] != 'ok':
        return
    try:
        event = row['events'][1]
        semantic.need(type(event['workers']) is int and event['workers'] == requested['workers'],
                      'native worker count differs')
        semantic.need(type(event['pool_ns']) is int and 0 <= event['pool_ns'] < 2**64, 'pool creation time')
        row['pool_ms'] = event['pool_ns'] / 1e6
        row['catalogue_within_200ms'] = event['wall_ns'] < 200_000_000
        row['cloud_pool_catalogue_ms'] = row['cloud_ms'] + row['pool_ms'] + row['catalogue_ms']
    except (ValueError, KeyError, TypeError, OverflowError) as error:
        base.attempt_error(row, 'parallel', error, 'invalid_output')


def comparisons(rows, requested):
    result = []
    keys = sorted({(r['case'], r['kmax']) for r in requested})
    for name, kmax in keys:
        expected = [r for r in requested if (r['case'], r['kmax']) == (name, kmax)]
        found = [r for r in rows if (r['case'], r['kmax']) == (name, kmax) and r['status'] == 'ok']
        digests = {r['semantic']['sha256'] for r in found}
        work = {(profiles.work_signature(r), profiles.q4_signature(r)) for r in found}
        raw_equal = all(len({r['canonical_sha256'] for r in found if r['coord_bits'] == bits}) <= 1
                        for bits in (21, 24))
        equal = len(digests) <= 1 and len(work) <= 1 and raw_equal
        result.append(dict(case=name, kmax=kmax, successful=[identity(r) for r in found],
                           requested=len(expected), semantic_equal=len(digests) <= 1,
                           geometric_work_equal=len(work) <= 1, same_profile_bytes_equal=raw_equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected)
                           else 'incomplete'))
    return result


def run(args):
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    builds = profiles.checked_builds(args)
    supplement = profiles.checked_supplement(args.supplement)
    manifest, manifest_hash = profiles.inputs(args.data)
    cases = {row['name']: row for row in manifest['cases']}
    requested = schedule()
    report = dict(schema=SCHEMA, attempt_schema='ehgp.v11.catalogue_attempt.v2', complete=False,
                  scope='CPU catalogue only; FULL forests/verticals/segmentation/GPU absent',
                  manifest=manifest, manifest_sha256=manifest_hash, builds=list(builds.values()),
                  qualification_sha256=base.digest(args.qualification), supplement_sha256=supplement,
                  requested=requested, requested_runs=len(requested), timeout_seconds=TIMEOUT,
                  native_schedule_bound_seconds=TIMEOUT * len(requested), budget_seconds=args.budget_seconds,
                  timing_scope='API: two passes/sort/output; pool creation and Cloud separately; process includes IO',
                  memory_scope='native Buffer reservations including live Cloud; not RSS or Python decoder',
                  repetitions='three fresh processes for LiDAR K5 W48; other configurations one process',
                  leaf_size=16, max_leaf=256, full_contract='not_testable_missing_tower',
                  runs=[], not_run=[], comparisons=[], full_schedule_completed=False, conforming=False)
    path = args.out / 'parallel.json'

    def save():
        report['comparisons'] = comparisons(report['runs'], requested)
        base.save(path, report)

    save()
    failed_baselines = set()
    for request in requested:
        key = request['case'], request['coord_bits']
        reason = None
        if key in failed_baselines:
            reason = 'same_profile_K5_W48_first_attempt_failed'
        elif time.monotonic() - started + TIMEOUT + 20 >= args.budget_seconds:
            reason = 'campaign_budget_before_launch'
        if reason:
            report['not_run'].append(dict(request, reason=reason))
            save()
            continue
        ordinal = len(report['runs'])

        def checkpoint(row):
            semantic.need(len(report['runs']) == ordinal, 'attempt checkpoint duplicated')
            semantic.need(identity(row) == identity(request), 'checkpoint identity')
            report['runs'].append(row)
            save()

        row = profiles.measure(Path(builds[request['coord_bits']]['path']), cases[request['case']],
                               request['coord_bits'], request['kmax'], args, checkpoint,
                               workers=request['workers'], repetition=request['repetition'], timeout=TIMEOUT)
        check_parallel(row, request)
        semantic.need(len(report['runs']) == ordinal + 1, 'attempt checkpoint absent')
        report['runs'][ordinal] = row
        if row['status'] != 'ok' and request['kmax'] == 5 and request['workers'] == 48 and request['repetition'] == 0:
            failed_baselines.add(key)
        save()
        print('%s B%d K%d W%d r%d %s' % (*identity(request), row['status']), flush=True)
    report['complete'] = True
    report['full_schedule_completed'] = not report['not_run'] and len(report['runs']) == len(requested)
    report['conforming'] = report['full_schedule_completed'] and all(r['status'] == 'ok' for r in report['runs']) and all(
        row['status'] == 'equal' for row in report['comparisons'])
    report['campaign_wall_seconds'] = time.monotonic() - started
    save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('builds', 'data', 'out', 'work', 'qualification', 'supplement'):
        parser.add_argument('--' + option, type=Path, required=True)
    parser.add_argument('--budget-seconds', type=int, default=750)
    args = parser.parse_args()
    if not 60 <= args.budget_seconds <= 3600:
        parser.error('campaign budget outside 60..3600 seconds')
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        print('catalogue_parallel_refused: ' + type(error).__name__, flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
