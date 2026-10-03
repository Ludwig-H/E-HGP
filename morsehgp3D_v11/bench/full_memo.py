#!/usr/bin/env python3
"""Fixed versus tuple-memo FULL ablation; whole inputs and fresh native processes."""
import argparse
from pathlib import Path
import sys
import time

import full_campaign as full

base, need, profiles = full.base, full.need, full.profiles
SCHEMA = 'ehgp.v11.full_memo.v1'
VARIABLE_WORK = {'descent_steps', 'part_meb_presentations', 'part_diameter_pairs', 'trace_meb_calls',
                 'trace_meb_presentations', 'trace_diameter_pairs', 'census_point_tests'} | {
                     'memo_' + name for name in full.MEMO}
INVARIANT_WORK = full.WORK - VARIABLE_WORK


def schedule():
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    synthetic = sorted((name for name in profiles.COUNTS if not name.startswith('lidar')), key=profiles.COUNTS.get)
    return [dict(case=name,coord_bits=bits,kmax=5,workers=48,repetition=0,optimizations=mode)
            for names, formats in ((lidar,(21,24)), (synthetic,(21,)))
            for name in names for bits in formats for mode in (3,7)]


def comparisons(rows, requested):
    wanted = [full.identity(row) for row in requested]
    actual = [full.identity(row) for row in rows]
    need(len(wanted) == len(set(wanted)) and len(actual) == len(set(actual)) and set(actual) <= set(wanted),
         'FULL memo comparison inventory')
    result = []
    for name in sorted({r['case'] for r in requested}):
        expected = [r for r in requested if r['case'] == name]
        found = [r for r in rows if r['case'] == name and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        raw_equal = all(len({r['semantic']['raw_sha256'] for r in found if r['coord_bits'] == bits}) <= 1
                        for bits in (21,24))
        work_equal = len({tuple(tuple(o['work'][key] for key in sorted(INVARIANT_WORK))
                               for o in r['events'][2]['orders']) for r in found}) <= 1
        equal = semantic_equal and raw_equal and work_equal
        result.append(dict(case=name,kmax=5,requested=len(expected),successful=[full.identity(r) for r in found],
                           semantic_equal=semantic_equal,same_profile_bytes_equal=raw_equal,
                           invariant_work_equal=work_equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete'))
    return result


def run(args):
    reuse_enabled = getattr(args,'reuse_semantic',False)
    need(type(reuse_enabled) is bool,'FULL semantic reuse option')
    summary_cache = full.reuse.SummaryCache() if reuse_enabled else None
    args.out.mkdir(parents=True,exist_ok=True)
    args.work.mkdir(parents=True,exist_ok=False)
    started = time.monotonic()
    builds = profiles.checked_builds(args,executable='mhgp11_full_bench')
    supplement = profiles.checked_supplement(args.supplement)
    manifest, manifest_hash = profiles.inputs(args.data)
    cases = {r['name']:r for r in manifest['cases']}
    requested = schedule()
    report = dict(schema=SCHEMA,complete=False,conforming=False,manifest=manifest,manifest_sha256=manifest_hash,
                  qualification_sha256=base.digest(args.qualification),supplement_sha256=supplement,
                  builds=list(builds.values()),requested=requested,requested_runs=len(requested),
                  timeout_seconds=full.TIMEOUT,budget_seconds=args.budget_seconds,
                  work_schema='ehgp.v11.full_work.v4',memo_capacity=full.MEMO_CAPACITY,
                  semantic_reuse_enabled=reuse_enabled,
                  semantic_reuse_scope='every payload fully rehashed; summaries reused under SHA256 identity assumption',
                  optimization_modes={'3':'cache_J2_indirect_sort','7':'cache_J2_indirect_sort_tuple_memo'},
                  scope='CPU FULL K1..5; complete integer inputs; unit weights; fixed catalogue frontier',
                  timing_scope='index + catalogue/lookup + forests/verticals; Cloud/Pool/IO separate',
                  excluded='segmentation; input grid preparation; point hierarchy; GPU',
                  precision='same 1mm inputs in u21/u24; synthetic common u18 coordinate domain',
                  repetitions='one fresh process per case/profile/mode; mode3 then mode7',
                  comparison_scope='semantic across profiles/modes, bytes within profile, unchanged work separately',
                  invariant_work=sorted(INVARIANT_WORK),variable_work=sorted(VARIABLE_WORK),
                  memory_scope='Buffer reservations with Cloud and temporary memo; not RSS or Python',
                  omission_policy='budget only; failure does not suppress another mode',
                  leaf_size=16,max_leaf=256,runs=[],launch_intents=[],not_run=[],comparisons=[],
                  full_schedule_completed=False)
    path = args.out/'full_memo.json'

    def save():
        report['comparisons'] = comparisons(report['runs'],requested)
        base.save(path,report)

    save()
    for request in requested:
        if time.monotonic()-started+full.TIMEOUT+20 >= args.budget_seconds:
            report['not_run'].append(dict(request,reason='campaign_budget_before_launch')); save(); continue
        ordinal = len(report['runs'])
        call_args = argparse.Namespace(**vars(args)); call_args.optimizations = request['optimizations']
        report['launch_intents'].append(full.launch_intent(Path(builds[request['coord_bits']]['path']),
            cases[request['case']],request['coord_bits'],request['kmax'],call_args,request['workers'],
            request['repetition'],full.TIMEOUT,request['optimizations']))
        save()

        def checkpoint(row):
            need(len(report['runs']) == ordinal and full.identity(row) == full.identity(request), 'FULL memo checkpoint')
            report['runs'].append(row); save()

        cache_option = {'semantic_cache':summary_cache} if summary_cache is not None else {}
        row = full.measure(Path(builds[request['coord_bits']]['path']),cases[request['case']],request,call_args,checkpoint,**cache_option)
        need(len(report['runs']) == ordinal+1, 'missing FULL memo checkpoint')
        report['runs'][ordinal] = row; save()
        print('%s B%d K%d W%d r%d mode%d %s' % (*full.identity(request),row['status']),flush=True)
    report['complete'] = True
    report['full_schedule_completed'] = not report['not_run'] and len(report['runs']) == len(requested)
    report['conforming'] = report['full_schedule_completed'] and all(r['status'] == 'ok' for r in report['runs']) and all(
        c['status'] == 'equal' for c in report['comparisons'])
    report['campaign_wall_seconds'] = time.monotonic()-started
    save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('builds','data','out','work','qualification','supplement'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--reuse-semantic',action='store_true')
    parser.add_argument('--budget-seconds',type=int,default=600)
    args = parser.parse_args()
    if not 60 <= args.budget_seconds <= 700:
        parser.error('campaign budget outside 60..700 seconds')
    try:
        return run(args)
    except (OSError,ValueError,KeyError,TypeError,StopIteration) as error:
        print('full_memo_refused: '+type(error).__name__,flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
