#!/usr/bin/env python3
"""Capture LIVE normal/-O readbacks and causal receipt-reader mutations."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--kind', choices=('qualification', 'measure'), required=True)
    args = parser.parse_args()
    script = HERE / ('run.py' if args.kind == 'qualification' else 'measure.py')
    spec = importlib.util.spec_from_file_location('real_checked_' + args.kind, script)
    reader = importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
    directory = args.capture.resolve();target = directory / 'checks';target.mkdir(exist_ok=False)
    before = reader.sha(Path(__file__))
    commands = []
    for name, flags in (('normal', ['-B']), ('optimized', ['-B', '-O'])):
        argv = [sys.executable, *flags, str(script), '--readback', str(directory)]
        result = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        for stream in ('stdout', 'stderr'):
            (target / (name + '.' + stream)).write_bytes(getattr(result, stream))
        commands.append(dict(name=name, argv=argv, exit_code=result.returncode,
                             stdout_sha256=reader.sha(target / (name + '.stdout')),
                             stderr_sha256=reader.sha(target / (name + '.stderr'))))
        reader.need(result.returncode == 0, 'readback ' + name)
    reader.need(reader.read(target / 'normal.stdout') == reader.read(target / 'optimized.stdout'), 'normal/-O identity')
    original = reader.read;pristine = original(directory / 'capture.json');mutants = []
    for name in ('missing_command', 'wrong_argv', 'wrong_exit', 'open_group',
                 'wrong_source', 'wrong_cases', 'false_gpu', 'unclosed_capture'):
        bad = copy.deepcopy(pristine)
        if name == 'missing_command':
            bad['commands'].pop()
        elif name == 'wrong_argv':
            bad['commands'][0]['argv'] = ['true']
        elif name == 'wrong_exit':
            bad['commands'][0]['exit_code'] = 1
        elif name == 'open_group':
            bad['commands'][0]['group_closed'] = False
        elif name == 'wrong_source':
            bad['pins_after'][next(iter(bad['pins_after']))] = '0'*64
        elif name == 'wrong_cases':
            bad['cases'] = [{'forged_case': True}]
        elif name == 'false_gpu':
            bad['GCP_used'] = True
        else:
            bad['status'] = 'running'
        def altered(path):
            return bad if Path(path) == directory / 'capture.json' else original(path)
        cause = None
        with patch.object(reader, 'read', altered):
            try:
                reader.readback(directory)
            except ValueError as error:
                cause = str(error)
        reader.need(cause is not None, 'surviving reader mutant ' + name)
        mutants.append(dict(name=name, rejected=True, cause=cause))
    reader.need(reader.sha(Path(__file__)) == before, 'checker source closure')
    result = dict(status='PASS', GCP_used=False, kind=args.kind, commands=commands, mutations=mutants,
                  reader_sha256=reader.sha(script), checker_sha256=before)
    reader.save(target / 'checks.json', result);print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
