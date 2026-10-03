"""Pure controls of preserved evidence, including coupled JUnit/LastTest corruptions."""
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
    def replace(blob,before,after):
        modified = blob.replace(before,after)
        if modified==blob: raise RuntimeError('mutation did not apply')
        return modified
    good(c.check)
    declarations = {r['name']:r for r in pins['matrix']['configurations']}
    configs = {r['name']:r for r in c.js(data[c.BASE+'summary.json'])['configurations']}
    def config(name,edit=lambda _r:None,alter=lambda _d,_p:None):
        value,items = copy.deepcopy(configs[name]),data.copy(); edit(value); prefix = c.BASE+name+'/'
        items[prefix+'result.json'] = encode(value); alter(items,prefix)
        return c.gate(value,items,declarations[name])
    good(lambda:config('gcc_release')); good(lambda:config('mutants')); good(lambda:config('bits24'))
    for name,key,value in [('gcc_release','status','build_failed'),('gcc_release','conforming',False),
                           ('gcc_release','not_run',[{'test':'missing'}]),('mutants','status','ok'),
                           ('mutants','conforming',True),('mutants','failures',[])]:
        bad(lambda name=name,key=key,value=value:config(name,lambda r:r.__setitem__(key,value)))
    for name,step,key,value in [('gcc_release',1,'exit_code',2),('mutants',3,'exit_code',0),
                                ('gcc_release',1,'status','failed'),('mutants',3,'timed_out',True)]:
        bad(lambda name=name,step=step,key=key,value=value:config(name,lambda r:r['steps'][step].__setitem__(key,value)))
    bad(lambda:config('gcc_release',lambda r:r['tests'].__setitem__('passed',579)))
    bad(lambda:config('mutants',lambda r:r['failures'].append(r['failures'][0])))
    bad(lambda:config('gcc_release',lambda r:r.__setitem__('threads',1)))
    bad(lambda:config('gcc_release',alter=lambda d,p:d.__setitem__(p+'build.log',d[p+'build.log']+b'error: compiler failure\n')))
    def mutate_proof(items,prefix):
        value=c.js(items[prefix+'build_provenance.json']); value['complete']=False
        items[prefix+'build_provenance.json']=encode(value)
    bad(lambda:config('gcc_release',alter=mutate_proof))
    def matrix(path,edit):
        changed=data.copy(); value=c.js(changed[path]); edit(value); changed[path]=encode(value)
        return c.matrices(changed,pins)
    good(lambda:c.matrices(data,pins))
    for key,value in [('conforming',True),('exit_code',0),('complete',False),('signals',[15]),('budget_seconds',700)]:
        bad(lambda key=key,value=value:matrix(c.BASE+'summary.json',lambda r:r.__setitem__(key,value)))
    bad(lambda:matrix(c.BASE+'summary.json',lambda r:r['configurations'].pop()))
    bad(lambda:matrix(c.SUPP+'summary.json',lambda r:r.__setitem__('conforming',False)))
    bad(lambda:matrix(c.SUPP+'summary.json',lambda r:r['configurations'][0]['tests'].__setitem__('passed',263)))
    good(lambda:c.commands(receipt,worker,data,pins))
    for path,value in [('stdout',b'conforme\n'),('stderr',b'error'),('files/full_parallel.json',b'unexpected')]:
        changed=data.copy(); changed['results/cmd/002_census_full/'+path]=value
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    for key,value in [('commands_ok','3'),('status','completed'),('interrupted','1')]:
        changed=dict(worker,**{key:value}); bad(lambda changed=changed:c.commands(receipt,changed,data,pins))
    for before,after in [(b'group_closed=1',b'group_closed=0'),(b'residual_group_killed=0',b'residual_group_killed=1'),
                          (b'streams_truncated=0',b'streams_truncated=1'),(b'exit_code=2',b'exit_code=0'),
                          (b'requested_timeout_seconds=570',b'requested_timeout_seconds=650')]:
        changed=data.copy(); path='results/cmd/002_census_full/meta.txt'
        changed[path]=replace(changed[path],before,after)
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    changed=data.copy(); path='results/cmd/002_census_full/argv.txt'
    changed[path]=replace(changed[path],b'--reuse-census',b'--parallel-verticals')
    bad(lambda:c.commands(receipt,worker,changed,pins))
    good(lambda:c.other_mutants(data,pins)); good(lambda:c.tower_mutants(data,pins))
    prefix=c.BASE+'mutants/'
    def tower(edit):
        changed=data.copy(); root=c.old.foundation.ET.fromstring(changed[prefix+'junit.xml'])
        case=next(x for x in root.iter('testcase') if x.get('name')==c.FAILED)
        output=case.find('system-out'); before=output.text; after=edit(before)
        if before==after: raise RuntimeError('tower mutation did not apply')
        output.text=after; changed[prefix+'junit.xml']=c.old.foundation.ET.tostring(root)
        changed[prefix+'LastTest.log']=replace(changed[prefix+'LastTest.log'],before.strip().encode(),after.strip().encode())
        return c.tower_mutants(changed,pins)
    # Both logs change together, so the failure diagnosis, not copy equality, must reject them.
    for before,after in [
        ('INVALIDE le mutant ne passe pas la construction : il doit etre tue par sa porte','TUE      construction'),
        ('INVALIDES module=tower : 1 sur 88','mutants_ok module=tower mutants=88 tues=88'),
        ('[-Werror=maybe-uninitialized]','[other diagnostic]'),
        ('forest_build.cpp:63:15: error:','forest_build.cpp:62:15: error:'),
        ('59 |   std::optional<LevelRank> rank;','59 |   unknown;'),
        ('63 |     if (!rank || *rank != next)','63 |     if (false)'),
        ('cc1plus: all warnings being treated as errors','other compiler'),
        ('run_expect_verdict code','run_expect_verdict delai'),
        ('code de sortie 3, attendu 0','code de sortie 0, attendu 0'),
        ('TUE      code','TUE      signal'),
        ('q3_non_strict_admis','missing_mutant'),
        ('census_reuse_logical_slot_count','census_reuse_fake_second_pass'),
    ]:
        bad(lambda before=before,after=after:tower(lambda s:s.replace(before,after)))
    bad(lambda:tower(lambda s:s.replace('ordre_supports_inverse                   TUE      code\n','')))
    changed=data.copy(); path=prefix+'LastTest.log'
    changed[path]=replace(changed[path],b'\nTest Failed.\n',b'\nTest Passed.\n')
    bad(lambda:c.tower_mutants(changed,pins))
    changed=data.copy(); changed[prefix+'LastTest.log']+=changed[prefix+'LastTest.log']
    bad(lambda:c.tower_mutants(changed,pins))
    changed=data.copy(); changed[prefix+'junit.xml']=replace(changed[prefix+'junit.xml'],b'name="mhgp11_mutants_core"',b'name="missing_core"')
    bad(lambda:c.other_mutants(changed,pins))
    wrong=copy.deepcopy(pins); wrong['mutants']['tower'][c.INVALID]='construction'
    bad(lambda:c.tower_mutants(data,wrong))
    for kind in ('raw_field','cleanup','private_key','reserve','guest_guard','overflow','retrieval','stop_command',
                 'key_command','source','generation','target','start','compact','manifest','archive_hash'):
        with tempfile.TemporaryDirectory(prefix='census-failure-reader-') as tmp:
            folder=Path(tmp); compact=copy.deepcopy(receipt); raw=c.js(Path(receipt['raw_receipt_local']).read_bytes())
            for name in ('results.tar.gz','matrix.json','asan18.json','inputs.json'): (folder/name).symlink_to(c.HERE/name)
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
            elif kind=='target': raw['target']['zone']='us-central1-b'; compact['target']=copy.deepcopy(raw['target'])
            elif kind=='start': raw['start_certified']=False; compact['start_certified']=False
            elif kind=='generation': raw['closing_generation']='wrong'; compact['closing_generation']='wrong'
            elif kind=='compact': (folder/'matrix.json').unlink(); (folder/'matrix.json').write_text('{}\n')
            elif kind=='manifest': raw['data_files'][0]['sha256']='0'*64; compact['data_files']=copy.deepcopy(raw['data_files'])
            else: raw['results_sha256']='0'*64; compact['results_sha256']='0'*64
            blob=encode(raw); (folder/'raw.json').write_bytes(blob); (folder/'DONE').write_text('3\n')
            compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=c.sha(blob))
            (folder/'receipt.json').write_bytes(encode(compact)); bad(lambda:c.transport(folder,pins))
    good(lambda:c.preflight_and_first(c.HERE,pins)); good(lambda:c.declared_schedule(pins))
    good(lambda:c.live_sources(c.HERE,receipt))
    for key in ('package_sha256','results_sha256'):
        changed=dict(receipt,**{key:'0'*64}); bad(lambda changed=changed:c.live_sources(c.HERE,changed))

    mutations=[
        lambda r:r.__setitem__('commit','0'*40),lambda r:r.__setitem__('gcp_mutations','performed'),
        lambda r:r.__setitem__('plan_sha256','0'*64),lambda r:r.__setitem__('warnings',['lost warning']),
        lambda r:r['budget'].__setitem__('oversubscribed',True),lambda r:r['budget'].__setitem__('build_and_setup_seconds',0),
        lambda r:r['budget'].__setitem__('worker_window_seconds',1800),
        lambda r:r['budget'].__setitem__('command_timeouts_sum_seconds',1680),
        lambda r:r['plan']['commands'][2].__setitem__('timeout_seconds',650),
    ]
    for edit in mutations:
        with tempfile.TemporaryDirectory(prefix='census-preflight-reader-') as tmp:
            folder=Path(tmp); p=copy.deepcopy(pins); (folder/'first_failure.txt').symlink_to(c.HERE/'first_failure.txt')
            value=c.js((c.HERE/'preflight.json').read_bytes()); edit(value)
            blob=encode(value); (folder/'preflight.json').write_bytes(blob)
            p['preflight.json']=dict(bytes=len(blob),sha256=c.sha(blob),raw_path=str(folder/'preflight.json'))
            bad(lambda:c.preflight_and_first(folder,p))
    with tempfile.TemporaryDirectory(prefix='census-reader-import-') as tmp:
        folder=Path(tmp)
        for filename in ('check.py','source_contract.json'): (folder/filename).write_bytes((c.HERE/filename).read_bytes())
        for path in pins['helpers']:
            destination=folder/'reader_sources'/path; destination.parent.mkdir(parents=True,exist_ok=True)
            destination.write_bytes((c.HERE/'reader_sources'/path).read_bytes())
        first=folder/'reader_sources'/pins['helpers'][0]; first.write_bytes(first.read_bytes()+b'\n# corrupted helper\n')
        bad(lambda:c.load('bad_frozen_census_reader',folder/'check.py'))
    print(json.dumps(dict(positives=positives,corruptions_rejected=rejected,native_calls=0),sort_keys=True))


if __name__=='__main__': main()
