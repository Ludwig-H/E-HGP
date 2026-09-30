"""Guarded reader R2: verify closed bytes and nonempty pins BEFORE imports."""
from copy import deepcopy
import hashlib
import importlib.util
import json
from math import isfinite
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
PROOF = ROOT.parent/'relative_filter_20260930'
PROOF_MANIFEST_SHA = '848f31c4fd74228a2978ffbbdade9a917275c0873b700d5d062472b81e311ec3'
PIN_SETS = {
    'execution.json': {'normal_v2', 'ubsan_v2', 'drop_rest', 'fixed_margin',
                       'relative_filter.cpp', 'inputs.py', 'inputs_v1.py', 'judge.py', 'record.py'},
    'judge_r2_execution.json': {'judge.py', 'judge_v1.py', 'record_judge_r2.py', 'requests.json',
                                'normal.stdout', 'ubsan.stdout', 'drop_rest.stdout', 'fixed_margin.stdout'},
    'judge_r3_execution.json': {'judge.py', 'judge_v1.py', 'judge_r2.py', 'record_judge_r3.py',
                                'requests.json', 'normal.stdout', 'ubsan.stdout',
                                'drop_rest.stdout', 'fixed_margin.stdout'},
}


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def manifest(folder):
    names = set()
    for line in (folder/'SHA256SUMS').read_text().splitlines():
        expected, name = line.split('  ', 1)
        path = folder/name
        require(len(expected) == 64 and all(c in '0123456789abcdef' for c in expected), 'SHA schema')
        require(not Path(name).is_absolute() and '..' not in Path(name).parts and
                name not in names and path.is_file() and not path.is_symlink(), 'manifest path')
        require(sha(path) == expected, 'SHA mismatch: '+name)
        names.add(name)
    require(names == {str(path.relative_to(folder)) for path in folder.rglob('*')
                      if path.is_file() and path != folder/'SHA256SUMS'}, 'incomplete manifest')
    return len(names)


def read(path):
    def unique(pairs):
        answer = {}
        for key, value in pairs:
            require(key not in answer, 'duplicate JSON key')
            answer[key] = value
        return answer
    def constant(value):
        raise ValueError('nonfinite JSON constant')
    def finite(value):
        result = float(value)
        require(isfinite(result), 'nonfinite JSON float')
        return result
    return json.loads(path.read_text(), object_pairs_hook=unique,
                      parse_constant=constant, parse_float=finite)


def pins(record, name):
    require(record['status'] == 'PASS' and record['GCP_used'] is False and
            type(record['before']) is dict and record['before'] == record['after'], 'record closure')
    names = [Path(path).name for path in record['before']]
    require(len(names) == len(set(names)) and set(names) == PIN_SETS[name], 'required pins')
    if name != 'execution.json':
        require(type(record['native_executions']) is int and record['native_executions'] == 0,
                'rejudgment executed native code')


if __name__ == '__main__':
    inner = sys.argv[1:] == ['--inner-only']
    require(inner or not sys.argv[1:], 'unsupported arguments')
    if not inner:
        manifest(ROOT)
    require(sha(PROOF/'SHA256SUMS') == PROOF_MANIFEST_SHA, 'different original closure')
    original_files = manifest(PROOF)
    records = {name: read(PROOF/'independent'/name) for name in PIN_SETS}
    for name, record in records.items():
        pins(record, name)
    # These are structural guard controls, not altered native captures.
    controls = 0
    for name, record in records.items():
        empty = deepcopy(record)
        empty['before'] = empty['after'] = {}
        try:
            pins(empty, name)
        except ValueError as error:
            require(str(error) == 'required pins', 'wrong empty-pin refusal cause')
            controls += 1
        else:
            raise ValueError('empty pins survived')
    for name in ('judge_r2_execution.json', 'judge_r3_execution.json'):
        altered = deepcopy(records[name])
        altered['native_executions'] = 1
        try:
            pins(altered, name)
        except ValueError as error:
            require(str(error) == 'rejudgment executed native code', 'wrong native-scope cause')
            controls += 1
        else:
            raise ValueError('native-scope alteration survived')
    # No module from the proof packet is executed until ALL checks above pass.
    spec = importlib.util.spec_from_file_location('closed_relative_filter_reader_r1', PROOF/'verify.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    result = dict(status='PASS', reader_revision=2, original_files=original_files,
                  structural_guard_refusals=controls, independent=reader.independent(),
                  prototype=reader.prototype(), adversarial_r1=reader.adversarial(),
                  scope='closed proof replay only; no engine/GPU/performance qualification', GCP_used=False)
    print(json.dumps(result, sort_keys=True))
