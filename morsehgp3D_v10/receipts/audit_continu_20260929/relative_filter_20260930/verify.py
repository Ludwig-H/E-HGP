"""Read-only replay of closed relative-filter proofs; not a native engine test."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


judge = module('relative_filter_reader_judge', ROOT/'independent'/'judge.py')


def read(path):
    return judge.load(path.read_text())


def manifest(folder):
    entries = set()
    for line in (folder/'SHA256SUMS').read_text().splitlines():
        sha, name = line.split('  ', 1)
        require(len(sha) == 64 and all(c in '0123456789abcdef' for c in sha), 'digest schema')
        path = folder/name
        require(not Path(name).is_absolute() and '..' not in Path(name).parts and
                name not in entries and path.is_file() and not path.is_symlink(), 'manifest path')
        require(digest(path) == sha, 'SHA mismatch: '+name)
        entries.add(name)
    actual = {str(path.relative_to(folder)) for path in folder.rglob('*')
              if path.is_file() and path != folder/'SHA256SUMS'}
    require(actual == entries, 'incomplete manifest')
    return len(entries)


def independent():
    folder = ROOT/'independent'
    inputs = module('relative_filter_reader_inputs', folder/'inputs.py')
    requests = read(folder/'requests.json')
    require(requests == inputs.panel() and len(requests) == 3600, 'regenerated requests differ')
    require((folder/'requests.stdin').read_text() ==
            '\n'.join(map(inputs.line, requests))+'\n', 'serialized panel differs')
    require(read(folder/'inputs_v1_domain_review.json')['outside'] and
            not read(folder/'inputs_domain_review.json')['outside'], 'input preflight history')
    records = [('execution.json', 'judge_v1.py', 'judge_'),
               ('judge_r2_execution.json', 'judge_r2.py', 'judge_r2_'),
               ('judge_r3_execution.json', 'judge.py', 'judge_r3_')]
    prototype = read(ROOT/'prototype'/'receipt.json')
    binary_shas = {Path(path).name: sha for path, sha in
                   prototype['provenance']['binary_hashes_after_relocation'].items()}
    expected_failures = {'drop_rest': 'conversion excludes true integer at 315',
                         'fixed_margin': 'unsafe interior at 2687'}
    judgments = 0
    native = 0
    for record_name, judge_name, prefix in records:
        record = read(folder/record_name)
        require(record['status'] == 'PASS' and record['GCP_used'] is False and
                record['before'] == record['after'], 'execution/source closure')
        for raw, sha in record['before'].items():
            name = Path(raw).name
            if name in binary_shas:
                require(sha == binary_shas[name], 'native binary provenance')
            else:
                archive = ROOT/'prototype'/name if name == 'relative_filter.cpp' else folder/name
                if name == 'judge.py':
                    archive = folder/judge_name
                require(digest(archive) == sha, 'record source/output SHA: '+name)
        commands = {item['name']: item for item in record['commands']}
        require(len(commands) == len(record['commands']), 'duplicate command')
        current = module('relative_filter_reader_'+judge_name.replace('.', '_'), folder/judge_name)
        expected_names = set()
        for case in ('normal', 'ubsan', 'drop_rest', 'fixed_margin'):
            require((folder/(case+'.stderr')).read_text() == '', 'native diagnostic')
            if record_name == 'execution.json':
                expected_names.add(case)
                require(commands[case]['exit_code'] == 0, 'native invocation failed')
                native += 1
            answers = [current.load(line) for line in (folder/(case+'.stdout')).read_text().splitlines()]
            try:
                result = current.judge(requests, answers)
                rc = 0
            except ValueError as error:
                result = dict(status='FAIL', error=str(error))
                rc = 1
            if case in expected_failures:
                require(rc == 1 and result['error'] == expected_failures[case], 'mutant causal refusal')
            else:
                require(rc == 0 and result['counts']['contacts'] == 196 and
                        result['counts']['translations'] == 40 and result['exact_rank_checks'] == 147,
                        'positive/nonvacuous gate')
            for mode in ('normal', 'optimized'):
                name = prefix+case+'_'+mode
                expected_names.add(name)
                require(commands[name]['exit_code'] == rc and read(folder/(name+'.stdout')) == result and
                        (folder/(name+'.stderr')).read_text() == '', 'archived judgment differs')
                judgments += 1
        require(set(commands) == expected_names, 'missing/extra paired commands')
    require((folder/'normal.stdout').read_bytes() == (folder/'ubsan.stdout').read_bytes(),
            'normal/UBSan differential')
    r3 = read(folder/'judge_r3_execution.json')
    require(r3['native_executions'] == 0 and len(r3['probes']) == 7 and
            all(row['refused'] is True for row in r3['probes']), 'R3 refusal probes')
    for token in ('NaN', 'Infinity', '-Infinity', '1e999', '-1e999'):
        try:
            judge.load('{"ignored":{"nested":'+token+'}}')
        except ValueError:
            pass
        else:
            raise ValueError('nonfinite JSON probe survived')
    return dict(unique_requests=3600, native_invocations=native,
                archived_judgments=judgments, contacts=196, translations=40,
                static_rank_checks=147, mutant_failures=expected_failures)


def prototype():
    folder = ROOT/'prototype'
    receipt = read(folder/'receipt.json')
    require(receipt['gcp_used'] is False and receipt['full'] is False and
            receipt['production_exact_fallback'] is False and receipt['repository_edited'] is False,
            'prototype scope')
    runs = read(folder/'small_runs.json')
    require(runs['native_calls'] == 3 and len(runs['executions']) == 3, 'small invocations')
    for call in runs['executions']:
        label = 'smoke' if call['mode'] == 'smoke' else call['mode']
        require(call['exit_code'] == 0 and call['stderr'] == '' and
                (folder/(label+'_small.stdout.jsonl')).read_text() == call['stdout'] and
                (folder/(label+'_small.stderr.txt')).read_text() == '', 'small closed capture')
    smoke = read(folder/'smoke_small.stdout.jsonl')
    require(smoke['checks'] == 50 and smoke['failures'] == 0, 'FENV smoke')
    return dict(native_invocations=3, rows_per_backend=27, smoke_checks=50)


def adversarial():
    folder = ROOT/'judge_adversarial_r1'
    record = read(folder/'receipt.json')
    require(record['native_programs_executed'] == 0 and record['GCP_used'] is False and
            record['original_sources_edited'] is False, 'adversarial scope')
    for name, sha in record['original_source_and_outputs_sha256'].items():
        target = ROOT/'independent'/('judge_v1.py' if name == 'judge.py' else name)
        require(digest(target) == sha, 'adversarial fixture identity')
    left, right = (read(folder/(mode+'.output')) for mode in ('normal', 'optimized'))
    require(left.pop('optimize_flag') == 0 and right.pop('optimize_flag') == 1 and left == right,
            'adversarial normal/-O differs')
    require(left['status'] == 'PASS' and left['cases'] == 56 and
            left['observed_rejections'] == left['expected_rejections'] == 53 and
            left['duplicate_json_key_refused'] is True and not left['failures'], 'adversarial gate')
    outputs = [row for row in left['results'] if row['scope'] == 'answer']
    require(len(outputs) == 48 and all(row['rejected'] is True for row in outputs), 'forged outputs')
    return dict(output_falsifications=48, r1_scoped_tolerances=3,
                replay='checks recorded adversarial outcomes, not an extra native run')


if __name__ == '__main__':
    inner = sys.argv[1:] == ['--inner-only']
    require(inner or not sys.argv[1:], 'unsupported arguments')
    manifests = {name: manifest(ROOT/name) for name in
                 ('prototype', 'independent', 'judge_adversarial_r1')}
    if not inner:
        manifests['root'] = manifest(ROOT)
    assembly = read(ROOT/'assembly.json')
    for folder, entries in assembly['copied_sha256'].items():
        for name, sha in entries.items():
            require(digest(ROOT/folder/name) == sha, 'assembly changed bytes')
    print(json.dumps(dict(status='PASS', scope='closed proof replay, not engine qualification',
                         manifests=manifests, independent=independent(), prototype=prototype(),
                         adversarial_r1=adversarial(), GCP_used=False), sort_keys=True))
