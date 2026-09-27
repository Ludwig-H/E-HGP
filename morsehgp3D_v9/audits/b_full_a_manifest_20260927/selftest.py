#!/usr/bin/env python3
"""Offline mutations of the frozen LIVE reader; no compilation or geometry."""
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
RUN_SHA = 'dd36210c8f9a42aeb8a4b597d137745b1ae1f8e9bf6079c7a6bd27fc262d5c61'
if hashlib.sha256((HERE / 'run.py').read_bytes()).hexdigest() != RUN_SHA:
    raise RuntimeError('reader pin before import')
spec = importlib.util.spec_from_file_location('a_manifest_reader', HERE / 'run.py')
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
        with tempfile.TemporaryDirectory(prefix='mhgp9-a-manifest-reader-') as tmp:
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

    gate_field('mutants', [True]*5)
    gate_field('duplicate_roots', 0)
    gate_field('shared_plateau_roots', 0)
    gate_field('late_failure_refused', False)
    gate_field('max_parents', 31)
    gate_field('raw_roots', golden['gate']['raw_roots'] + 1)
    gate_field('status', 'failed')
    runner.need(len(rejected) == 20, 'selftest inventory')
    print(json.dumps(dict(status='PASS', mutations=len(rejected), names=rejected,
                         reader_sha256=RUN_SHA, geometry_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
