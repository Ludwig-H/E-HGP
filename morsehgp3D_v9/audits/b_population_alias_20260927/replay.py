#!/usr/bin/env python3
"""Retain the ptrace failure and re-run its exact compiled programs outside it."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time

import run as prior

OUT = prior.RECEIPT / 'outside_sandbox'
need, sha = prior.need, prior.digest


def pins():
    original = json.loads((prior.RECEIPT / 'manifest.json').read_text())
    need(original['status'] == 'failed', 'preserve initial failed capture')
    need(original['commands'][-1]['name'] == 'reproduce_sanitize', 'initial failure identity')
    need('LeakSanitizer does not work under ptrace' in
         (prior.RECEIPT / 'reproduce_sanitize.stderr').read_text(), 'initial ptrace diagnostic')
    result = {path: sha(Path(path)) for path in original['sources_before']}
    need(result == original['sources_before'], 'compiled source changed since first run')
    for path in [*prior.RECEIPT.glob('*.stdout'), *prior.RECEIPT.glob('*.stderr'),
                 prior.RECEIPT / 'manifest.json', Path(__file__),
                 prior.BUILD / 'release', prior.BUILD / 'sanitize']:
        result[str(path)] = sha(path)
    return result


def capture():
    OUT.mkdir(exist_ok=False)
    state = dict(status='running', pins_before=pins(), commands=[], GCP_used=False)
    save = lambda: (OUT / 'capture.json').write_text(json.dumps(state, indent=2) + '\n')
    save()
    try:
        for name, argv, code in prior.recipe()[-3:]:
            start = time.monotonic()
            row = dict(name=name, argv=argv, expected_exit=code, status='running')
            state['commands'].append(row)
            save()
            env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=1')
            with (OUT / (name + '.stdout')).open('wb') as out, (OUT / (name + '.stderr')).open('wb') as err:
                completed = subprocess.run(argv, cwd=prior.REPO, env=env, stdout=out, stderr=err, check=False)
            row.update(exit_code=completed.returncode, wall_s=time.monotonic() - start, status='completed',
                       stdout_sha256=sha(OUT / (name + '.stdout')), stderr_sha256=sha(OUT / (name + '.stderr')))
            save()
            need(completed.returncode == code, 'unexpected code: ' + name)
        state['pins_after'] = pins()
        need(state['pins_after'] == state['pins_before'], 'closure')
        state['status'] = 'completed'
        save()
        check()
    except BaseException as error:
        state.update(status='failed', failure=str(error))
        save()
        raise


def check():
    state = json.loads((OUT / 'capture.json').read_text())
    need(state['status'] == 'completed' and state['GCP_used'] is False, 'completed local replay')
    need(state['pins_before'] == state['pins_after'] == pins(), 'LIVE sources/binaries/initial-failure pins')
    need(len(state['commands']) == 3, 'exact replay inventory')
    for row, (name, argv, code) in zip(state['commands'], prior.recipe()[-3:]):
        need(row['name'] == name and row['argv'] == argv and row['status'] == 'completed' and
             row['exit_code'] == row['expected_exit'] == code, 'command binding')
        for stream in ('stdout', 'stderr'):
            need(sha(OUT / (name + '.' + stream)) == row[stream + '_sha256'], 'raw output hash')
    for name in ('reproduce_release', 'reproduce_sanitize'):
        need(json.loads((OUT / (name + '.stdout')).read_text()) == prior.EXPECTED, 'changed population observation')
        need((OUT / (name + '.stderr')).read_bytes() == b'', 'unexpected diagnostic')
    need(json.loads((OUT / 'expected_shift_stop.stdout').read_text()) ==
         {'stage': 'before_product_shift', 'shell_size': 32}, 'setup')
    text = (OUT / 'expected_shift_stop.stderr').read_text()
    need('full_coverage_certificate.hpp:272:' in text and
         'shift exponent 32 is too large for 32-bit type' in text, 'expected arithmetic diagnostic')
    print(json.dumps(dict(status='passed', replay_commands=3, original_failed_capture_preserved=True)))


if __name__ == '__main__':
    need(sys.argv[1:] in (['run'], ['check']), 'usage: replay.py run|check')
    capture() if sys.argv[1] == 'run' else check()
