"""Pure controls of dense1 failure evidence; paired logs are corrupted together."""
import copy
import json
from pathlib import Path
import re
import tempfile

import check as c


def encode(value): return json.dumps(value,sort_keys=True,allow_nan=False).encode()


def main():
    pins=c.source_contract();receipt,worker,data=c.transport(c.HERE,pins);positives,rejected=0,0
    def good(action):
        nonlocal positives
        action();positives+=1
    def bad(action):
        nonlocal rejected
        try:action()
        except c.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError):rejected+=1;return
        raise ValueError('corruption accepted')
    def replace(blob,before,after):
        changed=blob.replace(before,after)
        if changed==blob:raise RuntimeError('mutation not applied')
        return changed
    good(c.check)
    declarations={r['name']:r for r in pins['matrix']['configurations']}
    configs={r['name']:r for r in c.js(data[c.BASE+'summary.json'])['configurations']}
    def config(name,edit=lambda _:None,alter=lambda _d,_p:None):
        row,items=copy.deepcopy(configs[name]),data.copy();edit(row);prefix=c.BASE+name+'/'
        items[prefix+'result.json']=encode(row);alter(items,prefix)
        return c.gate(row,items,declarations[name])
    for name in ('gcc_release','gcc_asan_ubsan','mutants'):good(lambda name=name:config(name))
    for name,key,value in [('gcc_release','status','ok'),('gcc_release','conforming',True),
                           ('gcc_release','status','build_failed'),('gcc_release','failures',[]),
                           ('gcc_release','not_run',[{'test':'missing'}]),('mutants','status','failed')]:
        bad(lambda name=name,key=key,value=value:config(name,lambda r:r.__setitem__(key,value)))
    for name,step,key,value in [('gcc_release',1,'exit_code',2),('gcc_release',3,'exit_code',0),
                               ('gcc_release',1,'status','failed'),('gcc_release',3,'timed_out',True),
                               ('mutants',3,'exit_code',8)]:
        bad(lambda name=name,step=step,key=key,value=value:config(name,lambda r:r['steps'][step].__setitem__(key,value)))
    bad(lambda:config('gcc_release',lambda r:r['tests'].__setitem__('passed',600)))
    bad(lambda:config('gcc_release',lambda r:r['failures'].append(r['failures'][0])))
    bad(lambda:config('gcc_release',lambda r:r.__setitem__('threads',1)))
    bad(lambda:config('gcc_release',alter=lambda d,p:d.__setitem__(p+'build.log',d[p+'build.log']+b'error: failure\n')))
    def output(name,before,after):
        def alter(items,prefix):
            root=c.old.foundation.ET.fromstring(items[prefix+'junit.xml'])
            case=next(x for x in root.iter('testcase') if x.get('name')==c.FAILED)
            node=case.find('system-out');original=node.text;node.text=replace(original,before,after)
            items[prefix+'junit.xml']=c.old.foundation.ET.tostring(root)
            items[prefix+'LastTest.log']=replace(items[prefix+'LastTest.log'],original.strip().encode(),node.text.strip().encode())
        return config(name,alter=alter)
    for name,before,after in [('gcc_release','oracle: anchor','oracle: native process'),
                             ('gcc_asan_ubsan','oracle: native process','oracle: anchor'),
                             ('gcc_release','run_expect_verdict code','run_expect_verdict lancement_impossible'),
                             ('gcc_release','code de sortie 1, attendu 0','code de sortie 139, attendu 0'),
                             ('gcc_asan_ubsan','native process','native process\nstack-use-after-scope'),
                             ('gcc_release','REFUS orientation_certificate_oracle: anchor','mhgp11_test_ok')]:
        bad(lambda name=name,before=before,after=after:output(name,before,after))
    def log_change(items,prefix):items[prefix+'LastTest.log']=replace(items[prefix+'LastTest.log'],b'\nTest Failed.\n',b'\nTest Passed.\n')
    bad(lambda:config('gcc_release',alter=log_change))
    def log_double(items,prefix):items[prefix+'LastTest.log']+=items[prefix+'LastTest.log']
    bad(lambda:config('gcc_release',alter=log_double))
    good(lambda:c.matrices(data,pins))
    for key,value in [('conforming',True),('exit_code',0),('complete',False),('signals',[15]),('budget_seconds',700)]:
        altered=data.copy();summary=c.js(altered[c.BASE+'summary.json']);summary[key]=value
        altered[c.BASE+'summary.json']=encode(summary);bad(lambda:c.matrices(altered,pins))
    altered=data.copy();summary=c.js(altered[c.SUPP+'summary.json']);summary['conforming']=True
    altered[c.SUPP+'summary.json']=encode(summary);bad(lambda:c.matrices(altered,pins))
    good(lambda:c.commands(receipt,worker,data,pins))
    for path,value in [('stdout',b'conforme\n'),('stderr',b'error'),('files/full_parallel.json',b'unexpected')]:
        changed=data.copy();changed['results/cmd/002_dense_full/'+path]=value
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    for key,value in [('commands_ok','3'),('status','completed'),('interrupted','1')]:
        changed=dict(worker,**{key:value});bad(lambda changed=changed:c.commands(receipt,changed,data,pins))
    for before,after in [(b'group_closed=1',b'group_closed=0'),(b'residual_group_killed=0',b'residual_group_killed=1'),
                          (b'streams_truncated=0',b'streams_truncated=1'),(b'exit_code=2',b'exit_code=0'),
                          (b'requested_timeout_seconds=570',b'requested_timeout_seconds=650')]:
        changed=data.copy();path='results/cmd/002_dense_full/meta.txt';changed[path]=replace(changed[path],before,after)
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    good(lambda:c.mutants(data,pins))
    for module,ident in [('tower','forest_cohort_nonbirth_reset'),('num','orientation_native_sign_inverted_u21')]:
        c.need(ident in pins['mutants'][module],'targeted mutant exists')
        altered=data.copy();pattern=re.compile(re.escape(ident).encode()+rb'(\s+TUE\s+)(?:code|ligne)');hits=0
        for name,blob in list(altered.items()):
            if name.startswith(c.BASE+'mutants/'):
                changed,n=pattern.subn(lambda m:ident.encode()+m[1]+b'signal',blob);altered[name]=changed;hits+=n
        if not hits:raise RuntimeError('mutation log missing')
        bad(lambda:c.mutants(altered,pins))
    for kind in ('raw_field','cleanup','private_key','reserve','guest_guard','overflow','retrieval','stop_command',
                 'key_command','source','generation','target','start','compact','manifest','archive_hash'):
        with tempfile.TemporaryDirectory(prefix='dense1-proof-') as tmp:
            folder=Path(tmp);compact=copy.deepcopy(receipt);raw=c.js(Path(receipt['raw_receipt_local']).read_bytes())
            for name in ('results.tar.gz','matrix.json','asan18.json','inputs.json'):(folder/name).symlink_to(c.HERE/name)
            if kind=='raw_field':raw.pop('targeted_shutdown_certified')
            elif kind=='cleanup':raw['oslogin_key_removed']=False;compact['oslogin_key_removed']=False
            elif kind=='private_key':raw['private_key_deleted']=False;compact['private_key_deleted']=False
            elif kind=='reserve':raw['reserve_released']=False;compact['reserve_released']=False
            elif kind=='guest_guard':raw['guest_guard_intact']=False
            elif kind=='overflow':raw['overflow']['evicted']=['lost']
            elif kind=='retrieval':raw['retrieval']='missing'
            elif kind in ('stop_command','key_command'):
                name='guarded_stop' if kind=='stop_command' else 'oslogin_remove'
                next(r for r in raw['host_commands'] if r['name']==name)['exit_code']=1
            elif kind=='source':raw['commit']='0'*40;compact['commit']='0'*40
            elif kind=='target':raw['target']['zone']='us-central1-b';compact['target']=copy.deepcopy(raw['target'])
            elif kind=='start':raw['start_certified']=False;compact['start_certified']=False
            elif kind=='generation':raw['closing_generation']='wrong';compact['closing_generation']='wrong'
            elif kind=='compact':(folder/'matrix.json').unlink();(folder/'matrix.json').write_text('{}\n')
            elif kind=='manifest':raw['data_files'][0]['sha256']='0'*64;compact['data_files']=copy.deepcopy(raw['data_files'])
            else:raw['results_sha256']='0'*64;compact['results_sha256']='0'*64
            blob=encode(raw);(folder/'raw.json').write_bytes(blob);(folder/'DONE').write_text('3\n')
            compact.update(raw_receipt_local=str(folder/'raw.json'),original_receipt_sha256=c.sha(blob))
            (folder/'receipt.json').write_bytes(encode(compact));bad(lambda:c.transport(folder,pins))
    good(lambda:c.preflight_and_first(c.HERE,pins));good(lambda:c.declared_schedule(pins));good(lambda:c.source_lifetime(pins))
    good(lambda:c.live_sources(c.HERE,receipt))
    for key in ('package_sha256','results_sha256'):
        changed=dict(receipt,**{key:'0'*64});bad(lambda changed=changed:c.live_sources(c.HERE,changed))
    for edit in (lambda r:r.__setitem__('commit','0'*40),lambda r:r.__setitem__('gcp_mutations','performed'),
                 lambda r:r['budget'].__setitem__('oversubscribed',True),lambda r:r['budget'].__setitem__('build_and_setup_seconds',0),
                 lambda r:r['budget'].__setitem__('worker_window_seconds',1800),
                 lambda r:r['plan']['commands'][2].__setitem__('timeout_seconds',650)):
        with tempfile.TemporaryDirectory(prefix='dense1-preflight-') as tmp:
            folder=Path(tmp);p=copy.deepcopy(pins);(folder/'first_failure.txt').symlink_to(c.HERE/'first_failure.txt')
            value=c.js((c.HERE/'preflight.json').read_bytes());edit(value);blob=encode(value)
            (folder/'preflight.json').write_bytes(blob)
            p['preflight.json']=dict(bytes=len(blob),sha256=c.sha(blob),raw_path=str(folder/'preflight.json'))
            bad(lambda:c.preflight_and_first(folder,p))
    print(json.dumps(dict(positives=positives,corruptions_rejected=rejected,native_calls=0),sort_keys=True))


if __name__=='__main__':main()
