"""Synthetic startup-only receipt controls; no cloud/native execution or historical receipt changes."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile

import check as reader

stockout, api = reader.stockout, reader.old


def fixture():
    old = '2026-10-02T10:00:00+00:00'
    current = '2026-10-02T11:00:00+00:00'
    end, observed = '2026-10-02T11:00:10+00:00', '2026-10-02T11:10:00+00:00'
    target = dict(project='fixture', zone='zone-a', instance='vm')
    raw = dict(schema='ehgp.v11.session_receipt.v1', source_kind='commit', evidence_grade='pushed_commit',
               commit=stockout.SOURCE, worker_source='commit:' + stockout.SOURCE,
               worker_plan_sha256='1'*64, plan_sha256='2'*64, package_sha256='3'*64,
               status='shutdown_uncertified', closure='generation_unknown', generation=None,
               pre_start_generation=old, start_certified=False, start_may_have_been_requested=True,
               gcp_mutation_phase_entered=True, targeted_shutdown_certified=False, worker_launched=False,
               worker_exit_code=None, worker_outcome=None, results_verified=False, private_key_deleted=True,
               oslogin_key_removed=True, reserve_released=True, target=target,
               host_commands=[dict(name='guarded_start', exit_code=1)],
               observed_after=dict(name='vm', status='TERMINATED', lastStartTimestamp=old, lastStopTimestamp=end))
    op = dict(name='operation-1', operationType='start', status='DONE', insertTime=current, endTime=end,
              targetLink='https://www.googleapis.com/compute/v1/projects/fixture/zones/zone-a/instances/vm',
              error=dict(errors=[dict(code='ZONE_RESOURCE_POOL_EXHAUSTED_WITH_DETAILS')]))
    external = dict(schema='ehgp.v11.external_closure.v1', session_status_unchanged='shutdown_uncertified',
                    conclusion='target_terminated_no_new_generation_after_failed_start', controller_recovery_code=74,
                    reads=[dict(argv=['gcloud', 'compute', 'instances', 'describe', 'vm', '--project=fixture',
                                      '--zone=zone-a'], observed_at=observed, response=copy.deepcopy(raw['observed_after'])),
                           dict(argv=['gcloud', 'compute', 'operations', 'list', '--project=fixture', '--zones=zone-a',
                                      '--filter=targetLink~vm AND operationType=start'],
                                observed_at=observed, response=[op])])
    return raw, external


def main():
    raw, external = fixture()
    good, closure = stockout.views(raw, external, api)
    api.need(good['status'] == 'shutdown_uncertified' and closure['pending_start_count'] == 0, 'positive')
    modes = ('commit', 'worker', 'results', 'cleanup', 'controller_green', 'generation', 'start_success',
             'pending_start', 'error', 'target', 'observed_running', 'observed_generation', 'duplicate_operation',
             'empty_operations', 'omitted_read', 'date', 'filtered_pending', 'query_limit', 'boolean_recovery')
    count = 0
    for mode in modes:
        r, e = copy.deepcopy(raw), copy.deepcopy(external)
        op, observation = e['reads'][1]['response'][0], e['reads'][0]['response']
        if mode == 'commit': r['commit'] = '0'*40
        elif mode == 'worker': r['worker_launched'] = True
        elif mode == 'results': r['results_verified'] = True
        elif mode == 'cleanup': r['private_key_deleted'] = False
        elif mode == 'controller_green': r['closure'] = 'stopped'
        elif mode == 'generation': r['generation'] = r['pre_start_generation']
        elif mode == 'start_success': r['host_commands'][0]['exit_code'] = 0
        elif mode == 'pending_start': op['status'] = 'RUNNING'
        elif mode == 'error': op['error']['errors'][0]['code'] = 'OTHER'
        elif mode == 'target': op['targetLink'] += '-other'
        elif mode == 'observed_running': observation['status'] = 'RUNNING'
        elif mode == 'observed_generation': observation['lastStartTimestamp'] = op['insertTime']
        elif mode == 'duplicate_operation': e['reads'][1]['response'].append(copy.deepcopy(op))
        elif mode == 'empty_operations': e['reads'][1]['response'].clear()
        elif mode == 'omitted_read': e['reads'].pop()
        elif mode == 'date': op['insertTime'] = '2026-10-01T11:00:00+00:00'
        elif mode == 'filtered_pending': e['reads'][1]['argv'][-1] += ' AND status=DONE'
        elif mode == 'query_limit': e['reads'][1]['argv'].append('--limit=1')
        elif mode == 'boolean_recovery': e['controller_recovery_code'] = True
        try:
            stockout.views(r, e, api)
        except (api.foundation.Refusal, ValueError, KeyError, TypeError, IndexError):
            count += 1
        else:
            raise ValueError('startup corruption accepted ' + mode)
    with tempfile.TemporaryDirectory(prefix='mhgp11-startup-reader-') as temp:
        root = Path(temp)
        source, capture = root/'v11.20261002.meb2', root/'meb2'
        source.mkdir(); capture.mkdir(); (source/'results').mkdir()
        original = source/'receipt.json'; observed = source/'external_closure.json'
        original.write_bytes(stockout.encoded(raw)); observed.write_bytes(stockout.encoded(external))
        (source/'DONE').write_text('74\n')
        receipt = dict(good, raw_receipt_local=str(original), external_closure_local=str(observed),
                       original_receipt_sha256=api.sha(original.read_bytes()),
                       original_external_closure_sha256=api.sha(observed.read_bytes()))

        def write():
            (capture/'receipt.json').write_bytes(stockout.encoded(receipt))
            (capture/'external_closure.json').write_bytes(stockout.encoded(closure))

        write()
        with contextlib.redirect_stdout(io.StringIO()):
            api.need(stockout.check(capture, api) is True, 'positive must remain a failed campaign')
        for mode in ('hash_receipt', 'hash_closure', 'compact_boolean', 'closure_boolean', 'archive', 'done', 'missing'):
            write()
            if mode == 'hash_receipt': original.write_bytes(b'{}')
            if mode == 'hash_closure': observed.write_bytes(b'{}')
            if mode == 'compact_boolean':
                x = dict(receipt, worker_launched=0); (capture/'receipt.json').write_bytes(stockout.encoded(x))
            if mode == 'closure_boolean':
                x = dict(closure, pending_start_count=False); (capture/'external_closure.json').write_bytes(stockout.encoded(x))
            if mode == 'archive': (capture/'results.tar.gz').write_bytes(b'')
            if mode == 'done': (source/'DONE').write_text('0\n')
            if mode == 'missing': observed.unlink()
            try:
                stockout.check(capture, api)
            except (api.foundation.Refusal, OSError, ValueError, KeyError, TypeError, IndexError):
                count += 1
            else:
                raise ValueError('transport corruption accepted ' + mode)
            original.write_bytes(stockout.encoded(raw)); observed.write_bytes(stockout.encoded(external))
            (source/'DONE').write_text('74\n'); (capture/'results.tar.gz').unlink(missing_ok=True)
    api.need(count == 26, 'startup corruption floor')
    print('stockout_reader_test conforms positives2 corruptions26 native0 cloud0')


if __name__ == '__main__':
    main()
