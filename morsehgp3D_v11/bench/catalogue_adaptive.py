#!/usr/bin/env python3
"""Adaptive frontier ablation on complete integer inputs; source-local collector, no FULL claim."""
import argparse
import json
from pathlib import Path
import sys
import time

import catalogue_diagnostics as diagnostics
import catalogue_parallel as parallel

profiles = parallel.profiles
base, need = profiles.base, profiles.semantic.need
SCHEMA = 'ehgp.v11.catalogue_adaptive.v1'
TIMEOUT = 15


def identity(row):
    mode = row['optimizations']
    need(type(mode) is int and mode in (3, 7), 'adaptive ablation mode')
    need(row.get('diagnostics') is True, 'ablation requires owned diagnostics')
    return (*parallel.identity(row), mode)


def schedule():
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    synthetic = sorted((name for name in profiles.COUNTS if not name.startswith('lidar')), key=profiles.COUNTS.get)
    return [dict(case=name, coord_bits=bits, kmax=5, workers=workers, repetition=0,
                 optimizations=mode, diagnostics=True)
            for names, workers in ((lidar, 48), (lidar, 8), (synthetic, 48))
            for name in names for bits in (21, 24) for mode in (3, 7)]


def check_adaptive(row, requested):
    need(identity(row) == identity(requested), 'adaptive attempt identity changed')
    parallel.check_parallel(row, requested)
    if row['status'] != 'ok':
        return
    try:
        need(row['stderr'] == '', 'unexpected native stderr')
        event = row['events'][1]
        need(diagnostics.check_request(row, event), 'missing requested diagnostics')
        row['diagnostic_summary'] = diagnostics.check(event, row['count'], row['coord_bits'])
        work = event['cache_work']
        need(type(work) is dict and set(work) == {'evaluations', 'hits', 'fallbacks'} and
             all(diagnostics.uint(v) for v in work.values()), 'cache work fields/values')
        need(event['logical']['region_line_tests'] == work['evaluations'] + work['hits'] and
             work['fallbacks'] <= work['evaluations'], 'cache request inventory')
        row['cache_work'] = dict(work)
    except (ValueError, KeyError, TypeError, OverflowError) as error:
        base.attempt_error(row, 'adaptive', error, 'invalid_output')


def comparisons(rows, requested):
    wanted, actual = set(map(identity, requested)), list(map(identity, rows))
    need(len(wanted) == len(requested) and len(actual) == len(set(actual)) and set(actual) <= wanted,
         'adaptive comparison inventory')
    result = []
    for name, kmax in sorted({(r['case'], r['kmax']) for r in requested}):
        expected = [r for r in requested if (r['case'], r['kmax']) == (name, kmax)]
        found = [r for r in rows if (r['case'], r['kmax']) == (name, kmax) and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        work_equal = len({(profiles.work_signature(r), profiles.q4_signature(r)) for r in found}) <= 1
        cache_equal = len({tuple(sorted(r['events'][1]['cache_work'].items())) for r in found}) <= 1
        raw_equal = all(len({r['canonical_sha256'] for r in found if r['coord_bits'] == bits}) <= 1 for bits in (21, 24))
        # Different frontiers may divide exactly the same work differently; compare plans only within a mode.
        plans_equal = all(len({json.dumps(diagnostics.stable(r['events'][1]['diagnostics']), sort_keys=True)
                              for r in found if r['optimizations'] == mode}) <= 1 for mode in (3, 7))
        equal = semantic_equal and work_equal and cache_equal and raw_equal and plans_equal
        result.append(dict(case=name, kmax=kmax, requested=len(expected), successful=[identity(r) for r in found],
                           semantic_equal=semantic_equal, geometric_work_equal=work_equal,
                           cache_work_equal=cache_equal, same_profile_bytes_equal=raw_equal,
                           same_mode_plan_equal=plans_equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete'))
    return result


def run(args):
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    builds = profiles.checked_builds(args)
    supplement = profiles.checked_supplement(args.supplement)
    manifest, manifest_hash = profiles.inputs(args.data)
    cases = {r['name']: r for r in manifest['cases']}
    requested = schedule()
    report = dict(schema=SCHEMA, attempt_schema='ehgp.v11.catalogue_attempt.v2', complete=False, conforming=False,
                  manifest=manifest, manifest_sha256=manifest_hash, builds=list(builds.values()),
                  qualification_sha256=base.digest(args.qualification), supplement_sha256=supplement,
                  requested=requested, requested_runs=len(requested), timeout_seconds=TIMEOUT,
                  native_schedule_bound_seconds=TIMEOUT*len(requested), budget_seconds=args.budget_seconds,
                  optimization_modes={'3': 'cache_and_indirect_sort_fixed_frontier',
                                      '7': 'cache_and_indirect_sort_adaptive_frontier'},
                  scope='whole integer inputs; CPU catalogue K5; no FULL/segmentation/GPU claim',
                  timing_scope='API includes requested diagnostic allocation/recording; Cloud/Pool separate; process includes IO',
                  diagnostic_scope='owned planning/tasks; disjoint non-exhaustive stage walls; task sums/max are not wall time',
                  memory_scope='Buffer reservations including live Cloud and requested diagnostics; not RSS or Python decoder',
                  comparison_scope='semantic/logical/q4/cache across all modes/W/profiles; bytes within profile; plans within mode',
                  precision='identical 1mm integer inputs in u21/u24; not finer quantization',
                  repetitions='one fresh process per mode/profile/W; mode3 then mode7; diagnostics enabled on both',
                  omission_policy='budget only; no failed mode suppresses another attempt',
                  leaf_size=16, max_leaf=256, runs=[], launch_intents=[], not_run=[], comparisons=[],
                  full_schedule_completed=False)
    path = args.out/'adaptive.json'

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
            request['repetition'], TIMEOUT, request['optimizations'], diagnostics=True))
        save()

        def checkpoint(row):
            need(len(report['runs']) == ordinal and identity(row) == identity(request), 'adaptive checkpoint')
            report['runs'].append(row); save()

        row = profiles.measure(Path(builds[request['coord_bits']]['path']), cases[request['case']],
            request['coord_bits'], request['kmax'], args, checkpoint, workers=request['workers'],
            repetition=request['repetition'], timeout=TIMEOUT, optimizations=request['optimizations'], diagnostics=True)
        check_adaptive(row, request)
        need(len(report['runs']) == ordinal+1, 'missing adaptive checkpoint')
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
    args = parser.parse_args()
    if not 60 <= args.budget_seconds <= 700:
        parser.error('campaign budget outside 60..700 seconds')
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        print('catalogue_adaptive_refused: '+type(error).__name__, flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
