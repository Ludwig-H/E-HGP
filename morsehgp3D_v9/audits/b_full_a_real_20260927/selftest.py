#!/usr/bin/env python3
"""Offline reader corruption tests; explicit min-label selftest port.
No compiler, geometry or cloud is executed here.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
RUN_SHA = '84280bf3405b92df64d808fb6d29cfb44d06549c25a45dbbf0834646e5e78f69'
PORT = HERE.parent / 'b_full_a_min_label_20260927/selftest.py'
PORT_SHA = '4a7ec1d2618a4795a50adb3739fabdb09a87b8b15294ec81b88d1b81068558ce'
if hashlib.sha256(PORT.read_bytes()).hexdigest() != PORT_SHA:
    raise RuntimeError('selftest attribution pin')
if hashlib.sha256((HERE / 'run.py').read_bytes()).hexdigest() != RUN_SHA:
    raise RuntimeError('reader pin before import')
spec = importlib.util.spec_from_file_location('a_real_reader', HERE / 'run.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    args = parser.parse_args()
    receipt = args.receipt.resolve()
    golden = runner.readback(receipt)
    runner.need(golden['scope'] == 'qualification', 'qualification reader corpus')
    rejected = []

    def reject(name, change):
        with tempfile.TemporaryDirectory(prefix='mhgp9-a-real-reader-') as tmp:
            directory = Path(tmp) / 'receipt'
            shutil.copytree(receipt, directory)
            state = runner.read(directory / 'capture.json')
            change(directory, state)
            runner.save(directory / 'capture.json', state)
            try:
                runner.readback(directory)
            except RuntimeError:
                rejected.append(name)
                return
            raise RuntimeError('reader accepted ' + name)

    for key, value in [('status', 'failed'), ('GCP_used', True), ('schema', 'wrong')]:
        reject(key, lambda _, s, k=key, v=value: s.__setitem__(k, v))
    reject('unbound_binary', lambda _, s: s['binaries'].__setitem__('release', s['binaries']['sanitize']))
    reject('extra_build', lambda _, s: s['builds'].__setitem__('other', s['builds']['release']))
    reject('missing_dependency', lambda _, s: (s['dependency_pins_before'].pop(next(iter(s['dependency_pins_before']))),
        s.__setitem__('dependency_pins_after', dict(s['dependency_pins_before']))))
    reject('source_closure', lambda _, s: s['source_pins_after'].__setitem__(next(iter(s['source_pins_after'])), '0'*64))
    reject('archive_closure', lambda _, s: s['build_pins'].__setitem__(next(p for p in s['build_pins'] if p.endswith('.a')), '0'*64))
    reject('object_closure', lambda _, s: s['build_pins'].__setitem__(next(p for p in s['build_pins'] if p.endswith('.o')), '0'*64))
    reject('compiler_plan_closure', lambda _, s: s['compile_plan_pins_after'].__setitem__(next(iter(s['compile_plan_pins_after'])), '0'*64))
    reject('missing_command', lambda _, s: s['commands'].pop())
    reject('changed_argv', lambda _, s: s['commands'][0]['argv'].append('--wrong'))
    reject('open_group', lambda _, s: s['commands'][0].__setitem__('group_closed', False))
    reject('chronology', lambda _, s: s['commands'][1].__setitem__('started_epoch', 0))
    reject('stdout_hash', lambda d, _: (d / 'gate_release.stdout').write_text('{}\n'))

    def semantic(name, command_name, change):
        def mutate(directory, state):
            path = directory / (command_name + '.stdout')
            value = runner.read(path)
            change(value)
            runner.save(path, value)
            command = next(c for c in state['commands'] if c['name'] == command_name)
            command['stdout_sha256'] = runner.sha(path)
            runner.save(directory / (command_name + '.command.json'), command)
        reject(name, mutate)

    for key, value in [('cases', 5), ('orders', True), ('continuations', 0), ('extra_blocks', 0), ('FULL_executed_by_candidate', True)]:
        semantic('gate_' + key, 'gate_release', lambda d, k=key, v=value: d.__setitem__(k, v))
    for key, value in [('n', 63), ('input_hash_u64', 0), ('sealed_catalogue', True), ('candidate_parallel', True),
                       ('FULL_executed_by_candidate', True), ('capture_capacity_bytes_initial', 1)]:
        semantic('smoke_' + key, 'smoke_release', lambda d, k=key, v=value: d.__setitem__(k, v))
    semantic('unknown_scope_field', 'smoke_release', lambda d: d.__setitem__('new_claim', 1))
    semantic('missing_order', 'smoke_release', lambda d: d['rows'].pop())
    semantic('work_bool', 'smoke_release', lambda d: d['rows'][0]['minimum_work'].__setitem__('V', True))
    semantic('history_identity', 'smoke_release', lambda d: d['rows'][0]['minimum_work'].__setitem__('history_entries', 0))
    semantic('wrong_parent_domain', 'smoke_release', lambda d: d['rows'][0]['minimum_work'].__setitem__('P_draft', 1 << 64))
    semantic('missing_work', 'smoke_release', lambda d: d['rows'][0]['minimum_work'].pop('dsu_steps'))
    semantic('negative_time', 'smoke_release', lambda d: d['rows'][0]['minimum_wall_ms'].__setitem__(0, -1))
    semantic('phase_exceeds_call', 'smoke_release', lambda d: d['rows'][0]['minimum_times'][0].__setitem__('total_ms', 1e20))
    semantic('false_whole_time', 'smoke_release', lambda d: d['times_ms'].__setitem__('process', 0))
    runner.need(len(rejected) == 35, 'selftest inventory')
    print(json.dumps(dict(status='PASS', rejected=len(rejected), names=rejected, reader_sha256=RUN_SHA,
                         geometry_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
