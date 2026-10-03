"""LIVE reader of adaptive1's failure and separate cleanup recovery.

Code0 means coherent failed evidence, never a qualified campaign. No native run,
tar extraction, current benchmark import, or raw diagnostic output occurs here.
"""
import importlib.util
import json
import math
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
SOURCE = 'f425c5fe7f255150488c3d66c7a1e402ebabd59e'
CONTRACT = '89f534d97269e5d620c430df4036f38215cd47f686cb7af29035bbb3efd56374'
spec = importlib.util.spec_from_file_location('adaptive_failure_q4',
    HERE.parent.parent/'catalogue_q4_20261002/check.py')
q4 = importlib.util.module_from_spec(spec); spec.loader.exec_module(q4)
old = q4.old
need, js, sha, BASE = old.need, old.js, old.sha, old.BASE
SUPP = 'results/cmd/001_asan18/files/matrix/'
COMMANDS = ('000_matrice', '001_asan18', '002_adaptive')
MODULES = ('core', 'num', 'sched', 'cloud', 'index', 'catalogue', 'tower')


def same(left, right, why):
    need(json.dumps(left, sort_keys=True, allow_nan=False) ==
         json.dumps(right, sort_keys=True, allow_nan=False), why)


def source_contract():
    blob = (HERE/'source_contract.json').read_bytes()
    need(sha(blob) == CONTRACT, 'contract_pin')
    value = js(blob); need(value['source'] == SOURCE, 'source_pin')
    for path, pin in value['files'].items():
        result = subprocess.run(['git', 'show', SOURCE+':morsehgp3D_v11/'+path],
            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        need(result.returncode == 0 and sha(result.stdout) == pin['sha256'] and
             len(result.stdout) == pin['bytes'], 'git_source_pin')
        for key, filename in [('plan', 'bench/plans/catalogue_adaptive_g4.json'),
                              ('matrix', 'tools/g4_matrix.json'),
                              ('supplement', 'bench/meb_asan18_matrix.json')]:
            if path == filename: same(js(result.stdout), value[key], 'source_contract_copy')
    return value


def recovery(receipt, folder):
    compact = js((folder/'recovery.json').read_bytes())
    path = Path(compact['raw_recovery_local']); blob = path.read_bytes(); raw = js(blob)
    need(sha(blob) == compact['original_recovery_sha256'], 'recovery_raw_hash')
    required = set(compact)-{'raw_recovery_local', 'original_recovery_sha256'}
    need(required <= set(raw), 'recovery_raw_fields')
    same({k: raw[k] for k in required}, {k: compact[k] for k in required}, 'recovery_compact_copy')
    need(compact['schema'] == 'ehgp.v11.recovery_receipt.v1' and
         compact['session'] == str(Path(receipt['raw_receipt_local']).parent) == str(path.parent),
         'recovery_session')
    need(compact['status'] == 'stopped' and compact['closure'] == 'already_terminated' and
         compact['targeted_shutdown_certified'] is True and compact['oslogin_key_removed'] is True and
         compact['errors'] == [] and compact['warnings'] == [], 'recovery_closure')
    need(compact['target'] == receipt['target'] and compact['closing_generation'] == receipt['generation'] and
         compact['observed_before_stop']['lastStartTimestamp'] == receipt['generation'] and
         compact['observed_before_stop']['status'] == 'TERMINATED' and
         compact['observed_before_stop']['name'] == receipt['target']['instance'], 'recovery_generation')
    original = js(Path(receipt['raw_receipt_local']).read_bytes())
    for record, code in [(original, 1), (raw, 0)]:
        calls = [c for c in record['host_commands'] if c['name'] == 'oslogin_remove']
        need(len(calls) == 1 and type(calls[0]['exit_code']) is int and calls[0]['exit_code'] == code and
             ['compute', 'os-login', 'ssh-keys', 'remove'] == calls[0]['argv'][1:5], 'oslogin_removal_command')
    need(receipt['oslogin_key_removed'] is False and receipt['warnings'] ==
         ["retrait OS Login en echec : la cle expire d'elle-meme (65 min)"], 'original_cleanup_failure')
    diagnostic = js((folder/'cleanup_failure.json').read_bytes())
    log = Path(diagnostic['raw_log_local'])
    need(log.parent.parent.parent == Path(receipt['raw_receipt_local']).parent, 'cleanup_log_session')
    blob = log.read_bytes()
    need(sha(blob) == diagnostic['sha256'] and len(blob) == diagnostic['bytes'] and
         diagnostic['contains_aborted'] is True and b'ABORTED' in blob, 'cleanup_aborted_evidence')


def timeout_mutants(config, data):
    prefix = BASE+'mutants/'
    same(js(data[prefix+'result.json']), config, 'mutant_result_copy')
    need(config['name'] == 'mutants' and config['status'] == 'timeout' and
         config['conforming'] is False and config['reason'] == 'ctest : timeout' and
         prefix+'junit.xml' not in data, 'mutant_timeout')
    expected = [dict(name='mhgp11_mutants_'+m+s, disabled=False,
                     labels=['long' if s == '' else 'fast', 'mutant'])
                for m in MODULES for s in ('_manifest', '_manifest_opt', '')]
    selected = js(data[prefix+'tests.json']); same(selected, expected, 'mutant_selection')
    names = [t['name'] for t in selected]
    lines = data[prefix+'ctest.log'].decode().splitlines()
    need(len(lines) == 42 and lines[0].startswith('Test project '), 'mutant_log_inventory')
    starts, passed = [], []
    for line in lines[1:]:
        start = re.fullmatch(r'\s*Start (\d+): (\w+)', line)
        if start:
            need(len(starts) == len(passed), 'mutant_unfinished_before_next')
            starts.append((int(start[1]), start[2])); continue
        result = re.fullmatch(r'\s*(\d+)/21 Test #(\d+): (\w+) \.+\s+Passed\s+(\d+\.\d+) sec', line)
        need(result is not None and len(starts) == len(passed)+1 and
             int(result[1]) == len(starts) and (int(result[2]), result[3]) == starts[-1], 'mutant_ctest_verdict')
        passed.append(result[3])
    need([name for _, name in starts] == names and passed == names[:-1] and
         len({i for i, _ in starts}) == 21, 'mutant_log_selection')
    same(config['tests'], dict(selected=21, passed=20, failed=0, not_run=1,
         ctest_total=None, ctest_failed=None), 'mutant_counts')
    same(config['passed_labels'], dict(fast=14, long=6, mutant=20), 'mutant_labels')
    same(config['not_run'], [dict(test=names[-1], state='no_result', labels=['long', 'mutant'],
         seconds=0.0, detail='aucun resultat (ctest coupe, ou porte jamais lancee)', excerpt=[])], 'mutant_open_result')
    need(config['failures'] == [], 'mutant_failed_gates')
    steps = old.foundation.unique((s['name'], s) for s in config['steps'])
    need(set(steps) == {'configure', 'build', 'list', 'test'}, 'mutant_steps')
    need(all(steps[n]['status'] == 'ok' and steps[n]['exit_code'] == 0 for n in ('configure', 'build', 'list')),
         'mutant_preparation')
    test = steps['test']
    need(test['status'] == 'timeout' and test['timed_out'] is True and test['exit_code'] == -9 and
         math.isfinite(test['seconds']) and math.isfinite(test['timeout_seconds']) and
         0 < test['timeout_seconds'] <= test['seconds'] < 550 and 550 <= config['seconds'] < 600,
         'mutant_deadline')
    return (21, 20, 0, 1)


def build(config, data, declared):
    for key in ('cmake_options', 'ctest_args'):
        same(config[key], declared[key], 'declared_'+key)
    if config['name'] == 'gcc_asan_ubsan18':
        value = js(data[BASE+config['name']+'/build_provenance.json'])
        need(value['schema'] == 'ehgp.v11.build_provenance.v1' and value['complete'] is True and
             value['errors'] == [], 'supplement_provenance')
        proof = old.foundation.unique((f['path'], f) for f in value['files'])
        need({'CMakeCache.txt', 'libmhgp11.a', 'mhgp11_num_probe', 'mhgp11_index_probe',
              'mhgp11_tower_probe', 'mhgp11_tower_forest_probe'} <= set(proof), 'supplement_binaries')
        for name, row in proof.items():
            need(not Path(name).is_absolute() and '..' not in Path(name).parts and
                 old.HEX.fullmatch(row['sha256']) and type(row['size']) is int and row['size'] >= 0,
                 'supplement_provenance_fields')
            if 'text' in row:
                blob = row['text'].encode()
                need(len(blob) == row['size'] and sha(blob) == row['sha256'], 'supplement_provenance_text')
    else:
        proof = old.provenance(config, data)
    if config['status'] == 'absent': return
    need('CMakeCache.txt' in proof, 'build_cache')
    cache = dict(line.split('=', 1) for line in proof['CMakeCache.txt']['text'].splitlines()
                 if '=' in line and not line.startswith(('#', '//')))
    for flag in declared['cmake_options']:
        key, value = flag[2:].split('=', 1)
        actual = [v for k, v in cache.items() if k.split(':')[0] == key]
        need(actual == [value.replace('{threads}', str(config['threads']))], 'cache_option')
    return proof


def matrices(data, pins):
    summary = js(data[BASE+'summary.json']); configs = summary['configurations']
    expected = {c['name']: c for c in pins['matrix']['configurations']}
    names = [c['name'] for c in configs]
    need(len(names) == len(set(names)) and set(names) == set(expected) and
         summary['requested'] == names and summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and
         summary['complete'] is True and summary['conforming'] is False and
         type(summary['exit_code']) is int and summary['exit_code'] == 1 and summary['signals'] == [], 'matrix_status')
    same(summary['statuses'], {c['name']: c['status'] for c in configs}, 'matrix_statuses')
    need({p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == set(names),
         'matrix_directories')
    counts = {}
    for config in configs:
        name = config['name']; build(config, data, expected[name])
        counts[name] = timeout_mutants(config, data) if name == 'mutants' else old.foundation.judge_config(config, data)
        want = 'timeout' if name == 'mutants' else 'absent' if name == 'clang_release' else 'ok'
        need(config['status'] == want and counts[name][0] == pins['matrix_counts'][name], 'matrix_gate_counts')
    extra = js(data[SUPP+'summary.json']); cfgs = extra['configurations']
    need(extra['requested'] == ['gcc_asan_ubsan18'] and len(cfgs) == 1 and
         cfgs[0]['name'] == 'gcc_asan_ubsan18' and extra['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and
         extra['complete'] is True and extra['conforming'] is True and type(extra['exit_code']) is int and
         extra['exit_code'] == 0 and extra['signals'] == [] and
         extra['statuses'] == {'gcc_asan_ubsan18': 'ok'}, 'supplement_status')
    mapped = {BASE+p[len(SUPP):]: v for p, v in data.items() if p.startswith(SUPP)}
    build(cfgs[0], mapped, pins['supplement']['configurations'][0])
    need(old.foundation.judge_config(cfgs[0], mapped) == (161, 161, 0, 0), 'supplement_counts')
    return counts


def commands(receipt, worker, data, pins):
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(COMMANDS), 'command_inventory')
    for name, code, declared in zip(COMMANDS, (1, 0, 2), pins['plan']['commands']):
        prefix = 'results/cmd/'+name+'/'
        meta = old.fields(data[prefix+'meta.txt']); q4.command(meta, code)
        need(meta['requested_timeout_seconds'] == str(declared['timeout_seconds']) and
             meta['streams_truncated'] == '0' and meta['residual_group_killed'] == '0', 'command_closure')
        argv = shlex.split(data[prefix+'argv.txt'].decode())
        root = argv[1].split('/src/morsehgp3D_v11/', 1)[0]
        replacements = dict(src=root+'/src', build=root+'/build', data=root+'/data',
                            out=root+'/results/cmd/'+name+'/files')
        same(argv, [a.format(**replacements) for a in declared['argv']], 'executed_plan')
    prefix = 'results/cmd/002_adaptive/'
    need(data[prefix+'stdout'] == b'catalogue_adaptive_refused: ValueError\n' and data[prefix+'stderr'] == b'' and
         not any(p.startswith(prefix+'files/') for p in data), 'no_adaptive_benchmark')
    need(worker['commands_total'] == '3' and worker['commands_ok'] == '1' and worker['status'] == 'failed' and
         worker['interrupted'] == '0' and receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1,
         'session_failure')


def check(folder=HERE):
    pins = source_contract()
    receipt, worker, data = old.read_capture(folder)
    raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    need(set(receipt)-{'raw_receipt_local', 'original_receipt_sha256', 'preserved_failure'} <= set(raw),
         'receipt_raw_fields')
    need(receipt['commit'] == SOURCE and receipt['results_sha256'] == pins['archive_sha256'], 'pinned_capture')
    need(receipt['plan_sha256'] == pins['files']['bench/plans/catalogue_adaptive_g4.json']['sha256'] and
         receipt['worker_plan_sha256'] == worker['plan_sha256'], 'plan_identity')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip() == '3' and
         all(receipt[k] is True for k in ('private_key_deleted', 'reserve_released')) and
         receipt['preserved_failure'] is True, 'closed_failure')
    recovery(receipt, folder)
    members = set()
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest, name = line.split('  ', 1); name = 'results/'+name.removeprefix('./')
        need(name not in members and name in data and sha(data[name]) == digest, 'manifest_hash'); members.add(name)
    need(members == set(data)-{'results/MANIFEST.sha256'}, 'manifest_inventory')
    for file, path in [('matrix.json', BASE+'summary.json'), ('asan18.json', SUPP+'summary.json')]:
        need((folder/file).read_bytes() == data[path], 'compact_copy')
    old.inputs(folder, receipt)
    counts = matrices(data, pins); commands(receipt, worker, data, pins)
    return dict(coherent=True, conforming=False, source=SOURCE,
                matrix_selected=sum(c[0] for c in counts.values()), matrix_passed=sum(c[1] for c in counts.values()),
                matrix_failed=sum(c[2] for c in counts.values()), matrix_no_closed_result=1,
                open_test='mhgp11_mutants_tower', asan18_selected=161, asan18_passed=161,
                completed_mutation_campaign_gates=6, individual_mutant_verdicts='not_certified',
                adaptive_attempts=0, adaptive_report_present=False, memo_measurements=0,
                original_oslogin_key_removed=False, recovered_oslogin_key_removed=True)


if __name__ == '__main__':
    try: print(json.dumps(check(), sort_keys=True))
    except (old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError, OSError,
            old.foundation.ET.ParseError, tarfile.TarError) as error:
        print('REFUS '+(str(error) if isinstance(error, old.foundation.Refusal) else type(error).__name__))
        raise SystemExit(1)
