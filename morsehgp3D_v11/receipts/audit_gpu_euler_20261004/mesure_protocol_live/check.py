#!/usr/bin/env python3
"""Portable pure Python replay of published measurement protocol; no native child."""
import ast
import collections
import contextlib
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types

ROOT = Path(__file__).resolve().parent
CHECKS = 0


def need(value, reason):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(reason)


def load_source(name):
    metadata = json.loads((ROOT / 'BEFORE.json').read_text())
    row = next(item for item in metadata['sources'] if item['copy'] == 'sources/' + name)
    path = ROOT / row['copy']
    raw = path.read_bytes()
    need(hashlib.sha256(raw).hexdigest() == row['sha256'], 'pinned source ' + name)
    tree = ast.parse(raw, filename=str(path))
    # Execute definitions/imports only; never run source __main__ dispatch.
    tree.body = [entry for entry in tree.body if not isinstance(entry, ast.If)]
    namespace = {'__name__': 'pinned_measurement_reference', '__file__': str(path)}
    exec(compile(tree, str(path), 'exec'), namespace)
    return namespace


def invoke_main(namespace, arguments):
    previous = sys.argv
    sys.argv = ['reference'] + list(arguments)
    try:
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return namespace['main']()
    finally:
        sys.argv = previous


def check_orders():
    source = load_source('ab_g4.py')
    williams = source['williams']
    counts = []
    for n in range(13):
        rows = williams(n)
        need(len(rows) == (1 if n <= 1 else n if n % 2 == 0 else 2*n), 'Williams cycle length')
        for row in rows:
            need(sorted(row) == list(range(n)), 'each sequence is a permutation')
        if n >= 2:
            positions = collections.Counter((v, j) for row in rows for j, v in enumerate(row))
            successors = collections.Counter(pair for row in rows for pair in zip(row, row[1:]))
            need(len(positions) == n*n and len(set(positions.values())) == 1, 'balanced positions full cycle')
            need(len(successors) == n*(n-1) and len(set(successors.values())) == 1,
                 'balanced directed immediate successions full cycle')
        counts.append({'variants': n, 'cycle_sequences': len(rows)})
    need(williams(2) == [[0, 1], [1, 0]], 'causal N=2 now alternates AB/BA')
    five = [williams(2)[i % 2] for i in range(5)]
    need(sum(row[0] == 0 for row in five) == 3 and sum(row[0] == 1 for row in five) == 2,
         'five repetitions not claimed balanced')
    need(5 % len(williams(2)) != 0, 'runtime balanced flag false with reps5 N2')
    return counts


def parent_report():
    return {
        'schema': 'ehgp.v11.ab_g4.v1', 'status': 'done', 'verdict': 'refus',
        'refusals': ['preserved parent refusal'],
        'identity': {'tiny': 'a'*64},
        'builds': {'base': {'exists': True}, 'new': {'exists': True}},
        'archives_sha256': {'base': 'b'*64},
        'plan': {'reps': 5, 'w1': False, 'variants': ['base', 'new'],
                 'orders': [['base', 'new'], ['new', 'base']], 'balanced': False},
        'timings': [
            {'frame': 'tiny', 'workers': '48', 'rep': rep, 'variant': variant,
             'code': 0, 'quiet': True, 'dump_sha256': 'a'*64,
             'summary': {'status': 'ok', 'exit': 'ok', 'wall_ns': wall}}
            for rep in range(5) for variant, wall in [('base', 200), ('new', 100)]
        ]
    }


def check_summary():
    source = load_source('ab_summary.py')
    observed = []
    with tempfile.TemporaryDirectory(prefix='measurement-reader-') as tmp:
        path = Path(tmp) / 'report.json'
        for scenario in ['all_valid', 'foreign_dump', 'failed_take', 'both_failed']:
            report = parent_report()
            if scenario == 'foreign_dump':
                report['timings'][3]['dump_sha256'] = 'c'*64
            elif scenario == 'failed_take':
                report['timings'][3]['code'] = 'timeout'
            elif scenario == 'both_failed':
                report['timings'][2]['code'] = 'timeout'
                report['timings'][3]['code'] = 'timeout'
            raw = (json.dumps(report, sort_keys=True) + '\n').encode()
            path.write_bytes(raw)
            need(invoke_main(source, [str(path), '--pairs', 'base:new', '--metrics', 'wall']) == 0,
                 'summary zero means diagnostic read')
            result = json.loads(Path(str(path)+'.paired.json').read_text())
            parent = result['parent']
            need(parent['sha256'] == hashlib.sha256(raw).hexdigest(), 'actual parent byte hash')
            for key in ['schema', 'status', 'verdict', 'refusals', 'identity', 'plan', 'builds', 'archives_sha256']:
                need(parent[key] == report[key], 'preserved parent ' + key)
            need(result['role'] == 'diagnostic' and parent['verdict'] == 'refus', 'no inferred conformity')
            need(result['takes_seen'] == 10, 'all takes accounted')
            count = 5 if scenario == 'all_valid' else 4
            row = result['rows'][0]
            need(row['expected_pairs'] == 5 and row['pairs'] == count, 'expected and retained pair counts')
            need(row['median_ratio'] == 0.5 and row['sign_p'] == 2/(2**count), 'ratios and exact sign test')
            need(row['min_two_sided_p'] == round(2/(2**count), 6), 'minimum possible p reported')
            need(len(result['excluded']) == (0 if scenario == 'all_valid' else 2 if scenario == 'both_failed' else 1),
                 'per-take exclusions retained')
            if scenario in ['foreign_dump', 'failed_take']:
                need(row['dropped'][0]['rep'] == 1, 'one-sided missing pair listed')
            if scenario == 'both_failed':
                need(row['dropped'] == [] and len(result['excluded']) == 2,
                     'both-excluded pair visible globally and in expected-minus-retained, not row dropped')
            observed.append({'scenario': scenario, 'pairs': row['pairs'], 'expected_pairs': row['expected_pairs'],
                             'excluded_takes': len(result['excluded']), 'dropped_rows': len(row['dropped'])})
    return observed


def check_timeout_checkpoint():
    source = load_source('full_timing.py')
    real_run = subprocess.run
    real_write = Path.write_text
    calls = []
    checkpoints = []
    with tempfile.TemporaryDirectory(prefix='measurement-checkpoint-') as tmp:
        base = Path(tmp)
        bench, xyz, ids, output = [base / name for name in ['fake-bench', 'tiny.u32le', 'tiny.ids.u32le', 'out.json']]
        bench.write_bytes(b'not executable; never invoked')
        xyz.write_bytes(bytes(12))
        ids.write_bytes(bytes(4))
        def fake_run(command, **kwargs):
            need(kwargs == {'capture_output': True, 'text': True, 'timeout': 600}, 'source invocation timeout')
            need(command[4:] == ['5', '16', '256', '0', '4294967295', '8589934592', '48', '16379'],
                 'unchanged native benchmark parameters')
            calls.append(command)
            if len(calls) == 2:
                raise subprocess.TimeoutExpired(command, 600, output=b'o'*3001, stderr=b'last stderr')
            text = '\n'.join(json.dumps(event) for event in [
                {'phase': 'full', 'status': 'ok', 'wall_ns': 20_000_000, 'domain_ns': 12_000_000,
                 'forest_ns': 8_000_000, 'cpu_seconds': 0.05},
                {'phase': 'domain', 'single_pass_ns': 9_000_000, 'catalogue_balls': 2},
                {'phase': 'exit', 'status': 'ok'}])
            return types.SimpleNamespace(returncode=0, stdout=text, stderr='')
        def monitor_write(path, data, *args, **kwargs):
            if path == output:
                snapshot = json.loads(data)
                checkpoints.append({'status': snapshot['status'], 'failures': snapshot['failures'],
                                    'takes': len(snapshot['rows'][0]['takes'])})
            return real_write(path, data, *args, **kwargs)
        subprocess.run = fake_run
        Path.write_text = monitor_write
        try:
            code = invoke_main(source, ['--bench', str(bench), '--extra', 'tiny='+str(xyz)+':'+str(ids),
                                       '--work', str(base/'work'), '--out', str(output), '--reps', '3'])
        finally:
            subprocess.run = real_run
            Path.write_text = real_write
        result = json.loads(output.read_text())
        need(code == 1 and result['status'] == 'complete' and result['failures'] == 1,
             'failure persisted while bank completes')
        takes = result['rows'][0]['takes']
        need([item['status'] for item in takes] == ['ok', 'timeout', 'ok'], 'continued after timeout')
        need(takes[1]['code'] is None and not takes[1]['ok'] and len(takes[1]['stdout_tail']) == 2000,
             'timeout status and bounded byte stdout tail')
        need(takes[1]['stderr_tail'] == 'last stderr', 'stderr diagnostics retained')
        need(checkpoints == [{'status': 'running', 'failures': 0, 'takes': 1},
                             {'status': 'running', 'failures': 1, 'takes': 2},
                             {'status': 'running', 'failures': 1, 'takes': 3},
                             {'status': 'complete', 'failures': 1, 'takes': 3}], 'checkpoint after every finished take')
        row = result['rows'][0]
        need(row['provenance']['sites_sha256'] == hashlib.sha256(xyz.read_bytes()).hexdigest(), 'actual XYZ identity')
        need(row['provenance']['ids_sha256'] == hashlib.sha256(ids.read_bytes()).hexdigest(), 'actual IDs identity')
        need(row['sites'] == 1 and row['wall_median_ms'] == 20.0, 'synthetic scalar scene retained')
        need('aucune ne qualifie une trame entiere' in result['bins_note'], 'descriptive bins scope')
        return {'native_children': 0, 'calls_simulated': len(calls), 'checkpoints': checkpoints,
                'final_status': result['status'], 'failures': result['failures'],
                'take_statuses': [item['status'] for item in takes]}


def main():
    orders = check_orders()
    summary = check_summary()
    timeout = check_timeout_checkpoint()
    print(json.dumps({'schema': 'ehgp.audit.measure_protocol_live.v1', 'verdict': 'pass', 'checks': CHECKS,
                      'orders': orders, 'summary': summary, 'timeout': timeout,
                      'scope': 'published Python definitions only; fake subprocess, zero native/GCP'},
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
