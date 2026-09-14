#!/usr/bin/env python3
"""Close each bounded model attempt; preserve failures and exact source bytes."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def main(name):
    if not name or any(not (c.isalnum() or c == '_') for c in name):
        raise RuntimeError('invalid receipt name')
    receipt, output, archive = (BASE / (name + suffix) for suffix in ('_RUN.json', '_RESULT.json', '_sources.zip'))
    if any(p.exists() for p in (receipt, output, archive)):
        raise RuntimeError('refuse to overwrite a previous attempt')
    sources = [BASE / 'model.py', BASE / 'record.py',
               BASE.parent / 'q2_front_20260914/order_fixture.py',
               BASE.parent / 'p0_q2_census_bounds_probe.py']
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sources}
    record = dict(schema='mhgp8_q2_complement_model_run_v1', status='running',
                  started_utc=datetime.now(timezone.utc).isoformat(), sources=pins,
                  command=[sys.executable, '-B', str(BASE / 'model.py')],
                  python=sys.version, platform=platform.platform(), timeout_seconds=180)
    write(receipt, record)
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as capture:
        for source in sources:
            capture.write(source, str(source.relative_to(ROOT)))
    record.update(source_archive=str(archive.relative_to(ROOT)), source_archive_sha256=sha(archive))
    write(receipt, record)
    try:
        run = subprocess.run(record['command'], text=True, capture_output=True, timeout=180)
        record.update(returncode=run.returncode, stderr=run.stderr)
        output.write_text(run.stdout)
        record['result_sha256'] = sha(output)
        if run.returncode != 0:
            raise RuntimeError('bounded model failed; see preserved stderr')
        result = json.loads(run.stdout)
        if result['status'] != 'passed' or any(sha(ROOT / path) != pin for path, pin in pins.items()):
            raise RuntimeError('failed result or changed source during capture')
        record['status'] = 'passed'
    except BaseException as error:
        record.update(status='failed', error_type=type(error).__name__, error=str(error))
        if isinstance(error, subprocess.TimeoutExpired):
            record['stderr'] = (error.stderr or b'').decode(errors='replace')
            output.write_bytes(error.stdout or b'')
            record['result_sha256'] = sha(output)
        raise
    finally:
        record['finished_utc'] = datetime.now(timezone.utc).isoformat()
        write(receipt, record)
    print(json.dumps(dict(status=record['status'], receipt=str(receipt), result=str(output))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', default='r1')
    args = parser.parse_args()
    main(args.name)
