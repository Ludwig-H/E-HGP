#!/usr/bin/env python3
"""Close LiDAR campaigns, compare archived baselines and exercise receipt guards."""
from __future__ import annotations

import argparse
import copy
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
sys.path.insert(0, str(BASE))
import measure


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def completed(row):
    require(row['status'] == 'completed', 'row is not completed')
    measure.check(row)


def check():
    build_path = BASE / 'r1_BUILD.json'
    build = json.loads(build_path.read_text())
    require(build['status'] == 'passed', 'build did not pass')
    pins = {str(build_path.relative_to(ROOT)): sha(build_path)}
    archive = ROOT / build['archive']
    require(sha(archive) == build['archive_sha256'], 'changed build archive')
    pins[str(archive.relative_to(ROOT))] = sha(archive)
    with zipfile.ZipFile(archive) as z:
        require(set(z.namelist()) == set(build['source_sha256']), 'wrong archive paths')
        for path, pin in build['source_sha256'].items():
            require(hashlib.sha256(z.read(path)).hexdigest() == pin, 'changed archived source')
    summaries, rows, signatures = [], [], {}
    campaigns = sorted(BASE.glob('campaign_*'))
    require({p.name for p in campaigns} == {'campaign_' + plan for plan in measure.PLANS},
            'missing or unexpected campaign in this closed six-plan package')
    for directory in campaigns:
        summary = measure.validate(directory)
        summaries.append(summary)
        for path in directory.iterdir():
            require(path.is_file(), 'unexpected nested campaign data')
            pins[str(path.relative_to(ROOT))] = sha(path)
        for line in (directory / 'MEASURES.jsonl').read_text().splitlines():
            row = json.loads(line)
            completed(row)
            rows.append(row)
            r = row['result']
            key = tuple(row['key'])
            discrete = {k: v for k, v in r.items() if not k.endswith('_ms')}
            require(key not in signatures or signatures[key] == discrete,
                    'repeat changed discrete work or output')
            signatures[key] = discrete
    old_root = BASE.parent / 'q2_sibling_20260914'
    previous = {}
    # The sibling campaign did not repeat s10/s12. These two baselines
    # belong to the earlier, separately closed complete-q2 campaign.
    directory = BASE.parent / 'q2_front_20260914/campaign_separation'
    done = json.loads((directory / 'COMPLETION.json').read_text())
    path = directory / 'MEASURES.jsonl'
    require(done['status'] == 'completed' and sha(path) == done['measures_sha256'],
            'old separation baseline not closed')
    for p in (path, directory / 'COMPLETION.json'):
        pins[str(p.relative_to(ROOT))] = sha(p)
    for line in path.read_text().splitlines():
        row = json.loads(line)
        require(row['status'] == 'completed' and row['returncode'] == 0,
                'old separation invocation failed')
        previous[tuple(row['key'])] = row['result']
    for directory in sorted(old_root.glob('campaign_*')):
        done = json.loads((directory / 'COMPLETION.json').read_text())
        path = directory / 'MEASURES.jsonl'
        require(done['status'] == 'completed' and sha(path) == done['measures_sha256'],
                'old baseline not closed')
        pins[str(path.relative_to(ROOT))] = sha(path)
        pins[str((directory / 'COMPLETION.json').relative_to(ROOT))] = sha(directory / 'COMPLETION.json')
        for line in path.read_text().splitlines():
            row = json.loads(line)
            if row['key'][-1] == 'baseline' and row['key'][0].startswith('single_'):
                previous[tuple(row['key'][:5])] = row['result']
    baseline_matches = 0
    fields = ('digest', 'front_work', 'census_work', 'candidate_pairs', 'input_rectangles',
              'anchor_queries', 'accepted_pairs', 'rejected_pairs')
    for row in rows:
        if row['key'][5:] == ['none', 'global']:
            key = tuple(row['key'][:5])
            require(key in previous, 'missing old baseline')
            require(all(row['result'][f] == previous[key][f] for f in fields),
                    'new baseline changed old output or work')
            baseline_matches += 1
    require(len(rows) == 36 and len(signatures) == 32 and baseline_matches == 9,
            'incomplete measurement/configuration/baseline coverage')
    seed = next(row for row in rows if row['key'][5:] == ['sibling', 'complement'])
    mutants = []
    changes = [
        ('wrong_mode', lambda r: r.update(witness_order='global')),
        ('missing_structure', lambda r: r['order_work'].__setitem__('structural_splits', 0)),
        ('false_cursor', lambda r: r['census_work'].__setitem__('cursor_advances', 0)),
        ('bad_task_mass', lambda r: r['census_work'].__setitem__('query_tasks', 0)),
        ('missing_sibling_field', lambda r: r['sibling_work'].pop('proposals')),
        ('lost_support', lambda r: r['digest'].__setitem__('supports', r['digest']['supports'] + 1)),
        ('not_finite', lambda r: r.__setitem__('total_ms', float('inf'))),
    ]
    for name, mutate in changes:
        bad = copy.deepcopy(seed)
        mutate(bad['result'])
        bad['stdout'] = json.dumps(bad['result'])
        try:
            completed(bad)
        except (RuntimeError, KeyError, ValueError) as error:
            mutants.append(dict(name=name, detected_by=str(error)))
        else:
            raise RuntimeError('undetected receipt mutant: ' + name)
    bad = copy.deepcopy(seed)
    bad['status'] = 'failed'
    try:
        completed(bad)
    except RuntimeError as error:
        mutants.append(dict(name='failed_row', detected_by=str(error)))
    else:
        raise RuntimeError('failed row accepted')
    compact = []
    for row in rows:
        r = row['result']
        compact.append(dict(key=row['key'], total_ms=r['total_ms'], pipeline_ms=r['pipeline_total_ms'],
                            count_nodes=r['census_work']['count_node_visits'],
                            count_bounds=r['census_work']['count_bound_tests'],
                            count_points=r['census_work']['count_point_tests'],
                            tasks=r['census_work']['query_tasks'], order_work=r['order_work'],
                            sibling_work=r['sibling_work'], payload_nodes=r['census_work']['payload_node_visits'],
                            rectangles=r['input_rectangles'], anchors=r['anchor_queries'],
                            supports=r['accepted_pairs']))
    return dict(status='passed', campaigns=summaries, rows=len(rows), configurations=len(signatures),
                old_baseline_matches=baseline_matches, receipt_mutants=mutants, pins=pins, measures=compact)


def record(name):
    require(name.isidentifier(), 'invalid name')
    output = BASE / (name + '_VALIDATION.json')
    require(not output.exists(), 'refuse overwrite')
    report = dict(status='running', schema='mhgp8_q2_order_lidar_validation_v1',
                  public_status='not_claimed', gcp_used=False, commands=[],
                  started_utc=datetime.now(timezone.utc).isoformat(),
                  verifier_sha256=sha(Path(__file__)))
    report['reader_preflight'] = 'Initial manual check stopped at missing old baseline: s10/s12 were in q2_front/campaign_separation, not the sibling campaigns. Both closed sources are now checked explicitly; no measured row changed.'

    def save():
        output.write_text(json.dumps(report, indent=2, sort_keys=True, allow_nan=False) + '\n')

    def run(command):
        step = dict(command=command)
        report['commands'].append(step)
        save()
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=60)
        step.update(returncode=result.returncode, stderr=result.stderr,
                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest())
        require(result.returncode == 0, 'reader or documentation check failed: ' + result.stderr)
        return result.stdout

    save()
    try:
        normal = json.loads(run([sys.executable, '-B', str(Path(__file__)), '--check']))
        optimized = json.loads(run([sys.executable, '-B', '-O', str(Path(__file__)), '--check']))
        require(normal == optimized, 'normal and optimized closure differ')
        report.update(checks=normal, normal_and_optimized_identical=True)
        sys.path.insert(0, str(ROOT / 'tools'))
        import check_docs
        docs = [BASE / 'README.md', BASE.parent / 'DIALOGUE_COURANT.md',
                BASE.parent / 'q2_product_20260914/README.md']
        errors = [e for p in docs for e in check_docs.validate(p)]
        report['documents'] = {str(p.relative_to(ROOT)): sha(p) for p in docs}
        require(not errors, 'audit documentation errors: ' + str(errors))
        report['canonical_docs'] = run([sys.executable, '-B', 'tools/check_docs.py'])
        report['registry'] = run([sys.executable, '-B', 'tools/check_implementation_status.py'])
        report['status'] = 'passed'
    except BaseException as error:
        report.update(status='failed', error_type=type(error).__name__, error=str(error))
        raise
    finally:
        report['finished_utc'] = datetime.now(timezone.utc).isoformat()
        save()
    print(json.dumps(dict(status=report['status'], receipt=str(output), rows=report['checks']['rows'])))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--name', default='r1')
    args = parser.parse_args()
    if args.check:
        print(json.dumps(check(), sort_keys=True))
    else:
        record(args.name)
