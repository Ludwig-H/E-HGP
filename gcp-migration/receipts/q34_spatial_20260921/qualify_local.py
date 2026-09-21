#!/usr/bin/env python3
"""Close two local mocked adapter selftests; no native/cloud command allowed."""
import json
import os
from pathlib import Path
import signal
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import q34_spatial_worker_v8 as worker
import q34_spatial_session_v8 as session
validator = worker.load_validator()
from run_p0_matrix import invoke, on_signal, utc_stamp, write_json, require
from run_q4_family_checks import pins, digest


def main():
    native = validator.authority(ROOT / 'build/v8_q4_seed_cells_r2_20260921', True)
    names = set(native['source_sha256']) | set(native['artifact_sha256']) | set(validator.PROTOCOL_SOURCES)
    names |= set(worker.PROTOCOL_NAMES) | {worker.HELPER, 'gcp-migration/full_probe_session_v7.py',
        *('gcp-migration/' + name for name in session.GUARDS), str(Path(__file__).resolve()),
        *validator.AUTHORITY_HASHES}
    before = pins(names)
    output = Path(tempfile.mkdtemp(prefix='local_', dir=HERE))
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, error, status = [], None, 'failed'
    try:
        for optimized in (False, True):
            command = [sys.executable, '-B', *(['-O'] if optimized else []),
                       str(ROOT / 'gcp-migration/q34_spatial_selftest_v8.py')]
            record = dict(command=command, started_utc=utc_stamp(), status='failed', exit_code=None)
            try:
                invoke(command, dict(os.environ), ROOT, record, new_session=True)
                require(record['exit_code'] == 0 and 'Ran 9 tests' in record['stderr'] and
                        record['stderr'].rstrip().endswith('OK'), 'local mocked selftest failed')
                record['status'] = 'passed'
            finally:
                record['finished_utc'] = utc_stamp()
                path = output / ('optimized.json' if optimized else 'normal.json')
                write_json(path, record)
                records.append(dict(path=path.name, sha256=digest(path)))
        status = 'passed'
    except BaseException as cause:
        error = f'{type(cause).__name__}: {cause}'
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        after, closing = None, []
        try:
            after = pins(names)
            require(after == before and all(digest(output / r['path']) == r['sha256'] for r in records),
                    'local qualification closure changed')
        except BaseException as cause:
            closing.append(f'{type(cause).__name__}: {cause}')
        final = dict(status=status if not closing else 'failed', error=error, closing_errors=closing,
            records=records, hashes_before=before, hashes_after=after, native_sources=216,
            tests_per_command=9, GCP_used=False, native_execution=False, GPU_executed=False, FULL_executed=False)
        write_json(output / 'COMPLETION.json', final)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    print(json.dumps(dict(path=str(output), status=final['status'], commands=len(records), closing_errors=closing)))
    require(final['status'] == 'passed', 'local qualification failed')


if __name__ == '__main__':
    main()
