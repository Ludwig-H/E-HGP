"""Reader of the closed full1 failure only. Code0 preserves, never promotes, that failure."""
import importlib.util
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('full_failure_q4',HERE.parent/'catalogue_q4_20261002/check.py')
q4 = importlib.util.module_from_spec(spec); spec.loader.exec_module(q4)
old,need,js,BASE = q4.old,q4.need,q4.js,q4.BASE
SOURCE = '80e77544e018b42093e8369a587ce7a2e85a6b88'
ARCHIVE = 'f6f76b50c979e3fc27db07a5f544d92d72aef324865ac301a10487889ba41646'
FAILURES = {'mhgp11_tower_full_bench_io','mhgp11_tower_full_bench_io_opt'}
SUPP = 'results/cmd/001_asan18/files/matrix/'
DIFF = 'results/cmd/002_v10diff/files/diff/'


def configuration(cfg,data,prefix):
    if cfg['status'] == 'absent': return
    cases = list(old.foundation.ET.fromstring(data[prefix+cfg['name']+'/junit.xml']).iter('testcase'))
    bad = {c.get('name') for c in cases if c.find('failure') is not None}
    expected = set() if cfg['name'] in ('style','mutants') else FAILURES
    need(bad == expected and {f['test'] for f in cfg['failures']} == expected,'failure_inventory')
    for case in cases:
        need(case.get('status') == ('fail' if case.get('name') in bad else 'run') and
             case.find('skipped') is None,'unexecuted_gate')
        if case.get('name') in bad:
            need('ValueError: FULL natif petit temoin' in (case.findtext('system-out') or ''),'failure_diagnostic')


def check(folder):
    receipt,worker,data = old.read_capture(folder)
    need(receipt['commit'] == SOURCE and receipt['results_sha256'] == ARCHIVE,'pinned_capture')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip() == '3' and
         all(receipt[k] is True for k in ('private_key_deleted','oslogin_key_removed','reserve_released')),'closed_failure')
    names = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest,name = line.split('  ',1); name = 'results/'+name.removeprefix('./')
        need(name not in names and name in data and old.sha(data[name]) == digest,'archive_manifest'); names.add(name)
    need(names == set(data)-{'results/MANIFEST.sha256'},'archive_inventory')
    for filename,path in [('matrix.json',BASE+'summary.json'),('asan18.json',SUPP+'summary.json'),
                          ('v10diff.json',DIFF+'summary.json')]:
        need((folder/filename).read_bytes() == data[path],'compact_copy')
    old.inputs(folder,receipt)
    counts,builds,code = q4.judge_matrix(data)
    need(code == 1,'matrix_failure_preserved')
    for cfg in js(data[BASE+'summary.json'])['configurations']: configuration(cfg,data,BASE)
    extra = js(data[SUPP+'summary.json']); cfg = extra['configurations'][0]
    need(extra['requested'] == ['gcc_asan_ubsan18'] and len(extra['configurations']) == 1 and
         extra['complete'] is True and extra['conforming'] is False and extra['exit_code'] == 1,'supplement_status')
    mapped = {BASE+p[len(SUPP):]:v for p,v in data.items() if p.startswith(SUPP)}
    result = old.foundation.judge_config(cfg,mapped); old.provenance(cfg,mapped)
    configuration(cfg,data,SUPP)
    need(result == (139,137,2,0),'supplement_counts')
    proof = js(data[SUPP+'gcc_asan_ubsan18/build_provenance.json'])
    need(proof['complete'] is True and not proof['errors'],'supplement_provenance')
    diff = js(data[DIFF+'summary.json'])
    need(diff['source_v10'] == 'c764e121aa52f2e5dd9b85fbe308c9c3511ff55e' and
         diff['cases'] == [] and diff['complete'] is False and diff['conforming'] is False and
         diff['errors'] == ['ValueError: qualification incomplete/non conforme'] and
         diff['qualification_sha256'] == old.sha(data[BASE+'summary.json']),'diff_refusal')
    need([c['name'] for c in diff['commands']] == ['configure_v10','build_v10'],'diff_commands')
    for command in diff['commands']:
        need(command['status'] == 'ok' and command['returncode'] == 0,'v10_build')
        for stream in ('stdout','stderr'):
            entry = command[stream]; blob = data[DIFF+entry['path']]
            need(entry['sha256'] == old.sha(blob) and entry['bytes'] == len(blob),'v10_stream')
    names = ('000_matrice','001_asan18','002_v10diff','003_full')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(names),'commands')
    metas = [old.fields(data['results/cmd/'+n+'/meta.txt']) for n in names]
    for meta,exit_code,timeout in zip(metas,(1,1,1,2),(650,200,300,680)):
        q4.command(meta,exit_code)
        need(meta['group_closed'] == '1' and meta['streams_truncated'] == '0' and
             meta['residual_group_killed'] == '0' and meta['requested_timeout_seconds'] == str(timeout),'command_closure')
    need(data['results/cmd/003_full/stdout'] == b'full_campaign_refused: ValueError\n' and
         not any(p.startswith('results/cmd/003_full/files/') for p in data),'no_full_benchmark')
    need(worker['commands_total'] == '4' and worker['commands_ok'] == '0' and worker['status'] == 'failed' and
         receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1 and
         receipt['preserved_failure'] is True,'worker_failure')
    return dict(coherent=True,conforming=False,source=SOURCE,matrix_selected=sum(c[0] for c in counts.values()),
                matrix_passed=sum(c[1] for c in counts.values()),matrix_failed=sum(c[2] for c in counts.values()),
                asan18_selected=139,asan18_passed=137,asan18_failed=2,
                distinct_failed_gates=sorted(FAILURES),v10_build_ok=True,v10_comparisons=0,full_benchmarks=0)


if __name__ == '__main__':
    try: print(json.dumps(check(HERE/'full1'),sort_keys=True))
    except (old.foundation.Refusal,ValueError,KeyError,TypeError,IndexError,OSError) as error:
        print('REFUS '+str(error)); raise SystemExit(1)
