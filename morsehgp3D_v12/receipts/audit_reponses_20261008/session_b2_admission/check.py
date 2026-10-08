#!/usr/bin/env python3
"""Recheck the closed local B2 archive and Git; no native or remote execution."""
import argparse
import hashlib
import importlib.util
import json
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def need(condition, message):
    if not condition:
        raise ValueError(message)


def evaluate(repo, session):
    cap = json.loads((HERE / 'capture.json').read_text())
    for path, pin in cap['helpers'].items():
        raw = (repo / path).read_bytes()
        need(len(raw) == pin['bytes'] and hashlib.sha256(raw).hexdigest() == pin['sha256'], 'helper pin')
    ref = cap['cohort_reference']
    raw = (repo / ref['path']).read_bytes()
    need(len(raw) == ref['bytes'] and hashlib.sha256(raw).hexdigest() == ref['sha256'], 'cohort reference pin')
    initial = {}
    for path, pin in cap['local_files'].items():
        raw = (session / path).read_bytes()
        need(len(raw) == pin['bytes'] and hashlib.sha256(raw).hexdigest() == pin['sha256'], 'local primary pin')
        initial[path] = raw
    helper = load_module(repo / next(iter(cap['helpers'])), 'archive_reader')
    with tempfile.TemporaryDirectory(prefix='audit-b2-') as temp:
        work = Path(temp)
        actual = load_module(HERE / 'provenance.py', 'b2_provenance').run(repo, session, work)
        expected = {k: v for k, v in cap.items() if k not in ('observed_utc', 'cohort_reference')}
        need({k: v for k, v in actual.items() if k != 'observed_utc'} == expected, 'provenance replay')
        package = helper.files(initial['package/package.tar.gz'])
        critical = ('microbancs/mes_t2d_b2/pilote_t2d_b2.py', 'microbancs/mes_t2d_b2/bras_t2d_b2.json',
                    'microbancs/outils/lecteur_full.py', 'microbancs/outils/banc_full.py')
        for relative in critical:
            path = work / 'source' / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(package['morsehgp3D_v12/' + relative])
        fs = helper.files(initial['results/results.tar.gz'])
        base = 'results/cmd/001_t2d_b2_pilote/files/t2d_b2/'
        for path, raw in fs.items():
            if path.startswith(base) and (path.endswith('.jsonl') or path == base + 'rapport_t2d_b2.json'):
                target = work / 'returned' / path[len(base):]
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(raw)
        result = load_module(HERE / 'numerical.py', 'b2_numerical').run(repo, session, work)
        result = json.loads(json.dumps(result))
    for path, raw in initial.items():
        need((session / path).read_bytes() == raw, 'primary changed during replay')
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--session', type=Path, required=True)
    args = ap.parse_args()
    result = evaluate(args.repo, args.session)
    need(result == json.loads((HERE / 'results.json').read_text()), 'numerical replay')
    print('B2 verified: 456 archive entries, 373 processes / 2816 FULL / 2100 decisive warm; '
          'lot and tables adopted; no CPU-only FULL measured')


if __name__ == '__main__':
    main()
