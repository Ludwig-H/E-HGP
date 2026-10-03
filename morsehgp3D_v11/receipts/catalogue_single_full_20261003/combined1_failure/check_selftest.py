"""Corrupt the failed evidence in memory; no native process or cloud call."""
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
    for key,value in [('status','ok'),('status','failed'),('conforming',True),('failures',[]),('not_run',[{'test':'missing'}])]:
        bad(lambda key=key,value=value:config('gcc_release',lambda r:r.__setitem__(key,value)))
    for step,key,value in [(1,'exit_code',0),(3,'exit_code',0),(1,'status','ok'),(1,'timed_out',True)]:
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
    for before,after in [(b'full_probe.cpp:142:58:',b'full_probe.cpp:143:58:'),
                          (b"no member named 'incidences'",b"no member named 'balls'"),
                          (b'error:',b'warning:')]:
        bad(lambda before=before,after=after:config('gcc_release',alter=replacement('build.log',before,after)))
    for before,after in [(b'FileNotFoundError:',b'RuntimeError:'),
                          (b"/build/mhgp11_full_bench'",b"/build/mhgp11_catalogue_bench'"),
                          (b'ValueError: disjoint boundary admitted',b'ValueError: different'),
                          (b"sum(row['catalogue_stage_ms'].values()) == row['stage_ms']['domain']",b'wrong()'),
                          (b'full_catalogue_collector_test.py&quot;, line 131',b'full_catalogue_collector_test.py&quot;, line 132'),
                          (b'run_expect_verdict code',b'run_expect_verdict conforme')]:
        # XML writes literal quotes rather than entities inside text.
        before,after = before.replace(b'&quot;',b'"'),after.replace(b'&quot;',b'"')
        bad(lambda before=before,after=after:config('gcc_release',alter=replacement('junit.xml',before,after)))
    for before,after in [(b'TEMOIN ROUGE module=tower : aucun mutant juge',b'TUE all'),
                          (b'temoin [] : construction en echec',b'temoin [] : porte en echec'),
                          (b'run_expect_verdict code',b'run_expect_verdict conforme')]:
        bad(lambda before=before,after=after:config('mutants',alter=replacement('junit.xml',before,after)))
    bad(lambda:config('gcc_release',alter=lambda d,p:d.__setitem__(p+'build.log',d[p+'build.log']+b'error: extra\n')))
    def provenance_edit(items,prefix):
        value = c.js(items[prefix+'build_provenance.json'])
        value['files'].append(dict(path='mhgp11_full_bench',size=0,sha256=c.sha(b'')))
        items[prefix+'build_provenance.json'] = encode(value)
    bad(lambda:config('gcc_release',alter=provenance_edit))
    def matrix(path,edit):
        changed = data.copy(); value = c.js(changed[path]); edit(value); changed[path] = encode(value)
        return c.matrices(changed,pins)
    good(lambda:c.matrices(data,pins))
    for key,value in [('conforming',True),('exit_code',0),('complete',False),('signals',[15]),('budget_seconds',700)]:
        bad(lambda key=key,value=value:matrix(c.BASE+'summary.json',lambda r:r.__setitem__(key,value)))
    bad(lambda:matrix(c.BASE+'summary.json',lambda r:r['configurations'].pop()))
    bad(lambda:matrix(c.SUPP+'summary.json',lambda r:r.__setitem__('conforming',True)))
    bad(lambda:matrix(c.SUPP+'summary.json',lambda r:r['configurations'][0]['tests'].__setitem__('passed',209)))
    good(lambda:c.commands(receipt,worker,data,pins))
    for path,value in [('stdout',b'conforme\n'),('stderr',b'error'),('files/full_parallel.json',b'unexpected')]:
        changed = data.copy(); changed['results/cmd/002_combined_full/'+path] = value
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    for key,value in [('commands_ok','3'),('status','completed'),('interrupted','1')]:
        changed = dict(worker,**{key:value}); bad(lambda changed=changed:c.commands(receipt,changed,data,pins))
    for before,after in [(b'group_closed=1',b'group_closed=0'),(b'residual_group_killed=0',b'residual_group_killed=1'),
                          (b'streams_truncated=0',b'streams_truncated=1'),(b'exit_code=2',b'exit_code=0')]:
        changed = data.copy(); path = 'results/cmd/002_combined_full/meta.txt'
        changed[path] = changed[path].replace(before,after)
        if changed[path]==data[path]: raise RuntimeError('meta mutation did not apply')
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    good(lambda:c.other_mutants(data,pins))
    for before,after in [(b' TUE      code',b' TUE signal'),(b' TUE      construction',b' TUE delai'),
                          (b'dont_signal=0',b'dont_signal=1')]:
        changed = data.copy(); path = c.BASE+'mutants/LastTest.log'
        changed[path] = changed[path].replace(before,after,1)
        if changed[path]==data[path]: raise RuntimeError('mutation token not found')
        bad(lambda changed=changed:c.other_mutants(changed,pins))
    for kind in ('missing','cleanup','source','generation','compact','manifest','archive','target','guard','oslogin'):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            for name in ('results.tar.gz','matrix.json','asan18.json','inputs.json'):
                (folder/name).symlink_to(c.HERE/name)
            raw = c.js(Path(receipt['raw_receipt_local']).read_bytes()); compact = copy.deepcopy(receipt)
            if kind=='missing': raw.pop('private_key_deleted')
            elif kind=='cleanup': raw['reserve_released']=False; compact['reserve_released']=False
            elif kind=='source': raw['commit']='0'*40; compact['commit']='0'*40
            elif kind=='generation': raw['closing_generation']='wrong'; compact['closing_generation']='wrong'
            elif kind=='compact': (folder/'matrix.json').unlink(); (folder/'matrix.json').write_text('{}\n')
            elif kind=='manifest': raw['data_files'][0]['sha256']='0'*64; compact['data_files']=copy.deepcopy(raw['data_files'])
            elif kind=='target': raw['target']['zone']='us-central1-b'; compact['target']=copy.deepcopy(raw['target'])
            elif kind=='guard': raw['guest_guard_intact']=False
            elif kind=='oslogin':
                next(r for r in raw['host_commands'] if r['name']=='oslogin_remove')['exit_code']=1
            else: raw['results_sha256']='0'*64; compact['results_sha256']='0'*64
            blob = encode(raw); (folder/'raw.json').write_bytes(blob); (folder/'DONE').write_text('3\n')
            compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=c.sha(blob))
            (folder/'receipt.json').write_bytes(encode(compact)); bad(lambda:c.transport(folder,pins))
    print(json.dumps(dict(positives=positives,corruptions_rejected=rejected,native_calls=0),sort_keys=True))


if __name__=='__main__': main()
