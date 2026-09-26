#!/usr/bin/env python3
"""Capture complementary reader checks in a fresh directory, no cloud calls."""
import argparse
import json
import os
from pathlib import Path
import signal
import sys

sys.dont_write_bytecode = True
import run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('fresh_output', type=Path)
    args = parser.parse_args()
    directory = args.fresh_output.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    paths = [Path(__file__), run.HERE / 'validate_recipe.py', run.HERE / 'run.py',
             run.HERE / 'inputs.py', args.capture.resolve() / 'capture.json', run.HELPER]
    pins = {str(p): run.sha(p) for p in paths}
    commands = run.command_class()(directory, dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
    state = dict(status='failed', GCP_used=False, pins_before=pins)

    def interrupted(signum, _frame):
        raise InterruptedError('signal ' + str(signum))

    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    try:
        for mode, flags in (('normal', ['-B']), ('optimized', ['-O', '-B'])):
            for action in ('selftest', 'readback'):
                argv = [sys.executable, *flags, str(run.HERE / 'validate_recipe.py'), '--' + action]
                if action == 'readback':
                    argv.append(str(args.capture.resolve()))
                code, stdout, _ = commands.run(action + '_' + mode, argv, timeout=None)
                run.need(code == 0 and json.loads(stdout)['status'] == 'PASS', 'complementary check failed')
        state['status'] = 'completed'
    except BaseException as error:
        state['error'] = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        state['commands'] = commands.rows
        state['pins_after'] = {path: run.sha(path) for path in pins}
        if pins != state['pins_after']:
            state['status'] = 'failed'
        run.save(directory / 'checks.json', state)
    run.need(state['status'] == 'completed', 'checker source closure failed')
    print(json.dumps(dict(status='PASS', commands=len(commands.rows), GCP_used=False)))


if __name__ == '__main__':
    main()
