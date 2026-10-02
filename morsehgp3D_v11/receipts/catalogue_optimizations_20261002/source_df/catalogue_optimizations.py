#!/usr/bin/env python3
"""Independent exact catalogue ablations; whole inputs, CPU only, no FULL timing claim."""
import argparse
from pathlib import Path
import sys
import time

import catalogue_parallel as parallel

profiles = parallel.profiles
base, need = profiles.base, profiles.semantic.need
SCHEMA = 'ehgp.v11.catalogue_optimizations.v1'
TIMEOUT = 15


def identity(row):
    mode = row['optimizations']
    need(type(mode) is int and 0 <= mode < 4, 'optimization identity')
    return (*parallel.identity(row), mode)


def schedule():
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    synthetic = sorted((name for name in profiles.COUNTS if not name.startswith('lidar')),
                       key=profiles.COUNTS.get)
    result = []
    for names, workers, modes in ((lidar,48,(0,2,1,3)), (lidar,8,(3,)), (synthetic,48,(3,))):
        for name in names:
            for bits in (21,24):
                for mode in modes:
                    result.append(dict(case=name,coord_bits=bits,kmax=5,workers=workers,repetition=0,
                                       optimizations=mode))
    return result


def check_optimization(row, requested):
    need(identity(row) == identity(requested), 'full optimization identity changed')
    parallel.check_parallel(row, requested)
    if row['status'] != 'ok':
        return
    try:
        need(row['stderr'] == '', 'unexpected native stderr')
        event = row['events'][1]
        mode, work = event['optimizations'], event['cache_work']
        need(type(mode) is int and mode == requested['optimizations'], 'native optimization differs')
        need(type(work) is dict and set(work) == {'evaluations','hits','fallbacks'}, 'cache work fields')
        need(all(type(v) is int and 0 <= v < 2**64 for v in work.values()), 'cache work unsigned values')
        need(event['logical']['region_line_tests'] == work['evaluations'] + work['hits'], 'cache request inventory')
        need(work['fallbacks'] <= work['evaluations'], 'cache fallback subset')
        need(mode & 1 or work['hits'] == work['fallbacks'] == 0, 'disabled cache has reused work')
        row['cache_work'] = dict(work)
    except (ValueError, KeyError, TypeError, OverflowError) as error:
        base.attempt_error(row, 'optimization', error, 'invalid_output')


def comparisons(rows, requested):
    wanted = set(map(identity, requested))
    actual = list(map(identity, rows))
    need(len(wanted) == len(requested) and len(set(actual)) == len(actual) and set(actual) <= wanted,
         'optimization comparison inventory')
    result = []
    for name, kmax in sorted({(r['case'],r['kmax']) for r in requested}):
        expected = [r for r in requested if (r['case'],r['kmax']) == (name,kmax)]
        found = [r for r in rows if (r['case'],r['kmax']) == (name,kmax) and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        work_equal = len({(profiles.work_signature(r),profiles.q4_signature(r)) for r in found}) <= 1
        raw_equal = all(len({r['canonical_sha256'] for r in found if r['coord_bits'] == bits}) <= 1
                        for bits in (21,24))
        equal = semantic_equal and work_equal and raw_equal
        result.append(dict(case=name,kmax=kmax,requested=len(expected),successful=[identity(r) for r in found],
                           baseline_successful=[identity(r) for r in found if r['optimizations'] == 0],
                           semantic_equal=semantic_equal,geometric_work_equal=work_equal,
                           same_profile_bytes_equal=raw_equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete'))
    return result


def run(args):
    args.out.mkdir(parents=True,exist_ok=True)
    args.work.mkdir(parents=True,exist_ok=False)
    started = time.monotonic()
    builds = profiles.checked_builds(args)
    supplement = profiles.checked_supplement(args.supplement)
    manifest, manifest_hash = profiles.inputs(args.data)
    cases = {row['name']: row for row in manifest['cases']}
    requested = schedule()
    report = dict(schema=SCHEMA,attempt_schema='ehgp.v11.catalogue_attempt.v2',complete=False,conforming=False,
                  manifest=manifest,manifest_sha256=manifest_hash,builds=list(builds.values()),
                  qualification_sha256=base.digest(args.qualification),supplement_sha256=supplement,
                  requested=requested,requested_runs=len(requested),timeout_seconds=TIMEOUT,
                  native_schedule_bound_seconds=TIMEOUT*len(requested),budget_seconds=args.budget_seconds,
                  optimization_modes={'0':'neither','1':'center_line_cache','2':'exact_index_sort','3':'both'},
                  scope='whole integer inputs; exact CPU catalogue K5; no FULL/segmentation/GPU claim',
                  timing_scope='catalogue two passes/sort/output; Cloud/Pool separate; process includes IO',
                  diagnostic_scope='disjoint non-exhaustive stage walls; task sums/max distinct from wall',
                  memory_scope='native Buffer reservations including live Cloud; not RSS or Python decoder',
                  cache_work_scope='one generation pass: demands=evaluations+hits; fallbacks subset of evaluations',
                  comparison_scope='exact semantic and logical/q4 work across modes/profiles/W; raw bytes within profile',
                  precision='identical 1mm integers in u21/u24; not finer input quantization',
                  repetitions='one fresh process per declared mode/profile/W; fixed mode order 0,2,1,3',
                  omission_policy='budget only; no failed mode suppresses another attempt',
                  leaf_size=16,max_leaf=256,runs=[],launch_intents=[],not_run=[],comparisons=[],
                  full_schedule_completed=False)
    path = args.out/'optimizations.json'

    def save():
        report['comparisons'] = comparisons(report['runs'],requested)
        base.save(path,report)

    save()
    for request in requested:
        if time.monotonic()-started+TIMEOUT+20 >= args.budget_seconds:
            report['not_run'].append(dict(request,reason='campaign_budget_before_launch')); save(); continue
        ordinal = len(report['runs'])
        report['launch_intents'].append(profiles.launch_intent(Path(builds[request['coord_bits']]['path']),
            cases[request['case']],request['coord_bits'],request['kmax'],args,request['workers'],
            request['repetition'],TIMEOUT,request['optimizations']))
        save()

        def checkpoint(row):
            need(len(report['runs']) == ordinal and identity(row) == identity(request), 'optimization checkpoint')
            report['runs'].append(row); save()

        row = profiles.measure(Path(builds[request['coord_bits']]['path']),cases[request['case']],
            request['coord_bits'],request['kmax'],args,checkpoint,workers=request['workers'],
            repetition=request['repetition'],timeout=TIMEOUT,optimizations=request['optimizations'])
        check_optimization(row,request)
        need(len(report['runs']) == ordinal+1, 'missing optimization checkpoint')
        report['runs'][ordinal] = row
        save()
        print('%s B%d K%d W%d r%d mode%d %s' % (*identity(request),row['status']),flush=True)
    report['complete'] = True
    report['full_schedule_completed'] = not report['not_run'] and len(report['runs']) == len(requested)
    report['conforming'] = report['full_schedule_completed'] and all(r['status'] == 'ok' for r in report['runs']) and all(
        c['status'] == 'equal' for c in report['comparisons'])
    report['campaign_wall_seconds'] = time.monotonic()-started
    save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('builds','data','out','work','qualification','supplement'):
        parser.add_argument('--'+option,type=Path,required=True)
    parser.add_argument('--budget-seconds',type=int,default=700)
    args = parser.parse_args()
    if not 60 <= args.budget_seconds <= 700:
        parser.error('campaign budget outside 60..700 seconds')
    try:
        return run(args)
    except (OSError,ValueError,KeyError,TypeError,StopIteration) as error:
        print('catalogue_optimizations_refused: '+type(error).__name__,flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
