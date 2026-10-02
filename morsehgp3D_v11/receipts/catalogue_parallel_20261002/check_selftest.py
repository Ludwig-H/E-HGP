"""Countertests of parallel5 evidence; synthetic edits only, no native/cloud execution."""
import copy
import json
from pathlib import Path

import check as reader

HERE = Path(__file__).resolve().parent


def main():
    folder = HERE/'parallel5'
    receipt,worker,data = reader.old.read_capture(folder)
    manifest,mhash = reader.old.inputs(folder,receipt)
    _,builds,_ = reader.q4.judge_matrix(data)
    report = reader.js(data[reader.BENCH]); positives,corruptions = 0,0
    def judge(row):
        return reader.benchmark(row,manifest,mhash,reader.sha(data[reader.BASE+'summary.json']),
                                reader.sha(data[reader.SUPPLEMENT+'summary.json']),builds,data)
    def compare(row):
        row['comparisons'] = reader.js(json.dumps(reader.driver.comparisons(row['runs'],row['requested'])))
    judge(report); positives += 1
    # A valid incomplete checkpoint after one native success.
    partial = copy.deepcopy(report); partial.update(complete=False,conforming=False,full_schedule_completed=False)
    partial['runs'] = partial['runs'][:1]; partial['launch_intents'] = partial['launch_intents'][:1]
    compare(partial); judge(partial); positives += 1
    # Valid K5 first-attempt failure causes only later identical case/profile omissions.
    failed = copy.deepcopy(report); first = failed['runs'][0]
    template = copy.deepcopy(failed['runs'][-1])
    for key in ('case','coord_bits','kmax','workers','repetition','argv','count'):
        template[key] = copy.deepcopy(first[key])
    failed['runs'][0] = template
    failed['not_run'] = [dict(r,reason='same_profile_K5_W48_first_attempt_failed') for r in failed['requested']
                         if r['case']==first['case'] and r['coord_bits']==first['coord_bits'] and
                         reader.driver.identity(r)!=reader.driver.identity(first)]
    remove = {reader.driver.identity(r) for r in failed['not_run']}
    failed['runs'] = [r for r in failed['runs'] if reader.driver.identity(r) not in remove]
    failed['launch_intents'] = [r for r in failed['launch_intents'] if reader.driver.identity(r) not in remove]
    failed['full_schedule_completed'] = False; compare(failed); judge(failed); positives += 1
    budget = copy.deepcopy(report); budget.update(runs=[],launch_intents=[],full_schedule_completed=False)
    budget['not_run'] = [dict(r,reason='campaign_budget_before_launch') for r in budget['requested']]
    compare(budget); judge(budget); positives += 1
    def reject(row,mutation):
        nonlocal corruptions
        bad = copy.deepcopy(row); mutation(bad)
        try: judge(bad)
        except (reader.old.foundation.Refusal,ValueError,KeyError,TypeError,IndexError):
            corruptions += 1; return
        raise ValueError('undetected mutation')
    changes = (
        lambda r:r.__setitem__('conforming',True),
        lambda r:r.__setitem__('complete',False),
        lambda r:r.__setitem__('requested_runs',35),
        lambda r:r.__setitem__('timeout_seconds',30),
        lambda r:r.__setitem__('leaf_size',32),
        lambda r:r.__setitem__('budget_seconds',700),
        lambda r:r.__setitem__('manifest_sha256','0'*64),
        lambda r:r.__setitem__('qualification_sha256','0'*64),
        lambda r:r.__setitem__('supplement_sha256','0'*64),
        lambda r:r['builds'][1].__setitem__('sha256','0'*64),
        lambda r:r['builds'][1].__setitem__('cache_sha256','0'*64),
        lambda r:r['runs'][0].__setitem__('workers',8),
        lambda r:r['runs'][0]['argv'].__setitem__(-1,'8'),
        lambda r:r['runs'][0].__setitem__('count',1),
        lambda r:r['runs'][0].__setitem__('timeout_seconds',30),
        lambda r:r['runs'][0].__setitem__('pool_ms',-1),
        lambda r:r['runs'][0].__setitem__('catalogue_within_200ms',True),
        lambda r:r['runs'][0]['stage_ms'].__setitem__('sort',-1),
        lambda r:r['runs'][0]['semantic'].__setitem__('sha256','0'*64),
        lambda r:r['runs'][0].__setitem__('canonical_sha256','0'*64),
        lambda r:r['runs'][0]['qmin_counts'].__setitem__('4',0),
        lambda r:r['runs'][0]['events'][1]['logical'].__setitem__('prefixes',0),
        lambda r:r['runs'][0]['events'][1]['timings'].__setitem__('count_ns',0),
        lambda r:r['runs'][0]['events'][1].__setitem__('workers',8),
        lambda r:r['runs'][0].__setitem__('stdout',r['runs'][0]['stdout']+'not json\n'),
        lambda r:r['runs'][-1].__setitem__('status','ok'),
        lambda r:r['runs'][-1].__setitem__('exit_code',0),
        lambda r:r['runs'][-1].__setitem__('errors',[]),
        lambda r:r['runs'].pop(),
        lambda r:r['runs'].append(copy.deepcopy(r['runs'][0])),
        lambda r:r['launch_intents'].pop(),
        lambda r:r['launch_intents'][0].__setitem__('input_sha256','0'*64),
        lambda r:r['comparisons'][0].__setitem__('status','equal' if r['comparisons'][0]['status']!='equal' else 'incomplete'),
    )
    for mutation in changes: reject(report,mutation)
    reject(failed,lambda r:r['not_run'][0].__setitem__('reason','campaign_budget_before_launch'))
    reject(budget,lambda r:r['not_run'][0].__setitem__('reason','same_profile_K5_W48_first_attempt_failed'))
    reject(partial,lambda r:r.__setitem__('complete',True))
    reader.need((positives,corruptions)==(4,36),'selftest nonvacuity')
    print('parallel_reader_selftest positives4 corruptions36 native0')


if __name__ == '__main__': main()
