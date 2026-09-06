#!/usr/bin/env python3
"""Create-only, bounded rational proof-model capture; no C++ or benchmark."""
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path('/workspaces/E-HGP')
CONTROLLER = ROOT / 'build/v7_meb_probe_20260906_controller/capture.py'
SOURCE_PIN = 'f0b5afcd44ac4e793abf78f131f2a0c8e6ffcf5f6273e5dd5f5d5f1f03a12a7f'
GATE = ROOT / 'morsehgp3D_v7/tests/full_output_lower_bound_gate.py'
GATE_PIN = '01fd40103f89d878e1d89bb08fcc7592f4c684f3f5db9ef70443a2660f7dcb04'


def main():
    spec = importlib.util.spec_from_file_location('inert_meb_capture', CONTROLLER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    ctrl = module.Controller(SOURCE_PIN)
    c = ctrl.c
    module.require(c.sha(GATE) == GATE_PIN and len(sys.argv) == 2, 'gate_or_argv')
    out = c.new_directory('math', sys.argv[1])
    report = dict(status='failed', engine_invoked=False, gate_sha256=GATE_PIN,
                  driver_sha256=c.sha(Path(__file__)), commands={})
    try:
        values = []
        for optimized in (False, True):
            tag = 'optimized' if optimized else 'normal'
            argv = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B']
            if optimized:
                argv.append('-O')
            for argument, code in (('--selftest', 0), ('--unknown', 2)):
                label = tag + '_' + argument[2:]
                record = ctrl.command(out, label, argv + [str(GATE), argument], (code,), 30)
                report['commands'][label] = record
                module.require(record['status'] == 'completed', 'command:' + label)
                value = c.read_json(out / (label + '.stdout'))
                module.require((out / (label + '.stderr')).read_bytes() == b'', 'stderr')
                if code == 0:
                    module.require(value['status'] == 'passed' and len(value['mutants_killed']) == 8
                                   and sum(x['rational_identity_checks'] for x in value['cases']) == 2008,
                                   'nonvacuum')
                    module.require(value.pop('python_optimization') == int(optimized), 'mode')
                    values.append(value)
        module.require(module.encoded(values[0]) == module.encoded(values[1]), 'optimized_disagrees')
        module.require(c.sha(GATE) == GATE_PIN, 'gate_drift')
        report.update(status='completed', gate_before_after_stable=True)
    finally:
        report['artifacts'] = {p.name: c.sha(p) for p in out.iterdir() if p.is_file()}
        c.save(out / 'receipt.json', report)
    module.require(report['status'] == 'completed', 'capture_failed')


if __name__ == '__main__':
    main()
