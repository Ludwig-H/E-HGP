#!/usr/bin/env python3
"""Verify a closed model, optimized replay, existing root counts and docs."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def main(name, model_run):
    require(bool(name) and all(c.isalnum() or c == '_' for c in name), 'invalid name')
    require(bool(model_run) and all(c.isalnum() or c == '_' for c in model_run), 'invalid model run')
    report_path = BASE / (name + '_VALIDATION.json')
    require(not report_path.exists(), 'refuse to overwrite validation')
    record_path = BASE / (model_run + '_RUN.json')
    receipt = json.loads(record_path.read_text())
    result_path = BASE / (model_run + '_RESULT.json')
    pins = dict(receipt['sources'])
    for path in (record_path, result_path, ROOT / receipt['source_archive'],
                 BASE / 'r1_VALIDATION.json', BASE / 'r1_validation_sources.zip',
                 BASE / 'PROPOSITION.json', BASE / 'PRODUCT_REVIEW.json',
                 BASE / 'product_review_sources.zip', BASE / 'root_singletons.py',
                 BASE / 'verify.py', BASE / 'README.md', BASE.parent / 'DIALOGUE_COURANT.md',
                 ROOT / 'tools/check_docs.py', ROOT / 'tools/check_implementation_status.py'):
        pins[str(path.relative_to(ROOT))] = sha(path)
    report = dict(schema='mhgp8_q2_complement_validation_v1', status='running',
                  started_utc=datetime.now(timezone.utc).isoformat(), sources=pins,
                  commands=[], scope='bounded_model_and_static_review_not_cpp_execution',
                  gcp_used=False, public_status='not_claimed')
    report['model_run'] = model_run

    def save():
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True, allow_nan=False) + '\n')

    def run(command):
        item = dict(command=command, timeout_seconds=180)
        report['commands'].append(item)
        save()
        try:
            completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=180)
        except subprocess.TimeoutExpired as error:
            item.update(status='timeout', stdout=str(error.stdout), stderr=str(error.stderr))
            raise
        item.update(returncode=completed.returncode, stderr=completed.stderr,
                    stdout_sha256=hashlib.sha256(completed.stdout.encode()).hexdigest())
        if completed.returncode:
            item['stdout'] = completed.stdout
        require(completed.returncode == 0, 'validation command failed')
        return completed.stdout

    save()
    try:
        require(receipt['status'] == 'passed' and receipt['returncode'] == 0, 'unclosed model')
        require(sha(result_path) == receipt['result_sha256'], 'changed result')
        require(sha(ROOT / receipt['source_archive']) == receipt['source_archive_sha256'],
                'changed source archive')
        with zipfile.ZipFile(ROOT / receipt['source_archive']) as archive:
            require(set(archive.namelist()) == set(receipt['sources']), 'wrong archive paths')
            for path, pin in receipt['sources'].items():
                require(hashlib.sha256(archive.read(path)).hexdigest() == pin == sha(ROOT / path),
                        'changed archived source: ' + path)
        static = json.loads((BASE / 'PRODUCT_REVIEW.json').read_text())
        require(sha(BASE / 'product_review_sources.zip') == static['archive_sha256'],
                'changed static review archive')
        with zipfile.ZipFile(BASE / 'product_review_sources.zip') as archive:
            require(set(archive.namelist()) == set(static['sources']), 'wrong product archive paths')
            for path, pin in static['sources'].items():
                require(hashlib.sha256(archive.read(path)).hexdigest() == pin, 'changed product capture')
        previous = json.loads((BASE / 'r1_VALIDATION.json').read_text())
        with zipfile.ZipFile(BASE / 'r1_validation_sources.zip') as archive:
            for path in archive.namelist():
                require(hashlib.sha256(archive.read(path)).hexdigest() == previous['sources'][path],
                        'changed historical validation source')
        original = json.loads(result_path.read_text())
        optimized = json.loads(run([sys.executable, '-B', '-O', str(BASE / 'model.py')]))
        require(optimized == original, 'optimized model differs')
        report['normal_and_optimized_model_identical'] = True
        require(len(original['mutants']) == 8, 'missing mutant')
        report['mutants'] = {k: dict(fixture=v['fixture'], detected_by=v['detected_by'])
                             for k, v in original['mutants'].items()}
        rows = [row for case in original['cases'] for row in case['mode_results'].values()]
        rows += list(original['constructor']['mode_results'].values())
        rows += list(original['constructor_all_pairs'].values())
        for row in rows:
            w = row['work']
            require(w['node_visits'] == sum(w[k] for k in (
                'geometric_bound_tests', 'point_tests', 'structural_descents',
                'structural_b0_skips', 'structural_anchor_skips')), 'work ledger differs')
        report['work_ledgers_checked'] = len(rows)
        roots = json.loads(run([sys.executable, '-B', str(BASE / 'root_singletons.py')]))
        roots_o = json.loads(run([sys.executable, '-B', '-O', str(BASE / 'root_singletons.py')]))
        require(roots == roots_o, 'optimized root recount differs')
        report['root_singletons'] = roots
        report['normal_and_optimized_roots_identical'] = True
        pins.update(roots['pins'])
        sys.path.insert(0, str(ROOT / 'tools'))
        import check_docs
        own_docs = (BASE / 'README.md', BASE.parent / 'DIALOGUE_COURANT.md')
        errors = [e for path in own_docs for e in check_docs.validate(path)]
        report['own_docs'] = dict(paths=[str(p.relative_to(ROOT)) for p in own_docs], errors=errors)
        require(not errors, 'audit documentation errors')
        report['canonical_docs_stdout'] = run([sys.executable, '-B', 'tools/check_docs.py'])
        report['registry_stdout'] = run([sys.executable, '-B', 'tools/check_implementation_status.py'])
        require(all(sha(ROOT / p) == pin for p, pin in pins.items()), 'source changed during verification')
        report['status'] = 'passed'
    except BaseException as error:
        report.update(status='failed', error_type=type(error).__name__, error=str(error))
        raise
    finally:
        report['finished_utc'] = datetime.now(timezone.utc).isoformat()
        save()
    print(json.dumps(dict(status=report['status'], report=str(report_path),
                          mutants=len(report['mutants']), root_rows=len(report['root_singletons']['rows']))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', default='r1')
    parser.add_argument('--run', default='r1', help='closed model capture to verify')
    args = parser.parse_args()
    main(args.name, args.run)
