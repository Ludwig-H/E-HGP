#!/usr/bin/env python3
"""Offline reader checks, ported from frozen event selftest 2a9a52b6.
No compilation or geometry.
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
RUN_SHA = '78816029de56200d8b09526d1afeb931708e71154aad25206ee39983ff2313d1'
PORT_FILE = HERE.parent / 'b_full_a_events_20260927/selftest.py'
PORT_SHA = '2a9a52b6eb852fbf9438e4662ead847aadc5f14a0891a24baae4f3ec10c08105'
if hashlib.sha256(PORT_FILE.read_bytes()).hexdigest() != PORT_SHA:
    raise RuntimeError('selftest source attribution pin')
if hashlib.sha256((HERE / 'run.py').read_bytes()).hexdigest() != RUN_SHA:
    raise RuntimeError('reader pin before import')
spec = importlib.util.spec_from_file_location('a_minimum_reader', HERE / 'run.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    args = parser.parse_args()
    receipt = args.receipt.resolve()
    golden = runner.readback(receipt)
    rejected = []

    def reject(name, change):
        with tempfile.TemporaryDirectory(prefix='mhgp9-a-minimum-reader-') as tmp:
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
            raise RuntimeError('reader accepted mutation ' + name)

    def field(name, key, value):
        reject(name, lambda _, state: state.__setitem__(key, value))

    field('status', 'status', 'failed')
    field('GCP', 'GCP_used', True)
    field('schema', 'schema', 'wrong')
    reject('binary_build_unbound', lambda _, s: s['binaries'].__setitem__('release', s['binaries']['sanitize']))
    reject('extra_build', lambda _, s: s['builds'].__setitem__('other', s['builds']['release']))
    reject('missing_dependency', lambda _, s: (s['dependency_pins_before'].pop(next(iter(s['dependency_pins_before']))),
                                              s.__setitem__('dependency_pins_after', dict(s['dependency_pins_before']))))
    reject('source_closure', lambda _, s: s['source_pins_after'].__setitem__(next(iter(s['source_pins_after'])), '0'*64))
    reject('build_closure', lambda _, s: s['build_pins'].__setitem__(next(iter(s['build_pins'])), '0'*64))
    reject('command_missing', lambda _, s: s['commands'].pop())
    reject('command_argument', lambda _, s: s['commands'][0]['argv'].append('--wrong'))
    reject('open_group', lambda _, s: s['commands'][0].__setitem__('group_closed', False))
    reject('chronology', lambda _, s: s['commands'][1].__setitem__('started_epoch', 0))
    reject('stdout_hash', lambda d, _: (d / 'gate_release.stdout').write_text('{}\n'))

    def gate_field(key, value):
        def change(directory, state):
            name = 'gate_release'
            p = directory / (name + '.stdout')
            gate = runner.read(p)
            gate[key] = value
            runner.save(p, gate)
            command = next(c for c in state['commands'] if c['name'] == name)
            command['stdout_sha256'] = runner.sha(p)
            runner.save(directory / (name + '.command.json'), command)
        reject('semantic_' + key, change)

    gate_field('mutations', 0)
    gate_field('silent_groups', 0)
    gate_field('continuations', False)
    gate_field('max_parents', 31)
    gate_field('ancestor_queries', golden['gate']['ancestor_queries'] + 1)
    gate_field('vertices', 1 << 64)
    gate_field('parallel', True)
    gate_field('FULL_executed_by_candidate', True)
    gate_field('status', 'failed')
    gate_field('full_gate', False)
    gate_field('groups', False)
    gate_field('max_combined_capacity_observed_bytes', True)
    gate_field('history_entries', golden['gate']['history_entries'] + 1)
    gate_field('omitted_history_groups', 0)
    gate_field('cut_queries', 0)
    gate_field('ancestor_entries', golden['gate']['ancestor_entries'] + 1)
    runner.need(len(rejected) == 29, 'selftest inventory')
    print(json.dumps(dict(status='PASS', mutations=len(rejected), names=rejected,
                         reader_sha256=RUN_SHA, geometry_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
