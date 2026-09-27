#!/usr/bin/env python3
"""Normal/-O LIVE readers plus eight causal in-memory capture corruptions.

Explicit protocol adaptation of the pinned bands checker; the gate-only
capture has no case[0], so wrong_input becomes a forged nonempty case list.
"""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('waves_capture_checked', HERE / 'run.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def main():
    runner.need(len(sys.argv) == 2, 'capture directory required')
    directory = Path(sys.argv[1]).resolve()
    target = directory / 'checks'
    target.mkdir(exist_ok=False)
    before = runner.sha(Path(__file__))
    commands = []
    for name, flags in (('normal', ['-B']), ('optimized', ['-B', '-O'])):
        argv = [sys.executable, *flags, str(HERE / 'run.py'), '--readback', str(directory)]
        result = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        for stream in ('stdout', 'stderr'):
            (target / (name + '.' + stream)).write_bytes(getattr(result, stream))
        commands.append(dict(name=name, argv=argv, exit_code=result.returncode,
                             stdout_sha256=runner.sha(target / (name + '.stdout')),
                             stderr_sha256=runner.sha(target / (name + '.stderr'))))
        runner.need(result.returncode == 0, 'LIVE readback ' + name)
    runner.need(runner.read(target / 'normal.stdout') == runner.read(target / 'optimized.stdout'), 'normal/-O identity')
    original = runner.read
    pristine = original(directory / 'capture.json')
    mutations = []
    for name in ('missing_command', 'wrong_argv', 'wrong_exit', 'open_group',
                 'wrong_source', 'false_case', 'false_gpu', 'unclosed_capture'):
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
        elif name == 'false_case':
            bad['cases'] = [{'claimed': 'unmeasured'}]
        elif name == 'false_gpu':
            bad['GCP_used'] = True
        else:
            bad['status'] = 'running'
        def read(path):
            return bad if Path(path) == directory / 'capture.json' else original(path)
        cause = None
        with patch.object(runner, 'read', read):
            try:
                runner.readback(directory)
            except ValueError as error:
                cause = str(error)
        runner.need(cause is not None, 'surviving reader mutant ' + name)
        mutations.append(dict(name=name, rejected=True, cause=cause))
    runner.need(runner.sha(Path(__file__)) == before, 'checker source closure')
    result = dict(status='PASS', commands=commands, mutations=mutations, GCP_used=False,
                  runner_sha256=runner.sha(HERE / 'run.py'), checker_sha256=before)
    runner.save(target / 'checks.json', result)
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
