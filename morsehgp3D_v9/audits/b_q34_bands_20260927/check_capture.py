#!/usr/bin/env python3
"""Record normal/-O LIVE readbacks and causal in-memory receipt mutations."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('bands_capture_checked', HERE / 'run.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def main():
    runner.need(len(sys.argv) == 2, 'capture directory required')
    directory = Path(sys.argv[1]).resolve()
    target = directory / 'checks'
    target.mkdir(exist_ok=False)
    rows = []
    for name, flags in (('normal', ['-B']), ('optimized', ['-B', '-O'])):
        argv = [sys.executable, *flags, str(HERE / 'run.py'), '--readback', str(directory)]
        result = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        for stream in ('stdout', 'stderr'):
            (target / (name + '.' + stream)).write_bytes(getattr(result, stream))
        row = dict(name=name, argv=argv, exit_code=result.returncode,
                   stdout_sha256=runner.sha(target / (name + '.stdout')),
                   stderr_sha256=runner.sha(target / (name + '.stderr')))
        rows.append(row)
        runner.need(result.returncode == 0, 'readback ' + name)
    runner.need(runner.read(target / 'normal.stdout') == runner.read(target / 'optimized.stdout'),
                'normal/-O readback equality')
    pristine = runner.read(directory / 'capture.json')
    original_read = runner.read
    mutations = []
    for name in ('missing_command', 'wrong_argv', 'wrong_exit', 'open_group',
                 'wrong_source', 'wrong_input', 'false_gpu', 'unclosed_capture'):
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
        elif name == 'wrong_input':
            bad['cases'][0]['input_hash'] = '0'*16
        elif name == 'false_gpu':
            bad['GCP_used'] = True
        elif name == 'unclosed_capture':
            bad['status'] = 'running'

        def altered_read(path):
            return bad if Path(path) == directory / 'capture.json' else original_read(path)

        killed = None
        with patch.object(runner, 'read', altered_read):
            try:
                runner.readback(directory)
            except ValueError as error:
                killed = str(error)
        runner.need(killed is not None, 'reader mutant survived: ' + name)
        mutations.append(dict(name=name, rejected=True, cause=killed))
    out = dict(status='PASS', GCP_used=False, commands=rows, mutations=mutations,
               runner_sha256=runner.sha(HERE / 'run.py'), self_sha256=runner.sha(Path(__file__)))
    runner.save(target / 'checks.json', out)
    print(json.dumps(out, sort_keys=True))


if __name__ == '__main__':
    main()
