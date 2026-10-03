"""LIVE adaptive2 build-failure reader. Code0 preserves a nonconforming campaign.

Only archived JUnit/logs and pinned Git source are read; no benchmark or native
executable is imported or launched. Original local receipt remains mandatory.
"""
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
SOURCE = '18d1ba695022457b82117df3cc2dfba821d908ae'
CONTRACT = '183d5facd66fa37fe6cdea7d9bf1720c590b53923bd4e24d19f7dc3caddcac39'
spec = importlib.util.spec_from_file_location('adaptive2_failure_previous', HERE.parent/'adaptive1_failure/check.py')
first = importlib.util.module_from_spec(spec); spec.loader.exec_module(first)
q4, old = first.q4, first.old
need, js, sha, same, BASE, SUPP = first.need, first.js, first.sha, first.same, first.BASE, first.SUPP
FAILED = {'mhgp11_tower_memo_fault_starvation', 'mhgp11_tower_memo_fault_inventaire'}


def source_contract():
    blob = (HERE/'source_contract.json').read_bytes(); need(sha(blob) == CONTRACT, 'contract_pin')
    value = js(blob); need(value['source'] == SOURCE, 'source_pin')
    for path, pin in value['files'].items():
        result = subprocess.run(['git', 'show', SOURCE+':morsehgp3D_v11/'+path], cwd=HERE.parents[3],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        need(result.returncode == 0 and sha(result.stdout) == pin['sha256'] and
             len(result.stdout) == pin['bytes'], 'git_source_pin')
        for key, file in [('plan', 'bench/plans/catalogue_adaptive_g4.json'), ('matrix', 'tools/g4_matrix.json'),
                          ('supplement', 'bench/meb_asan18_matrix.json')]:
            if path == file: same(js(result.stdout), value[key], 'source_contract_copy')
        if path == 'tests/tower/memo_fault.cpp':
            need(result.stdout.splitlines()[30].strip() ==
                 b'for (auto& t : times.orders) t={2,3,5,7}; const auto preserved=times;', 'failing_source_line')
    return value


def build_failure(config, data, declared):
    name = config['name']; prefix = BASE+name+'/'
    result = old.foundation.judge_config(config, data)
    first.build(config, data, declared)
    if name in ('style', 'clang_release'):
        need(config['status'] == ('ok' if name == 'style' else 'absent'), 'optional_or_style')
        return result
    need(config['status'] == 'build_failed' and config['conforming'] is False and config['not_run'] == [],
         'build_failure_status')
    steps = old.foundation.unique((s['name'], s) for s in config['steps'])
    for key, status, code in [('configure', 'ok', 0), ('build', 'failed', 2), ('list', 'ok', 0), ('test', 'failed', 8)]:
        need(steps[key]['status'] == status and steps[key]['exit_code'] == code and
             steps[key].get('timed_out', False) is False, 'build_failure_steps')
    log = data[prefix+'build.log'].decode()
    errors = [line for line in log.splitlines() if 'error:' in line]
    need(len(errors) == 1 and 'memo_fault.cpp:31:5: error:' in errors[0] and
         errors[0].endswith('[-Werror=misleading-indentation]') and
         'for (auto& t : times.orders) t={2,3,5,7}; const auto preserved=times;' in log, 'compiler_diagnostic')
    cases = list(old.foundation.ET.fromstring(data[prefix+'junit.xml']).iter('testcase'))
    expected = {'mhgp11_mutants_tower'} if name == 'mutants' else FAILED
    bad = {c.get('name') for c in cases if c.find('failure') is not None}
    need(bad == expected and {f['test'] for f in config['failures']} == expected and
         len(config['failures']) == len(expected), 'failed_gate_inventory')
    for case in cases:
        failed = case.get('name') in expected
        need(case.get('status') == ('fail' if failed else 'run') and case.find('skipped') is None, 'gate_executed_state')
        if not failed: continue
        output = case.findtext('system-out') or ''
        if name == 'mutants':
            need('temoin [] : construction en echec' in output and
                 'TEMOIN ROUGE module=tower : aucun mutant juge' in output and
                 '[-Werror=misleading-indentation]' in output and
                 re.search(r'^TUE\b', output, re.MULTILINE) is None and
                 'run_expect_verdict code' in output.splitlines(), 'mutant_reference_not_built')
        else:
            need('run_expect_verdict lancement_impossible' in output.splitlines() and
                 'code 127 du shell' in output and 'mhgp11_tower_memo_fault' in output and
                 'mhgp11_test_ok' not in output, 'native_test_not_launched')
    need(result[2:] == (len(expected), 0), 'failure_counts')
    return result


def matrices(data, pins):
    summary = js(data[BASE+'summary.json']); configs = summary['configurations']
    declared = {c['name']: c for c in pins['matrix']['configurations']}
    names = [c['name'] for c in configs]
    need(len(names) == len(set(names)) and set(names) == set(declared) and summary['requested'] == names and
         summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         summary['conforming'] is False and type(summary['exit_code']) is int and
         summary['exit_code'] == 1 and summary['signals'] == [], 'matrix_status')
    same(summary['statuses'], {c['name']: c['status'] for c in configs}, 'matrix_statuses')
    need({p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == set(names),
         'matrix_directories')
    counts = {c['name']: build_failure(c, data, declared[c['name']]) for c in configs}
    need(all(counts[name][0] == pins['matrix_counts'][name] for name in names), 'matrix_counts')
    extra = js(data[SUPP+'summary.json']); cfgs = extra['configurations']
    need(extra['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and extra['complete'] is True and
         extra['requested'] == ['gcc_asan_ubsan18'] and len(cfgs) == 1 and cfgs[0]['name'] == 'gcc_asan_ubsan18' and
         extra['conforming'] is False and type(extra['exit_code']) is int and extra['exit_code'] == 1 and
         extra['signals'] == [] and extra['statuses'] == {'gcc_asan_ubsan18': 'build_failed'}, 'supplement_status')
    mapped = {BASE+p[len(SUPP):]: v for p, v in data.items() if p.startswith(SUPP)}
    need(build_failure(cfgs[0], mapped, pins['supplement']['configurations'][0]) == (178, 176, 2, 0),
         'supplement_counts')
    return counts


def commands(receipt, worker, data, pins):
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(first.COMMANDS), 'command_inventory')
    for name, code, declared in zip(first.COMMANDS, (1, 1, 2), pins['plan']['commands']):
        prefix = 'results/cmd/'+name+'/'
        meta = old.fields(data[prefix+'meta.txt']); q4.command(meta, code)
        need(meta['requested_timeout_seconds'] == str(declared['timeout_seconds']) and
             meta['streams_truncated'] == '0' and meta['residual_group_killed'] == '0', 'command_closure')
        argv = shlex.split(data[prefix+'argv.txt'].decode()); root = argv[1].split('/src/morsehgp3D_v11/', 1)[0]
        replacements = dict(src=root+'/src', build=root+'/build', data=root+'/data',
                            out=root+'/results/cmd/'+name+'/files')
        same(argv, [a.format(**replacements) for a in declared['argv']], 'executed_plan')
    prefix = 'results/cmd/002_adaptive/'
    need(data[prefix+'stdout'] == b'catalogue_adaptive_refused: ValueError\n' and data[prefix+'stderr'] == b'' and
         not any(p.startswith(prefix+'files/') for p in data), 'no_adaptive_benchmark')
    need(worker['commands_total'] == '3' and worker['commands_ok'] == '0' and worker['status'] == 'failed' and
         worker['interrupted'] == '0' and receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1,
         'session_failure')


def check(folder=HERE):
    pins = source_contract(); receipt, worker, data = old.read_capture(folder)
    raw = js(Path(receipt['raw_receipt_local']).read_bytes())
    need(set(receipt)-{'raw_receipt_local', 'original_receipt_sha256', 'preserved_failure'} <= set(raw),
         'receipt_raw_fields')
    need(receipt['commit'] == SOURCE and receipt['results_sha256'] == pins['archive_sha256'] and
         receipt['plan_sha256'] == pins['files']['bench/plans/catalogue_adaptive_g4.json']['sha256'] and
         receipt['worker_plan_sha256'] == worker['plan_sha256'], 'capture_identity')
    need((Path(receipt['raw_receipt_local']).parent/'DONE').read_text().strip() == '3' and
         all(receipt[k] is True for k in ('private_key_deleted', 'oslogin_key_removed', 'reserve_released')) and
         receipt['warnings'] == [] and receipt['preserved_failure'] is True, 'closed_failure')
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
                matrix_failed=sum(c[2] for c in counts.values()), matrix_no_closed_result=0,
                asan18_selected=178, asan18_passed=176, asan18_failed=2,
                compiler_issue='memo_fault.cpp:31 misleading-indentation',
                distinct_native_tests_not_launched=sorted(FAILED), native_test_launch_failures=14,
                completed_mutation_campaign_gates=6, tower_mutants_judged=0,
                adaptive_attempts=0, semantic_reuse_attempts=0, memo_measurements=0)


if __name__ == '__main__':
    try: print(json.dumps(check(), sort_keys=True))
    except (old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError, OSError,
            old.foundation.ET.ParseError, tarfile.TarError) as error:
        print('REFUS '+(str(error) if isinstance(error, old.foundation.Refusal) else type(error).__name__))
        raise SystemExit(1)
