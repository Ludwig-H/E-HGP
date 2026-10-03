#!/usr/bin/env python3
"""Parallel assembly ablation on whole inputs; exact bytes and paid work, no FULL claim."""
import argparse
from pathlib import Path
import sys
import time

import catalogue_diagnostics as diagnostics
import catalogue_parallel as parallel
import semantic_cache as reuse

profiles = parallel.profiles
base, need = profiles.base, profiles.semantic.need
SCHEMA = 'ehgp.v11.catalogue_assembly.v1'
TIMEOUT = 15


def identity(row):
    mode = row['optimizations']
    need(type(mode) is int and mode in (3, 7, 11, 15), 'assembly ablation mode')
    need(row.get('diagnostics', False) is False, 'assembly ablation diagnostics disabled')
    return (*parallel.identity(row), mode)


def schedule():
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    synthetic = sorted((name for name in profiles.COUNTS if not name.startswith('lidar')), key=profiles.COUNTS.get)
    return [dict(case=name, coord_bits=bits, kmax=5, workers=48, repetition=0,
                 optimizations=mode, diagnostics=False)
            for names, modes in ((lidar, (3, 11, 7, 15)), (synthetic, (3, 11)))
            for name in names for bits in (21, 24) for mode in modes]


def check_assembly(row, requested):
    need(identity(row) == identity(requested), 'assembly attempt identity changed')
    parallel.check_parallel(row, requested)
    if row['status'] != 'ok':
        return
    try:
        need(row['stderr'] == '', 'unexpected native stderr')
        event = row['events'][1]
        need(not diagnostics.check_request(row, event), 'unsolicited task diagnostics')
        work = event['cache_work']
        need(type(work) is dict and set(work) == {'evaluations', 'hits', 'fallbacks'} and
             all(diagnostics.uint(v) for v in work.values()), 'cache work fields/values')
        need(event['logical']['region_line_tests'] == work['evaluations'] + work['hits'] and
             work['fallbacks'] <= work['evaluations'], 'cache request inventory')
        row['cache_work'] = dict(work)
    except (ValueError, KeyError, TypeError, OverflowError) as error:
        base.attempt_error(row, 'assembly', error, 'invalid_output')


def comparisons(rows, requested):
    wanted, actual = set(map(identity, requested)), list(map(identity, rows))
    need(len(wanted) == len(requested) and len(actual) == len(set(actual)) and set(actual) <= wanted,
         'assembly comparison inventory')
    result = []
    for name, kmax in sorted({(r['case'], r['kmax']) for r in requested}):
        expected = [r for r in requested if (r['case'], r['kmax']) == (name, kmax)]
        found = [r for r in rows if (r['case'], r['kmax']) == (name, kmax) and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        work_equal = len({(profiles.work_signature(r), profiles.q4_signature(r)) for r in found}) <= 1
        cache_equal = len({tuple(sorted(r['events'][1]['cache_work'].items())) for r in found}) <= 1
        raw_equal = all(len({r['canonical_sha256'] for r in found if r['coord_bits'] == bits}) <= 1 for bits in (21, 24))
        equal = semantic_equal and work_equal and cache_equal and raw_equal
        result.append(dict(case=name, kmax=kmax, requested=len(expected), successful=[identity(r) for r in found],
                           semantic_equal=semantic_equal, geometric_work_equal=work_equal,
                           cache_work_equal=cache_equal, same_profile_bytes_equal=raw_equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete'))
    return result


def run(args):
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    builds = profiles.checked_builds(args)
    supplement = profiles.checked_supplement(args.supplement)
    manifest, manifest_hash = profiles.inputs(args.data)
    semantic_cache = reuse.SummaryCache() if getattr(args, 'reuse_semantic', False) else None
    cases = {r['name']: r for r in manifest['cases']}
    requested = schedule()
    report = dict(schema=SCHEMA, attempt_schema='ehgp.v11.catalogue_attempt.v2', complete=False, conforming=False,
                  manifest=manifest, manifest_sha256=manifest_hash, builds=list(builds.values()),
                  qualification_sha256=base.digest(args.qualification), supplement_sha256=supplement,
                  requested=requested, requested_runs=len(requested), timeout_seconds=TIMEOUT,
                  native_schedule_bound_seconds=TIMEOUT*len(requested), budget_seconds=args.budget_seconds,
                  optimization_modes={'3': 'fixed_serial_assembly', '11': 'fixed_parallel_assembly',
                                      '7': 'adaptive_serial_assembly', '15': 'adaptive_parallel_assembly'},
                  scope='whole integer inputs; CPU catalogue K5; no FULL/segmentation/GPU claim',
                  timing_scope='API includes all assembly work and allocations; Cloud/Pool separate; process includes IO',
                  diagnostic_scope='disjoint non-exhaustive stage walls; task sums/max refer to generation, not assembly',
                  memory_scope='Buffer reservations including live Cloud and assembly metadata; not RSS or Python decoder',
                  comparison_scope='semantic/logical/q4/cache across modes/profiles; bytes within profile; exact sorting unchanged',
                  precision='identical 1mm integer inputs in u21/u24; not finer quantization',
                  repetitions='one fresh process per mode/profile; LiDAR 3,11,7,15; uniform 3,11; all W48',
                  omission_policy='budget only; no failed mode suppresses another attempt',
                  leaf_size=16, max_leaf=256, runs=[], launch_intents=[], not_run=[], comparisons=[],
                  full_schedule_completed=False)
    if semantic_cache is not None:
        report['semantic_reuse'] = dict(schema=reuse.SCHEMA, capacity=semantic_cache.capacity,
            summary_limit_bytes=reuse.SUMMARY_LIMIT, assumption='SHA256 collision resistance; immutable campaign artifacts',
            scope='current payload fully rehashed; only validated summaries reused; all current-event checks repeated')
    path = args.out/'assembly.json'

    def save():
        report['comparisons'] = comparisons(report['runs'], requested)
        base.save(path, report)

    save()
    for request in requested:
        if time.monotonic()-started+TIMEOUT+20 >= args.budget_seconds:
            report['not_run'].append(dict(request, reason='campaign_budget_before_launch')); save(); continue
        ordinal = len(report['runs'])
        report['launch_intents'].append(profiles.launch_intent(Path(builds[request['coord_bits']]['path']),
            cases[request['case']], request['coord_bits'], request['kmax'], args, request['workers'],
            request['repetition'], TIMEOUT, request['optimizations'], diagnostics=False))
        if semantic_cache is not None:
            report['launch_intents'][-1]['semantic_reuse_requested'] = True
        save()

        def checkpoint(row):
            need(len(report['runs']) == ordinal and identity(row) == identity(request), 'assembly checkpoint')
            report['runs'].append(row); save()

        options = dict(workers=request['workers'], repetition=request['repetition'], timeout=TIMEOUT,
                       optimizations=request['optimizations'], diagnostics=False)
        if semantic_cache is not None:
            options['semantic_cache'] = semantic_cache
            options['validate_attempt'] = lambda row: check_assembly(row, request)
        row = profiles.measure(Path(builds[request['coord_bits']]['path']), cases[request['case']],
            request['coord_bits'], request['kmax'], args, checkpoint, **options)
        if semantic_cache is None:
            check_assembly(row, request)
        need(len(report['runs']) == ordinal+1, 'missing assembly checkpoint')
        report['runs'][ordinal] = row
        save()
        print('%s B%d K%d W%d r%d mode%d %s' % (*identity(request), row['status']), flush=True)
    report['complete'] = True
    report['full_schedule_completed'] = not report['not_run'] and len(report['runs']) == len(requested)
    report['conforming'] = report['full_schedule_completed'] and all(r['status'] == 'ok' for r in report['runs']) and all(
        c['status'] == 'equal' for c in report['comparisons'])
    report['campaign_wall_seconds'] = time.monotonic()-started
    save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('builds', 'data', 'out', 'work', 'qualification', 'supplement'):
        parser.add_argument('--'+option, type=Path, required=True)
    parser.add_argument('--budget-seconds', type=int, default=700)
    parser.add_argument('--reuse-semantic', action='store_true',
                        help='reuse validated summaries within this campaign after complete SHA256 rereads')
    args = parser.parse_args()
    if not 60 <= args.budget_seconds <= 700:
        parser.error('campaign budget outside 60..700 seconds')
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        print('catalogue_assembly_refused: '+type(error).__name__, flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
