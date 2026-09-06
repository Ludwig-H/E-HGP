#!/usr/bin/env python3
"""Capture two pure reads of already closed runs; no watchdog or C++ command."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parent
PINS = ['1=5e3ccdb2880bd449ccc9bd99615300fc9caf609d2a0592dea41d39896cf308eb',
        '2=ca12d24b1e1f98e4074193158fb6de716dbc06a2dcdbb9d1dbda158578f8ef32',
        '4=7cf81cb19342059fb320d9514b2004e3cca4e71c19bde47140a986737bb4d4a0',
        '8=2364a2817193b5a11c3ab5f82eab0741f6a6e486a65b03210d1349b0743040d8']
READER_SHA = 'd318ac4204f20d67e4555c6d1fd080a080590471c3de82992f862e6fc6f25d9e'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def main():
    if sha(BASE / 'read_results.py') != READER_SHA:
        raise ValueError('reader changed')
    output = BASE / 'analysis_r1'
    output.mkdir()
    commands = []
    for optimized in (False, True):
        name = 'optimized' if optimized else 'normal'
        argv = [sys.executable, '-B', *(['-O'] if optimized else []), str(BASE / 'read_results.py'), '--directory', str(BASE)]
        for pin in PINS:
            argv.extend(['--run-pin', pin])
        row = dict(argv=argv, reader_sha256=READER_SHA, started_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()))
        save(output / (name + '.intent.json'), row)
        started = time.monotonic()
        with (output / (name + '.stdout')).open('xb') as out, (output / (name + '.stderr')).open('xb') as err:
            process = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=out, stderr=err, start_new_session=True)
            row['pid'] = process.pid
            try:
                row['exit_code'] = process.wait()
            except KeyboardInterrupt:
                os.killpg(process.pid, signal.SIGTERM)
                row['exit_code'] = process.wait()
                row['interrupted'] = True
            try:
                os.killpg(process.pid, 0)
                row['group_closed'] = False
            except ProcessLookupError:
                row['group_closed'] = True
        row.update(elapsed_seconds=time.monotonic()-started,
                   stdout_sha256=sha(output / (name + '.stdout')), stderr_sha256=sha(output / (name + '.stderr')))
        save(output / (name + '.command.json'), row)
        commands.append(row)
    equal = (output / 'normal.stdout').read_bytes() == (output / 'optimized.stdout').read_bytes()
    passed = equal and all(row['exit_code'] == 0 and row['group_closed'] and not row.get('interrupted') for row in commands)
    result = dict(status='completed' if passed else 'failed', engine_invoked=False, public_status='not_claimed',
                  commands=commands, normal_optimized_equal=equal, reader_stable=sha(BASE / 'read_results.py') == READER_SHA)
    if not result['reader_stable']:
        result['status'] = 'failed'
    save(output / 'receipt.json', result)
    print(json.dumps(result, sort_keys=True))
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
