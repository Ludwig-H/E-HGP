"""Pure controls of a preserved test failure; no native executable or cloud call."""
import copy
import json
from pathlib import Path
import tempfile

import check as c


def encode(value): return json.dumps(value,sort_keys=True,allow_nan=False).encode()


def main():
    pins = c.source_contract(); receipt,worker,data = c.transport(c.HERE,pins)
    positives,rejected = 0,0
    def good(action):
        nonlocal positives
        action(); positives += 1
    def bad(action):
        nonlocal rejected
        try: action()
        except c.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError):
            rejected += 1; return
        raise ValueError('corruption accepted')
    good(c.check)
    declarations = {r['name']:r for r in pins['matrix']['configurations']}
    configs = {r['name']:r for r in c.js(data[c.BASE+'summary.json'])['configurations']}
    def config(name,edit=lambda _r:None,alter=lambda _d,_p:None):
        value,items = copy.deepcopy(configs[name]),data.copy(); edit(value); prefix = c.BASE+name+'/'
        items[prefix+'result.json'] = encode(value); alter(items,prefix)
        return c.gate_failure(value,items,declarations[name])
    good(lambda:config('gcc_release')); good(lambda:config('mutants'))
    for key,value in [('status','ok'),('status','build_failed'),('conforming',True),('failures',[]),('not_run',[{'test':'missing'}])]:
        bad(lambda key=key,value=value:config('gcc_release',lambda r:r.__setitem__(key,value)))
    for step,key,value in [(1,'exit_code',2),(3,'exit_code',0),(1,'status','failed'),(1,'timed_out',True)]:
        bad(lambda step=step,key=key,value=value:config('gcc_release',lambda r:r['steps'][step].__setitem__(key,value)))
    bad(lambda:config('gcc_release',lambda r:r['tests'].__setitem__('failed',0)))
    bad(lambda:config('gcc_release',lambda r:r['failures'].append(r['failures'][0])))
    bad(lambda:config('gcc_release',lambda r:r.__setitem__('threads',1)))
    def replacement(path,before,after):
        def change(items,prefix):
            blob = items[prefix+path]; altered = blob.replace(before,after)
            if blob==altered: raise RuntimeError('mutation did not apply')
            items[prefix+path] = altered
        return change
    for before,after in [(b'census_reuse_test.cpp:35:',b'census_reuse_test.cpp:36:'),
                          (b'saturated &gt; 0',b'saturated &gt; 1'),
                          (b'controles=1338 echecs=1 plancher=500',b'controles=1338 echecs=0 plancher=500'),
                          (b'run_expect_verdict code',b'run_expect_verdict lancement_impossible')]:
        bad(lambda before=before,after=after:config('gcc_release',alter=replacement('junit.xml',before,after)))
    bad(lambda:config('gcc_release',alter=lambda d,p:d.__setitem__(p+'build.log',d[p+'build.log']+b'error: compiler failure\n')))
    for before,after in [(b'TEMOIN ROUGE module=tower : aucun mutant juge',b'TUE all'),
                          (b'echec sans mutation',b'mutant en echec'),
                          (b'run_expect_verdict code',b'run_expect_verdict conforme')]:
        bad(lambda before=before,after=after:config('mutants',alter=replacement('junit.xml',before,after)))
    def matrix(path,edit):
        changed = data.copy(); value = c.js(changed[path]); edit(value); changed[path] = encode(value)
        return c.matrices(changed,pins)
    good(lambda:c.matrices(data,pins))
    for key,value in [('conforming',True),('exit_code',0),('complete',False),('signals',[15]),('budget_seconds',700)]:
        bad(lambda key=key,value=value:matrix(c.BASE+'summary.json',lambda r:r.__setitem__(key,value)))
    bad(lambda:matrix(c.BASE+'summary.json',lambda r:r['configurations'].pop()))
    bad(lambda:matrix(c.SUPP+'summary.json',lambda r:r.__setitem__('conforming',True)))
    bad(lambda:matrix(c.SUPP+'summary.json',lambda r:r['configurations'][0]['tests'].__setitem__('passed',200)))
    good(lambda:c.commands(receipt,worker,data,pins))
    for path,value in [('stdout',b'conforme\n'),('stderr',b'error'),('files/parallel_full.json',b'unexpected')]:
        changed = data.copy(); changed['results/cmd/002_vertical_full/'+path] = value
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    for key,value in [('commands_ok','3'),('status','completed'),('interrupted','1')]:
        changed = dict(worker,**{key:value}); bad(lambda changed=changed:c.commands(receipt,changed,data,pins))
    for before,after in [(b'group_closed=1',b'group_closed=0'),(b'residual_group_killed=0',b'residual_group_killed=1'),
                          (b'streams_truncated=0',b'streams_truncated=1'),(b'exit_code=2',b'exit_code=0')]:
        changed = data.copy(); path = 'results/cmd/002_vertical_full/meta.txt'
        changed[path] = changed[path].replace(before,after)
        if changed[path]==data[path]: raise RuntimeError('meta mutation did not apply')
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    good(lambda:c.other_mutants(data,pins))
    original = data[c.BASE+'mutants/junit.xml']
    # A successful mutation campaign cannot be replaced by a skipped or failed gate.
    changed = data.copy(); changed[c.BASE+'mutants/junit.xml'] = original.replace(b'name="mhgp11_mutants_core"',b'name="missing_core"',1)
    bad(lambda:c.other_mutants(changed,pins))
    for kind in ('raw_field','cleanup','private_key','reserve','guest_guard','overflow','retrieval','stop_command','key_command','source','generation','target','start','compact','manifest','archive_hash'):
        with tempfile.TemporaryDirectory(prefix='vertical-failure-reader-') as tmp:
            folder = Path(tmp); compact = copy.deepcopy(receipt); raw = c.js(Path(receipt['raw_receipt_local']).read_bytes())
            for name in ('results.tar.gz','matrix.json','asan18.json','inputs.json'):
                (folder/name).symlink_to(c.HERE/name)
            if kind=='raw_field': raw.pop('targeted_shutdown_certified')
            elif kind=='cleanup': raw['oslogin_key_removed']=False; compact['oslogin_key_removed']=False
            elif kind=='private_key': raw['private_key_deleted']=False; compact['private_key_deleted']=False
            elif kind=='reserve': raw['reserve_released']=False; compact['reserve_released']=False
            elif kind=='guest_guard': raw['guest_guard_intact']=False
            elif kind=='overflow': raw['overflow']['evicted']=['lost']
            elif kind=='retrieval': raw['retrieval']='missing'
            elif kind in ('stop_command','key_command'):
                name='guarded_stop' if kind=='stop_command' else 'oslogin_remove'
                next(r for r in raw['host_commands'] if r['name']==name)['exit_code']=1
            elif kind=='source': raw['commit']='0'*40; compact['commit']='0'*40
            elif kind=='target':
                raw['target']['zone']='us-central1-b'; compact['target']=copy.deepcopy(raw['target'])
            elif kind=='start': raw['start_certified']=False; compact['start_certified']=False
            elif kind=='generation': raw['closing_generation']='wrong'; compact['closing_generation']='wrong'
            elif kind=='compact': (folder/'matrix.json').unlink(); (folder/'matrix.json').write_text('{}\n')
            elif kind=='manifest':
                raw['data_files'][0]['sha256']='0'*64; compact['data_files']=copy.deepcopy(raw['data_files'])
            else: raw['results_sha256']='0'*64; compact['results_sha256']='0'*64
            blob = encode(raw); (folder/'raw.json').write_bytes(blob); (folder/'DONE').write_text('3\n')
            compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=c.sha(blob))
            (folder/'receipt.json').write_bytes(encode(compact)); bad(lambda:c.transport(folder,pins))
    good(lambda:c.preflight_and_first(c.HERE,pins))
    # Re-pin modified auxiliary bytes, so these tests reach semantic guards,
    # not merely the checksum guard. The source plan remains fixed.
    mutations = [
        ('source',lambda r:r.__setitem__('commit','0'*40)),
        ('cloud',lambda r:r.__setitem__('gcp_mutations','performed')),
        ('plan',lambda r:r.__setitem__('plan_sha256','0'*64)),
        ('warning',lambda r:r.__setitem__('warnings',[])),
        ('oversubscribed',lambda r:r['budget'].__setitem__('oversubscribed',False)),
        ('setup',lambda r:r['budget'].__setitem__('build_and_setup_seconds',0)),
        ('window',lambda r:r['budget'].__setitem__('worker_window_seconds',1800)),
        ('commands',lambda r:r['budget'].__setitem__('command_timeouts_sum_seconds',1600)),
        ('timeout',lambda r:r['plan']['commands'][2].__setitem__('timeout_seconds',570)),
    ]
    for name,edit in mutations:
        with tempfile.TemporaryDirectory(prefix='vertical-preflight-reader-') as tmp:
            folder=Path(tmp); p=copy.deepcopy(pins)
            for filename in ('first_failure.txt','future_plan_95d.json'):
                (folder/filename).symlink_to(c.HERE/filename)
            value=c.js((c.HERE/'preflight.json').read_bytes()); edit(value)
            blob=encode(value); (folder/'preflight.json').write_bytes(blob)
            p['preflight.json']=dict(bytes=len(blob),sha256=c.sha(blob),raw_path=str(folder/'preflight.json'))
            bad(lambda:c.preflight_and_first(folder,p))
    # Copies of the historical readers must be verified before any import.
    with tempfile.TemporaryDirectory(prefix='vertical-reader-import-') as tmp:
        folder=Path(tmp)
        for filename in ('check.py','source_contract.json'):
            (folder/filename).write_bytes((c.HERE/filename).read_bytes())
        for path in pins['helpers']:
            destination=folder/'reader_sources'/path; destination.parent.mkdir(parents=True,exist_ok=True)
            destination.write_bytes((c.HERE/'reader_sources'/path).read_bytes())
        first=folder/'reader_sources'/pins['helpers'][0]
        first.write_bytes(first.read_bytes()+b'\n# corrupt frozen helper\n')
        bad(lambda:c.load('bad_frozen_vertical_reader',folder/'check.py'))
    print(json.dumps(dict(positives=positives,corruptions_rejected=rejected,native_calls=0),sort_keys=True))


if __name__=='__main__': main()
