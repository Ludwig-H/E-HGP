#!/usr/bin/env python3
"""Read only: byte closure, exact diagnostic delta, four local test receipts."""
import difflib
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent
OLD = '043197e4c73d92ff8845ddb5836c11fbb9ed9bdbd9a609edf7ce4a946d2c0fb3'
NEW = 'aaf8bcc1f703fbe5f5f02258bf5d82610a20720ed481aa1397385708aaaa7ec9'
PARENT = '8758776c41abbbe08009a55c6e97bc10f7d040ccaadb4d5bcc5a6d93d5bf16b2'
OLD_BLOCK = """        nvcc = shutil.which('nvcc') or next((p for p in CUDA_PATHS if Path(p).is_file() and os.access(p, os.X_OK)), None)
        compiler, smi = shutil.which('g++'), shutil.which('nvidia-smi')
        need(nvcc and compiler and smi and Path('/usr/bin/time').is_file(), 'existing tools required, no installation')
        need(Path(compiler).resolve() == Path('/usr/bin/g++').resolve(), 'strict adapter g++ binding')
"""
NEW_BLOCK = """        result['tool_discovery'] = discover_existing_tools(
            which=shutil.which, is_file=lambda path: Path(path).is_file(),
            executable=lambda path: os.access(path, os.X_OK), resolve=lambda path: Path(path).resolve())
        nvcc, compiler, smi = require_existing_tools(result['tool_discovery'])
"""


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def unique(pairs):
    value = {}
    for key, item in pairs:
        need(key not in value, 'duplicate JSON key')
        value[key] = item
    return value


def decode(raw):
    def invalid(_value):
        raise ValueError('nonfinite JSON constant')
    return json.loads(raw, object_pairs_hook=unique, parse_constant=invalid)


def main():
    def raw(name):
        return (ROOT / name).read_bytes()

    def obj(name):
        return decode(raw(name))

    manifest = obj('manifest.json')
    need(set(manifest) == {'schema', 'files'} and manifest['schema'] == 'mhgp7-terminal-tool-discovery-v1', 'schema')
    paths = list(ROOT.rglob('*'))
    need(not any(path.is_symlink() for path in paths), 'no symlink')
    need({path.relative_to(ROOT).as_posix() for path in paths if path.is_file()} == set(manifest['files']) | {'manifest.json'},
         'closed inventory')
    for name, pin in manifest['files'].items():
        path = Path(name)
        need(type(name) is str and not path.is_absolute() and '..' not in path.parts and str(path) == name, 'safe path')
        need(set(pin) == {'sha256', 'bytes'} and type(pin['bytes']) is int and pin['bytes'] >= 0
             and re.fullmatch('[0-9a-f]{64}', pin['sha256']), 'file pin format')
        data = raw(name)
        need(len(data) == pin['bytes'] and sha(data) == pin['sha256'] and not data.startswith(b'\x7fELF'), 'exact non-ELF bytes')
    old, new = raw('origins/worker_043197.py.source'), raw('sources/gcp-migration/terminal_batch_worker_v7.py')
    need(sha(old) == OLD and sha(new) == NEW, 'two worker identities')
    prior = obj('origins/provenance.json')
    need(prior == dict(parent_manifest_sha256=PARENT, logical_worker='snapshot/worker.py', worker_sha256=OLD,
                       old_snapshots_unchanged=True, new_GPU_snapshot_created=False), 'historical attribution')
    old_text, new_text = old.decode(), new.decode()
    old_prefix, old_main = old_text.split('def main():', 1)
    new_prefix, new_main = new_text.split('def main():', 1)
    need(old_prefix == new_prefix.split('def discover_existing_tools(', 1)[0], 'no old prefix changed')
    need(old_main.count(OLD_BLOCK) == 1 and new_main == old_main.replace(OLD_BLOCK, NEW_BLOCK),
         'only tool preflight main delta; observation precedes refusal')
    diff = ''.join(difflib.unified_diff(old_text.splitlines(keepends=True), new_text.splitlines(keepends=True),
                                      fromfile='worker_043197.py', tofile='worker_aaf8bcc1.py'))
    need(raw('worker.diff').decode() == diff, 'exact human-readable diff')
    receipt = obj('capture/receipt.json')
    need(receipt['status'] == 'passed' and receipt['source_stable'] is True and receipt['GCP_used'] is False
         and receipt['compiled'] is False and receipt['new_GPU_snapshot_created'] is False, 'qualification scope')
    before, after = obj('capture/sources_before.json'), obj('capture/sources_after.json')
    need(before == after and set(before) == set(receipt['source_mapping']), 'source stability inventory')
    for original, pin in before.items():
        need(sha(raw(receipt['source_mapping'][original])) == pin, 'exact captured source')
    names = ['pure_normal', 'pure_optimized', 'mock_normal', 'mock_optimized']
    need([row['name'] for row in receipt['commands']] == names, 'four commands')
    for row in receipt['commands']:
        name = row['name']
        need(row == obj('capture/' + name + '.command.json') and type(row['exit_code']) is int and row['exit_code'] == 0,
             'command passed')
        test = 'selftest_terminal_batch_worker_v7.py' if name.startswith('pure') else 'selftest_terminal_batch_tool_preflight_v7.py'
        need(row['argv'][:-1] == ['python3', '-B'] + (['-O'] if name.endswith('optimized') else [])
             and row['argv'][-1].endswith('/gcp-migration/' + test), 'exact test/interpreter command')
        for stream in ('stdout', 'stderr'):
            need(sha(raw('capture/' + name + '.' + stream)) == row[stream + '_sha256'], 'stream pin')
        need(not raw('capture/' + name + '.stderr'), 'empty stderr')
    need(raw('capture/pure_normal.stdout') == raw('capture/pure_optimized.stdout')
         and raw('capture/mock_normal.stdout') == raw('capture/mock_optimized.stdout'), 'normal/-O output identity')
    need(obj('capture/pure_normal.stdout') == dict(status='passed', checks=44, rejections=239, synthetic_only=True,
         tool_admission_combinations=512, subprocess_invoked=False, device_executed=False, GCP_used=False), 'pure nonvacuity')
    mocked = obj('capture/mock_normal.stdout')
    need(mocked['status'] == 'passed' and mocked['GCP_used'] is False and mocked['actual_commands'] == 0
         and mocked['all_guards_and_metadata_mocked'] is True and mocked['mocked_worker_runs'] == 5
         and mocked['scope'] == 'diagnostic_receipt_only_no_lifecycle_qualification', 'mock scope')
    need([case['absent'] for case in mocked['cases']] == ['nvcc', 'g++', 'nvidia-smi', '/usr/bin/time', 'g++_binding'],
         'five causal failures')
    for case in mocked['cases']:
        need(type(case['exit_code']) is int and case['exit_code'] == 1 and case['commands'] == [], 'no work after missing tool')
        expected = 'ValueError: existing tools required, no installation; missing: ' + case['absent']
        if case['absent'] == 'g++_binding':
            expected = 'ValueError: strict adapter g++ binding: selected=/mock/different-g++, resolved=/mock/different-g++, required=/usr/bin/g++, error=None'
        need(case['error'] == expected and case['tool_discovery']['missing'] ==
             ([] if case['absent'] == 'g++_binding' else [case['absent']]), 'retained precise refusal')
    print(json.dumps(dict(status='passed', files=len(manifest['files']), commands=4, admission_combinations=512,
                         mocked_preflights=5, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
