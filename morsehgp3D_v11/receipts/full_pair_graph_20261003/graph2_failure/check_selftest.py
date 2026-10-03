"""Pure adversarial controls: paired logs, closure, sources and declared-but-unstarted FULL units."""
import copy
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import check as c


def encode(value):return json.dumps(value,sort_keys=True,allow_nan=False).encode()


def main():
    pins=c.source_contract();receipt,worker,data=c.transport(c.HERE,pins);positives=rejected=0
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
                          ('gcc_release','status','failed'),('gcc_release','failures',[]),
                          ('gcc_release','not_run',[{'test':'missing'}]),('mutants','status','ok')]:
        bad(lambda name=name,key=key,value=value:config(name,lambda r:r.__setitem__(key,value)))
    for step,key,value in [(1,'exit_code',0),(1,'status','ok'),(3,'exit_code',0),(3,'timed_out',True)]:
        bad(lambda step=step,key=key,value=value:config('gcc_release',lambda r:r['steps'][step].__setitem__(key,value)))
    bad(lambda:config('gcc_release',lambda r:r['tests'].__setitem__('passed',639)))
    bad(lambda:config('gcc_release',lambda r:r['failures'].append(r['failures'][0])))
    bad(lambda:config('gcc_release',lambda r:r.__setitem__('threads',1)))
    for before,after in [(b"no match for 'operator=='",b"no match for 'operator!='"),
                         (b'descent_test.cpp:158:44',b'descent_test.cpp:157:44'),
                         (b'Wide<3>',b'Wide<2>')]:
        bad(lambda before=before,after=after:config('gcc_release',alter=lambda d,p:
            d.__setitem__(p+'build.log',replace(d[p+'build.log'],before,after))))
    bad(lambda:config('bits24',alter=lambda d,p:d.__setitem__(p+'build.log',
        b'\n'.join(line for line in d[p+'build.log'].splitlines() if b'error:' not in line or b':159:46' not in line))))
    def output(name,test,before,after):
        def alter(items,prefix):
            root=c.old.foundation.ET.fromstring(items[prefix+'junit.xml'])
            case=next(x for x in root.iter('testcase') if x.get('name')==test)
            node=case.find('system-out');original=node.text;node.text=replace(original,before,after)
            items[prefix+'junit.xml']=c.old.foundation.ET.tostring(root)
            items[prefix+'LastTest.log']=replace(items[prefix+'LastTest.log'],original.strip().encode(),node.text.strip().encode())
        return config(name,alter=alter)
    for test,before,after in [('mhgp11_tower_descent_singleton','lancement_impossible','code'),
                            ('mhgp11_tower_descent_inventaire','code 127 du shell','code 139 du shell'),
                            (c.VERTICAL,'obtenu 0, attendu 6','obtenu 6, attendu 6'),
                            (c.VERTICAL,'obtenu 0, attendu 1','obtenu 1, attendu 1'),
                            (c.VERTICAL,'obtenu 3, attendu 0','obtenu 0, attendu 0'),
                            (c.VERTICAL,'controles=38 echecs=8 plancher=30','controles=38 echecs=0 plancher=30'),
                            (c.VERTICAL,'code de sortie 1, attendu 0','code de sortie 139, attendu 0')]:
        bad(lambda test=test,before=before,after=after:output('gcc_release',test,before,after))
    bad(lambda:output('mutants','mhgp11_mutants_tower','TEMOIN ROUGE module=tower : aucun mutant juge',
                      'TEMOIN ROUGE module=tower : aucun mutant juge\nq3_non_strict_admis TUE code'))
    bad(lambda:config('gcc_release',alter=lambda d,p:d.__setitem__(p+'LastTest.log',
        replace(d[p+'LastTest.log'],b'\nTest Failed.\n',b'\nTest Passed.\n'))))
    bad(lambda:config('gcc_release',alter=lambda d,p:d.__setitem__(p+'LastTest.log',d[p+'LastTest.log']*2)))
    good(lambda:c.matrices(data,pins))
    for key,value in [('conforming',True),('exit_code',0),('complete',False),('signals',[15]),('budget_seconds',700)]:
        altered=data.copy();summary=c.js(altered[c.BASE+'summary.json']);summary[key]=value
        altered[c.BASE+'summary.json']=encode(summary);bad(lambda:c.matrices(altered,pins))
    altered=data.copy();summary=c.js(altered[c.SUPP+'summary.json']);summary['conforming']=True
    altered[c.SUPP+'summary.json']=encode(summary);bad(lambda:c.matrices(altered,pins))
    good(lambda:c.commands(receipt,worker,data,pins))
    for command in c.COMMANDS[2:]:
        for path,value in [('stdout',b'conforme\n'),('stderr',b'error'),('files/full_parallel.json',b'unexpected')]:
            changed=data.copy();changed['results/cmd/'+command+'/'+path]=value
            bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    for key,value in [('commands_ok','4'),('commands_total','3'),('status','completed'),('interrupted','1')]:
        changed=dict(worker,**{key:value});bad(lambda changed=changed:c.commands(receipt,changed,data,pins))
    for before,after in [(b'group_closed=1',b'group_closed=0'),(b'residual_group_killed=0',b'residual_group_killed=1'),
                         (b'streams_truncated=0',b'streams_truncated=1'),(b'exit_code=2',b'exit_code=0'),
                         (b'requested_timeout_seconds=570',b'requested_timeout_seconds=650')]:
        changed=data.copy();path='results/cmd/003_pair_graph_leaf8/meta.txt';changed[path]=replace(changed[path],before,after)
        bad(lambda changed=changed:c.commands(receipt,worker,changed,pins))
    good(lambda:c.mutants(data,pins))
    for ident in ('angle_droit_admis','prefixe_q3_obtus_rejete'):
        altered=data.copy();pattern=re.compile(re.escape(ident).encode()+rb'(\s+TUE\s+)(?:code|ligne)');hits=0
        for name,blob in list(altered.items()):
            if name.startswith(c.BASE+'mutants/'):
                changed,n=pattern.subn(lambda m:ident.encode()+m[1]+b'signal',blob);altered[name]=changed;hits+=n
        if not hits:raise RuntimeError('mutation log missing')
        bad(lambda:c.mutants(altered,pins))
    for kind in ('raw_field','cleanup','private_key','reserve','guest_guard','overflow','retrieval','stop_command',
                 'key_command','source','generation','target','start','compact','manifest','archive_hash'):
        with tempfile.TemporaryDirectory(prefix='graph2-proof-',dir=c.HERE) as tmp:
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
    good(lambda:c.preflight_and_first(c.HERE,pins));good(lambda:c.declared_schedule(pins));good(lambda:c.source_causes(pins))
    good(lambda:c.live_sources(c.HERE,receipt))
    for key in ('package_sha256','results_sha256'):
        changed=dict(receipt,**{key:'0'*64});bad(lambda changed=changed:c.live_sources(c.HERE,changed))
    for edit in (lambda r:r.__setitem__('commit','0'*40),lambda r:r.__setitem__('gcp_mutations','performed'),
                 lambda r:r['budget'].__setitem__('oversubscribed',True),lambda r:r['budget'].__setitem__('build_and_setup_seconds',0),
                 lambda r:r['budget'].__setitem__('worker_window_seconds',2400),
                 lambda r:r['plan']['commands'][3].__setitem__('timeout_seconds',650)):
        with tempfile.TemporaryDirectory(prefix='graph2-preflight-',dir=c.HERE) as tmp:
            folder=Path(tmp);p=copy.deepcopy(pins)
            for name in ('first_compile_failure.txt','first_vertical_failure.txt'):(folder/name).symlink_to(c.HERE/name)
            value=c.js((c.HERE/'preflight.json').read_bytes());edit(value);blob=encode(value)
            (folder/'preflight.json').write_bytes(blob)
            p['preflight.json']=dict(bytes=len(blob),sha256=c.sha(blob),raw_path=str(folder/'preflight.json'))
            bad(lambda:c.preflight_and_first(folder,p))
    # A changed dependency is rejected BEFORE executing any helper code.
    with tempfile.TemporaryDirectory(prefix='graph2-import-',dir=c.HERE) as tmp:
        folder=Path(tmp);shutil.copyfile(c.HERE/'check.py',folder/'check.py')
        shutil.copyfile(c.HERE/'source_contract.json',folder/'source_contract.json')
        for path in pins['helpers']:
            target=folder/'reader_sources'/path;target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(c.HERE/'reader_sources'/path,target)
        selected=folder/'reader_sources'/pins['helpers'][0]
        selected.write_text("raise RuntimeError('UNPINNED_HELPER_EXECUTED')\n")
        run=subprocess.run([sys.executable,'-B',str(folder/'check.py')],capture_output=True,text=True)
        c.need(run.returncode!=0 and 'historical_reader_pin' in run.stderr and
               'UNPINNED_HELPER_EXECUTED' not in run.stderr+run.stdout,'pin_before_import');rejected+=1
    print(json.dumps(dict(positives=positives,corruptions_rejected=rejected,native_calls=0),sort_keys=True))


if __name__=='__main__':main()
