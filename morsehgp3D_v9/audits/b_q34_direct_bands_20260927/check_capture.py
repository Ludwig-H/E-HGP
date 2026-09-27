#!/usr/bin/env python3
"""Reuse the frozen normal/-O plus eight-mutant harness with the new reader."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
BASE = HERE.parent / 'b_q34_bands_20260927/check_capture.py'
BASE_SHA = '2ad9af358281348d1004ea37c1b76998344ebd06d0f8b133d07da04d450f94fd'


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    if hashlib.sha256(BASE.read_bytes()).hexdigest() != BASE_SHA:
        raise ValueError('frozen reader-harness pin')
    harness = load('direct_frozen_harness', BASE)
    runner = load('direct_reader_checked', HERE / 'run.py')
    harness.HERE = HERE
    harness.runner = runner
    before = runner.sha(Path(__file__))
    harness.main()
    runner.need(runner.sha(BASE) == BASE_SHA and runner.sha(Path(__file__)) == before,
                'harness closure')
    path = Path(sys.argv[1]).resolve() / 'checks/checks.json'
    result = runner.read(path)
    result['adapter_sha256'] = before
    result['frozen_harness_sha256'] = BASE_SHA
    runner.save(path, result)
    print(json.dumps(dict(status='PASS', adapted_harness=BASE_SHA, adapter=before), sort_keys=True))


if __name__ == '__main__':
    main()
