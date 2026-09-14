#!/usr/bin/env python3
"""Paired, bounded audit of the autonomous sibling certificate on pinned inputs."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
LIDAR = BASE.parent / 'lidar08_20260914/prepared'
CHECKER = BASE.parent / 'q2_front_20260914/measure.py'
CHECKER_SHA = '8add7d2849eee2c7cf67ed2461c73cd0ad1c4105e26fbceb68774dd176079509'


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


require(sha(CHECKER) == CHECKER_SHA, 'changed q2 audit checker')
SPEC = importlib.util.spec_from_file_location('pinned_q2_checker', CHECKER)
REFERENCE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(REFERENCE)


def keys(dataset, sizes, separations=(8,), reverse=False):
    modes = ('sibling_remaining', 'sibling', 'baseline') if reverse else (
        'baseline', 'sibling', 'sibling_remaining')
    return [(dataset, n, s, 'samples', 'shared', mode)
            for n in sizes for s in separations for mode in modes]


PLANS = {
    'pilot': keys('single_000000', (8000,)),
    'repeat_pilot': keys('single_000000', (8000,), reverse=True),
    'other_scans': keys('single_000100', (8000,)) + keys('single_000200', (8000,)),
    'growth': keys('single_000000', (16000, 32000)),
    'check50k': keys('single_000000', (50000,)),
    'separation': keys('single_000000', (8000,), (10, 12)),
    'clusters8k': keys('clusters_seed3', (8000,)),
    'clusters16k': keys('clusters_seed3', (16000,)),
    'repeat_clusters8k': keys('clusters_seed3', (8000,), reverse=True),
}


def input_path(dataset, n):
    if dataset == 'clusters_seed3':
        return BASE / 'inputs' / f'clusters_n{n}.u16le'
    require(dataset in ('single_000000', 'single_000100', 'single_000200'), 'unknown dataset')
    return LIDAR / dataset / f'n{n}.u16le'


def stamp():
    return datetime.now(timezone.utc).isoformat()


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2,
                               sort_keys=True, allow_nan=False) + '\n')


def check(row):
    # Explicit reuse of the complete q2 input/output/mass checks pinned above.
    REFERENCE.check(dict(row, key=row['key'][:5]))
    result = row['result']
    mode = row['key'][5]
    require(result['sibling_mode'] == mode, 'wrong sibling mode')
    work = result['sibling_work']
    names = ('child_entries', 'population_eligible', 'bound_tests',
             'rejected_children', 'rejected_pairs')
    require(set(work) == set(names), 'unexpected sibling counters')
    require(all(type(work[name]) is int and work[name] >= 0 for name in names),
            'invalid sibling counter')
    require(work['bound_tests'] == work['population_eligible']
            <= work['child_entries'], 'broken sibling test ledger')
    require(work['rejected_children'] <= work['bound_tests']
            and work['rejected_children'] <= work['rejected_pairs']
            <= result['rejected_pairs'], 'broken sibling rejection ledger')
    require(result['census_work']['query_tasks'] == result['anchor_queries']
            + 2 * result['census_work']['query_splits'], 'changed query-task convention')
    require((mode != 'baseline' or all(work[name] == 0 for name in names)),
            'baseline unexpectedly used sibling instrumentation')


def command_for(binary, key):
    dataset, n, s, front, census, mode = key
    return [str(binary), str(input_path(dataset, n)), '10', str(s), front, census, mode]


def build_paths(binary, receipt):
    build = json.loads(receipt.read_text())
    require(build['status'] == 'passed'
            and build['source_commit'] == 'f7edd6463adfeba559f305d14361bf85a8c25703',
            'wrong or unqualified prototype build')
    require(build['modes'] == ['baseline', 'sibling', 'sibling_remaining'],
            'build does not cover all three variants')
    require(all(step.get('returncode') == step['expected_returncode'] for step in build['steps']),
            'build contains an unclosed step')
    probe = build['binaries']['probe']
    require((ROOT / probe['path']).resolve() == binary.resolve()
            and sha(binary) == probe['sha256'], 'binary differs from qualified build')
    paths = {binary, receipt}
    for field in ('archive', 'adapted_archive'):
        path = ROOT / build[field]
        require(sha(path) == build[field + '_sha256'], 'changed build source archive')
        paths.add(path)
    for name, expected in build['audit_inputs'].items():
        path = ROOT / name
        require(sha(path) == expected, 'changed audit build input: ' + name)
        paths.add(path)
    return paths


def attempt(key, command, cpu, stream):
    row = dict(key=key, command=command, started_utc=stamp(), status='failed',
               returncode=None, stdout='', stderr='')
    path = input_path(key[0], key[1])
    try:
        row.update(input_sha256=sha(path), input_fnv64=REFERENCE.fnv(path),
                   loadavg_before=os.getloadavg())
        try:
            run = subprocess.run(command, capture_output=True, text=True, timeout=180,
                                 preexec_fn=lambda: os.sched_setaffinity(0, {cpu}))
            row.update(returncode=run.returncode, stdout=run.stdout, stderr=run.stderr)
            if run.returncode == 0:
                parsed = json.loads(run.stdout)
                # NaN and overflowed exponents (1e999) must remain raw stdout,
                # never poison the finally that preserves this failed attempt.
                json.dumps(parsed, allow_nan=False)
                row['result'] = parsed
        except subprocess.TimeoutExpired as error:
            row.update(returncode=124, timeout=True,
                       stdout=(error.stdout or b'').decode(errors='replace'),
                       stderr=(error.stderr or b'').decode(errors='replace'))
        check(row)
        row['status'] = 'completed'
        return row
    except BaseException as error:
        row.update(status='interrupted' if isinstance(error, KeyboardInterrupt) else 'failed',
                   error_type=type(error).__name__, error=str(error))
        raise
    finally:
        row['finished_utc'] = stamp()
        stream.write(json.dumps(row, ensure_ascii=False, allow_nan=False) + '\n')
        stream.flush()


def validate(directory):
    manifest = json.loads((directory / 'MANIFEST.json').read_text())
    done = json.loads((directory / 'COMPLETION.json').read_text())
    require(manifest['runner_sha256'] == sha(Path(__file__)), 'changed runner')
    require(manifest['checker_sha256'] == CHECKER_SHA, 'changed checker authority')
    require(done['status'] == 'completed', 'incomplete campaign')
    require(done['manifest_sha256'] == sha(directory / 'MANIFEST.json')
            and done['measures_sha256'] == sha(directory / 'MEASURES.jsonl'), 'broken closure')
    for path, pin in manifest['pins'].items():
        require(sha(ROOT / path) == pin, 'changed pinned input: ' + path)
    build_paths(ROOT / manifest['binary'], ROOT / manifest['build_receipt'])
    rows = [json.loads(line) for line in (directory / 'MEASURES.jsonl').read_text().splitlines()]
    require([row['key'] for row in rows] == [list(key) for key in PLANS[manifest['plan']]],
            'wrong paired matrix')
    results = {}
    for row in rows:
        check(row)
        key = row['key']
        require(row['command'] == command_for(ROOT / manifest['binary'], key),
                'command/result mismatch')
        path = input_path(key[0], key[1])
        require(row['input_sha256'] == manifest['pins'][str(path.relative_to(ROOT))],
                'row input differs from manifest')
        pair = tuple(key[:5])
        result = row['result']
        paired = {name: result[name] for name in ('digest', 'front_work', 'candidate_pairs',
                  'input_rectangles', 'anchor_queries', 'accepted_pairs', 'rejected_pairs')}
        paired['payload'] = {name: value for name, value in result['census_work'].items()
                             if name.startswith('payload_')}
        require(pair not in results or results[pair] == paired,
                'paired input/front/support/payload changed')
        results[pair] = paired
    return dict(status='passed', plan=manifest['plan'], rows=len(rows), paired_inputs=len(results))


def measure(plan, binary, build_receipt):
    directory = BASE / ('campaign_' + plan)
    require(not directory.exists(), 'refuse to overwrite campaign')
    require(binary.is_file() and build_receipt.is_file(), 'missing qualified prototype build')
    paths = {input_path(key[0], key[1]) for key in PLANS[plan]}
    paths.update(build_paths(binary, build_receipt))
    paths.add(CHECKER)
    paths.update(p for p in (BASE / 'INPUTS.json', BASE / 'prepare_clusters.py') if p.is_file())
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sorted(paths)}
    cpus = sorted(os.sched_getaffinity(0))
    cpu = cpus[-1]
    manifest = dict(schema='mhgp8_q2_sibling_campaign_v1', plan=plan,
                    scope='audit_prototype_q2_stream_not_full', public_status='not_claimed',
                    source_commit='f7edd6463adfeba559f305d14361bf85a8c25703',
                    binary=str(binary.relative_to(ROOT)),
                    build_receipt=str(build_receipt.relative_to(ROOT)), matrix=PLANS[plan], pins=pins,
                    runner_sha256=sha(Path(__file__)), checker_sha256=CHECKER_SHA,
                    command=sys.argv, platform=platform.platform(), python=sys.version,
                    allowed_cpus=cpus, selected_cpu=cpu, timeout_seconds=180,
                    timeout_policy='failed_run_no_truncated_result', started_utc=stamp())
    directory.mkdir()
    write(directory / 'MANIFEST.json', manifest)
    status = 'failed'
    try:
        with (directory / 'MEASURES.jsonl').open('x') as stream:
            for key in PLANS[plan]:
                row = attempt(key, command_for(binary, key), cpu, stream)
                result = row['result']
                print(json.dumps(dict(key=key, pipeline_ms=result['pipeline_total_ms'],
                    count_nodes=result['census_work']['count_node_visits'],
                    sibling_work=result['sibling_work'])), flush=True)
        require(all(sha(ROOT / path) == pin for path, pin in pins.items()),
                'pinned input changed during campaign')
        status = 'completed'
    finally:
        measures = directory / 'MEASURES.jsonl'
        write(directory / 'COMPLETION.json', dict(status=status, finished_utc=stamp(),
            manifest_sha256=sha(directory / 'MANIFEST.json'),
            measures_sha256=sha(measures) if measures.exists() else None))
    print(json.dumps(validate(directory)), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', choices=PLANS)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--build-receipt', type=Path)
    parser.add_argument('--validate', type=Path)
    args = parser.parse_args()
    require((args.plan is None) != (args.validate is None), 'choose plan or validate')
    if args.validate:
        print(json.dumps(validate(args.validate.resolve())))
    else:
        require(args.binary is not None and args.build_receipt is not None,
                'measurement needs binary and its build receipt')
        measure(args.plan, args.binary.resolve(), args.build_receipt.resolve())
