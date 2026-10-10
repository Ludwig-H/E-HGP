#!/usr/bin/env python3
"""Rejoue le lecteur statistique public immuable sur les 300 journaux B3b épinglés."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def run(raw, repo):
    cap = json.loads((HERE / 'capture.json').read_text())
    source = subprocess.check_output(['git', 'show', cap['source_git'] + ':' + cap['reader_path']], cwd=repo)
    need(hashlib.sha256(source).hexdigest() == cap['reader_sha256'], 'immutable statistical reader')
    with tempfile.TemporaryDirectory(prefix='b3b-stats-reader-') as td:
        p = Path(td) / 'reader.py'
        p.write_bytes(source)
        spec = importlib.util.spec_from_file_location('b3b_stats_reader', p)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module.HERE = HERE
        result = module.run(raw, repo)
    expected = json.loads((HERE / 'results.json').read_text())
    need(json.loads(json.dumps(result)) == expected, 'stored results')
    return dict(status='ok', processes=result['processes'], hot_passes=result['hot_passes'],
                AA=result['AA'], statistical_rule=result['statistical_rule'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--raw', type=Path, required=True)
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args.raw, args.repo), ensure_ascii=False))
