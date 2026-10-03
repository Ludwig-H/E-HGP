#!/usr/bin/env python3
"""Serial versus regular-cell lanes: whole FULL inputs, ordered DSU publication."""
import argparse
from pathlib import Path
import sys
import time

import full_campaign as full

base, need, profiles = full.base, full.need, full.profiles
SCHEMA = 'ehgp.v11.full_parallel_campaign.v7'
DESCENT_WORK_MASK = 15 | 128 | 256 | 1024
REUSE_VARIABLE_WORK = {'vertical_descents', 'vertical_reuses', 'ancestor_find_steps'}
VARIABLE_WORK = {'descent_steps', 'part_meb_presentations', 'part_diameter_pairs', 'trace_meb_calls',
                 'trace_meb_presentations', 'trace_diameter_pairs', 'census_point_tests'} | {
                     'memo_' + name for name in full.MEMO} | REUSE_VARIABLE_WORK
INVARIANT_WORK = full.WORK - VARIABLE_WORK


def schedule(optimized_catalogue=False, parallel_verticals=False, reuse_census=False, dense_births=False,
             reuse_verticals=False):
    calendars = (optimized_catalogue, parallel_verticals, reuse_census, dense_births, reuse_verticals)
    need(all(type(v) is bool for v in calendars), 'FULL campaign options')
    need(sum(calendars) <= 1, 'exclusive FULL campaign calendars')
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    synthetic = sorted((name for name in profiles.COUNTS if not name.startswith('lidar')), key=profiles.COUNTS.get)
    def request(name, bits, mode, workers=48):
        return dict(case=name,coord_bits=bits,kmax=5,workers=workers,repetition=0,optimizations=mode)
    if parallel_verticals or reuse_census or dense_births or reuse_verticals:
        modes = (511,1023,2047) if reuse_verticals else (511,1023) if dense_births else (127,255,511) if reuse_census else (127,255)
        rows = [request(name,bits,mode) for name in lidar for bits in (21,24) for mode in modes]
        rows += [request(lidar[0],21,modes[-1],workers) for workers in (1,8)]
        return rows + [request(name,21,mode) for name in synthetic for mode in modes]
    if optimized_catalogue:
        return [request(name,bits,mode) for names, profiles_ in ((lidar,(21,24)),(synthetic,(21,)))
                for name in names for bits in profiles_ for mode in (15,63,127)]
    rows = [request(name,bits,mode) for name in lidar for bits in (21,24) for mode in (7,15)]
    rows += [request(lidar[0],21,15,workers) for workers in (1,8)]
    rows += [request(lidar[0],21,mode) for mode in (3,11)]
    rows += [request(name,21,15) for name in synthetic]
    return rows


def comparisons(rows, requested):
    wanted = [full.identity(row) for row in requested]
    actual = [full.identity(row) for row in rows]
    need(len(wanted) == len(set(wanted)) and len(actual) == len(set(actual)) and set(actual) <= set(wanted),
         'FULL parallel comparison inventory')
    result = []
    for name in sorted({r['case'] for r in requested}):
        expected = [r for r in requested if r['case'] == name]
        found = [r for r in rows if r['case'] == name and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        raw_equal = all(len({r['semantic']['raw_sha256'] for r in found if r['coord_bits'] == bits}) <= 1
                        for bits in (21,24))
        work_equal = len({tuple(tuple(o['work'][key] for key in sorted(INVARIANT_WORK))
                               for o in r['events'][2]['orders']) for r in found}) <= 1
        reference_work_equal = len({tuple(tuple(o['work'][key] for key in sorted(REUSE_VARIABLE_WORK))
                                         for o in r['events'][2]['orders'])
                                    for r in found if not r['optimizations'] & 1024}) <= 1
        reuse_counts_equal = all(len({tuple((o['work']['vertical_descents'], o['work']['vertical_reuses'])
                                           for o in r['events'][2]['orders'])
                                      for r in found if bool(r['optimizations'] & 1024) == active}) <= 1
                                 for active in (False, True))
        lane_work_equal = all(len({tuple(tuple(sorted(o['work'].items())) for o in r['events'][2]['orders'])
                                   for r in found if (r['optimizations'] & DESCENT_WORK_MASK) == mode}) <= 1
                              for mode in {r['optimizations'] & DESCENT_WORK_MASK for r in expected})
        lane_counts_equal = len({tuple(tuple(o['parallel'][key] for key in sorted(full.parallel.COUNTS))
                                      for o in r['events'][2]['orders'])
                                 for r in found if r['optimizations'] & 8}) <= 1
        vertical_counts_equal = all(len({tuple(tuple(o['vertical_parallel'][key] for key in sorted(full.vertical.COUNTS))
                                               for o in r['events'][2]['orders'])
                                          for r in found if r['optimizations'] & 128 and
                                          bool(r['optimizations'] & 1024) == active}) <= 1 for active in (False, True))
        census = full.workspace.comparisons(found, full.WORK, need)
        equal = (semantic_equal and raw_equal and work_equal and reference_work_equal and reuse_counts_equal and lane_work_equal and lane_counts_equal
                 and vertical_counts_equal and census['other_work_equal'] and census['point_tests_equal'])
        result.append(dict(case=name,kmax=5,requested=len(expected),successful=[full.identity(r) for r in found],
                           semantic_equal=semantic_equal,same_profile_bytes_equal=raw_equal,
                           invariant_work_equal=work_equal,lane_work_equal=lane_work_equal,lane_counts_equal=lane_counts_equal,
                           reference_work_equal=reference_work_equal,reuse_counts_equal=reuse_counts_equal,
                           vertical_counts_equal=vertical_counts_equal,census_workspace=census,
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
    optimized_catalogue = getattr(args,'optimized_catalogue',False)
    parallel_verticals = getattr(args,'parallel_verticals',False)
    reuse_census = getattr(args,'reuse_census',False)
    dense_births = getattr(args,'dense_births',False)
    reuse_verticals = getattr(args,'reuse_verticals',False)
    requested = schedule(optimized_catalogue,parallel_verticals,reuse_census,dense_births,reuse_verticals)
    modes = {'3':'cache_J2_indirect_sort','7':'cache_J2_indirect_sort_tuple_memo',
             '11':'cache_J2_indirect_sort_regular_lanes','15':'cache_J2_indirect_sort_regular_lanes_private_memo',
             '63':'mode15_adaptive_parallel_assembly','127':'mode63_single_pass','255':'mode127_parallel_verticals','511':'mode255_reused_census_workspace','1023':'mode511_dense_birth_lookup',
             '2047':'mode1023_reused_regular_vertical_seeds'}
    report = dict(schema=SCHEMA,optimized_catalogue=optimized_catalogue,parallel_verticals=parallel_verticals,
                  reuse_census=reuse_census,dense_births=dense_births,reuse_verticals=reuse_verticals,
                  complete=False,conforming=False,manifest=manifest,manifest_sha256=manifest_hash,
                  qualification_sha256=base.digest(args.qualification),supplement_sha256=supplement,
                  builds=list(builds.values()),requested=requested,requested_runs=len(requested),
                  timeout_seconds=full.TIMEOUT,budget_seconds=args.budget_seconds,
                  work_schema='ehgp.v11.full_work.v5',parallel_schema='ehgp.v11.full_parallel.v1',vertical_schema=full.vertical.SCHEMA,census_workspace_schema=full.workspace.SCHEMA,
                  census_comparison_schema=full.workspace.COMPARISON_SCHEMA,dense_lookup_schema=full.dense.SCHEMA,
                  regular_vertical_schema=full.regular.SCHEMA,
                  census_comparison_mask=full.workspace.PAIRED_WORK_MASK,
                  memo_capacity=full.MEMO_CAPACITY,regular_batch_capacity=4096,descent_lanes=48,lane_memo_capacity=4096,
                  semantic_reuse_enabled=reuse_enabled,
                  semantic_reuse_scope='every payload fully rehashed; summaries reused under SHA256 identity assumption',
                  optimization_modes={str(mode):modes[str(mode)] for mode in sorted({r['optimizations'] for r in requested})},
                  catalogue_execution_schema='ehgp.v11.catalogue_execution.v1',
                  scope='CPU FULL K1..5; complete integer inputs; unit weights; '+
                        ('strict regular vertical seed reuse compared' if reuse_verticals else
                         'sparse versus dense birth lookup compared' if dense_births else
                         'parallel verticals and physical census workspaces compared' if reuse_census else
                         'parallel verticals compared' if parallel_verticals else
                         'catalogue options compared' if optimized_catalogue else 'fixed catalogue frontier'),
                  timing_scope='index + catalogue/lookup + forests/verticals; Cloud/Pool/IO separate',
                  excluded='segmentation; input grid preparation; point hierarchy; GPU',
                  precision='same 1mm inputs in u21/u24; synthetic common u18 coordinate domain',
                  repetitions='one fresh process per case/profile/mode/worker; '+
                              ('ordered511 then1023 then2047 W48, then mode2047 W1/W8 on ng00u21' if reuse_verticals else
                               'paired511 then1023 W48, then mode1023 W1/W8 on ng00u21' if dense_births else
                               'ordered127 then255 then511 W48, then mode511 W1/W8 on ng00u21' if reuse_census else
                               'paired127 then255 W48, then mode255 W1/W8 on ng00u21' if parallel_verticals else
                               'modes15/63/127 W48, no repetition' if optimized_catalogue else
                               'paired7 then15, then W1/W8 and3/11 on ng00u21'),
                  comparison_scope='semantic across profiles/modes/workers, bytes within profile; paid work at fixed descent mode, including across catalogue options; structural work across descent modes',
                  invariant_work=sorted(INVARIANT_WORK),variable_work=sorted(VARIABLE_WORK),
                  descent_work_mask=DESCENT_WORK_MASK,
                  memory_scope='Buffer reservations with Cloud, catalogue arena, serial memo and all lane tables; not RSS or Python',
                  omission_policy='budget only; failure does not suppress another mode',
                  leaf_size=16,max_leaf=256,runs=[],launch_intents=[],not_run=[],comparisons=[],
                  full_schedule_completed=False)
    path = args.out/'full_parallel.json'

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
            need(len(report['runs']) == ordinal and full.identity(row) == full.identity(request), 'FULL parallel checkpoint')
            report['runs'].append(row); save()

        cache_option = {'semantic_cache':summary_cache} if summary_cache is not None else {}
        row = full.measure(Path(builds[request['coord_bits']]['path']),cases[request['case']],request,call_args,checkpoint,**cache_option)
        need(len(report['runs']) == ordinal+1, 'missing FULL parallel checkpoint')
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
    calendars = parser.add_mutually_exclusive_group()
    calendars.add_argument('--optimized-catalogue',action='store_true')
    calendars.add_argument('--parallel-verticals',action='store_true')
    calendars.add_argument('--reuse-census',action='store_true')
    calendars.add_argument('--dense-births',action='store_true')
    calendars.add_argument('--reuse-verticals',action='store_true')
    parser.add_argument('--budget-seconds',type=int,default=600)
    args = parser.parse_args()
    if not 60 <= args.budget_seconds <= 700:
        parser.error('campaign budget outside 60..700 seconds')
    try:
        return run(args)
    except (OSError,ValueError,KeyError,TypeError,StopIteration) as error:
        print('full_parallel_refused: '+type(error).__name__,flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
