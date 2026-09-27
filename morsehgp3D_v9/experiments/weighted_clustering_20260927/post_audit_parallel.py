#!/usr/bin/env python3
"""Explicit readback for the parallel pilot, including interrupted-unit reuse.

No receipt is rewritten or coerced to the serial schema. The pinned serial
post-audit supplies only arithmetic functions. Orchestration/provenance is
checked here before scores are replayed. No fit, geometry or EOM is rerun.
"""
from __future__ import annotations

import argparse
from fractions import Fraction as Q
import importlib.metadata
import json
from pathlib import Path
import sys

import post_audit as arithmetic

HERE = Path(__file__).resolve().parent
SCHEMA = 'mhgp9_weighted_full_gaussian_parallel_pilot_v1'
ARITHMETIC_SHA = '95cece7c7b74d839495485d8bad018abffb246458f2d4a800e68c479387e6e1d'
SERIAL_SHA = '30bab0dc0d3009ac68eb8434e01888d56d8ce83cfcacf0bab636a8a422d18d9f'
RUNNER = HERE / 'benchmark_full_weighted_parallel_r2.py'
PLAN = dict(cases=list(arithmetic.CASES), k=[5, 10], sizes=[20, 50], exp_z=[1, 2])
KEYS = [(c, k) for c in arithmetic.CASES for k in (5, 10)]
ENVIRONMENT = {key: '1' for key in ('OPENBLAS_NUM_THREADS', 'OMP_NUM_THREADS', 'MKL_NUM_THREADS',
                                  'NUMEXPR_NUM_THREADS', 'VECLIB_MAXIMUM_THREADS')}
CONTEXT_FIELDS = ('sources_before', 'native_binary', 'native_binary_sha256', 'qualification',
                  'qualification_sha256', 'baseline_receipt', 'baseline_receipt_sha256',
                  'manifest', 'manifest_sha256', 'input_hashes', 'fixed_pins')
FIXED_FIELDS = ('native_binary', 'native_binary_sha256', 'qualification', 'qualification_sha256',
                'baseline_receipt', 'baseline_receipt_sha256', 'manifest', 'manifest_sha256')
need, sha, read = arithmetic.need, arithmetic.sha, arithmetic.read


def pin(pins, path, digest):
    path = str(path)
    need(path not in pins or pins[path] == digest, 'conflicting provenance pin: ' + path)
    pins[path] = digest


def pins_from(pins, mapping):
    for path, digest in mapping.items():
        pin(pins, path, digest)


def check_pins(pins):
    for path, digest in pins.items():
        need(sha(path) == digest, 'LIVE pin: ' + path)


def key(row):
    return row['case'], row['k']


def row_key(row):
    return tuple(row[field] for field in ('case', 'k', 'min_cluster_size', 'exp_z', 'method'))


def payload_names():
    return [f'measure_z{z}.json.gz' for z in (1, 2)] + [
        f'weighted_m{m}_z{z}.json.gz' for z in (1, 2) for m in (20, 50)]


def validate_ledger(events, worker_commands, expected, workers):
    """Validate starts/joins and concurrency without consulting current PIDs."""
    need(type(workers) is int and workers in (1, 2, 3), 'declared worker count')
    commands = {key(row): row for row in worker_commands}
    need(len(commands) == len(worker_commands) and set(commands) == set(expected), 'worker command partition')
    active, seen, joined = {}, set(), set()
    maximum = 0
    for event in events:
        unit = key(event)
        need(unit in commands, 'ledger unknown unit')
        command = commands[unit]
        if event['event'] == 'start':
            need(unit not in seen, 'duplicate/restarted ledger unit')
            need(event == dict(event='start', case=unit[0], k=unit[1], pid=command['pid'], argv=command['argv']),
                 'start argv/PID binding')
            need(type(command['pid']) is int and command['pid'] > 0 and command['pid'] not in active.values(),
                 'positive nonconcurrent worker PID')
            active[unit] = command['pid']; seen.add(unit)
            maximum = max(maximum, len(active))
            need(maximum <= workers, 'worker concurrency bound')
        elif event['event'] == 'joined':
            need(unit in active and event == dict(event='joined', **command), 'join follows matching start')
            need(command['returncode'] == 0 and not command.get('interrupted'), 'joined successful worker')
            del active[unit]; joined.add(unit)
        else:
            raise ValueError('unknown ledger event')
    need(not active and seen == joined == set(expected), 'all and only new units joined')
    return maximum


def validate_reuse_header(old, proof, receipt):
    need(old['schema'] == arithmetic.SCHEMA and old['status'] == 'failed' and
         old.get('error') == 'KeyboardInterrupt()', 'original interruption remains a failure')
    need(old['plan'] == PLAN, 'original fixed plan')
    need(all(old[name] == receipt[name] for name in FIXED_FIELDS), 'same resumed geometry/input authority')
    expected_sources = {p: h for p, h in receipt['sources_before'].items() if p != str(RUNNER)}
    need(old['sources_before'] == expected_sources, 'exact resumed serial source inventory')
    need(proof['resume_verification_currentpins'] == old['sources_before'], 'new resume verification pins')
    need(proof['missing_original_failure_source_closure'] == ('sources_after' not in old),
         'do not invent original source closure')
    if 'sources_after' in old:
        need(old['sources_after'] == old['sources_before'], 'original recorded closure mismatch')
    need(all(receipt['input_hashes'].get(p) == h for p, h in old['input_hashes'].items()), 'same interrupted inputs')


def validate_cleanup(receipt):
    need(not receipt.get('cleanup_errors'), 'no failed owned-group cleanup')
    expected = [dict(pgid=command['pid'], signals=[], status='closed',
                     residual_group_before_cleanup=False, returncode=0)
                for command in receipt['worker_commands']]
    need(receipt.get('group_cleanup', []) == expected, 'all normal worker groups closed without residual descendants')
    return len(expected)


def validate_inputs(receipt, manifest):
    expected = {}
    for case_id in PLAN['cases']:
        case = manifest[case_id]
        need(case['labels_json'] in {case[name] for name in case['prepared_sha256']}, 'truth labels explicitly pinned')
        for name, digest in case['prepared_sha256'].items():
            pin(expected, case[name], digest)
    need(receipt['input_hashes'] == expected, 'exact 13-case manifest input inventory, including labels')


def untag(row, tag):
    """Compare provenance-tagged objects without altering a saved receipt."""
    need(row.get('reused_from') == tag, 'exact per-object reuse tag')
    return {name: value for name, value in row.items() if name != 'reused_from'}


def native_binding(directory, command, receipt, case, k, pins, tag=None):
    plain = untag(command, tag) if tag is not None else command
    need(tag is not None or 'reused_from' not in command, 'unproved native reuse tag')
    argv = [receipt['native_binary'], '--input', case['points_u32le'], '--k', str(k), '--workers', '1']
    need(plain['returncode'] == 0 and plain['argv'] == argv and key(plain) == (case['id'], k), 'native argv/unit binding')
    command_path = directory / 'command.json'
    need(read(command_path) == plain, 'native command JSON binding')
    pin(pins, command_path, sha(command_path))
    for suffix, filename in (('stdout', 'native.json'), ('stderr', 'native.stderr')):
        pin(pins, directory / filename, plain[suffix + '_sha256'])
    intent_path = directory / 'intent.json'
    need(read(intent_path) == dict(argv=argv, binary_sha256=receipt['native_binary_sha256'],
                                 input_sha256=receipt['input_hashes'][case['points_u32le']]), 'native intent binding')
    pin(pins, intent_path, sha(intent_path))


def validate_capture(capture):
    path = capture / 'receipt.json'; receipt = read(path)
    need(receipt['schema'] == SCHEMA and receipt['status'] == 'completed', 'completed parallel receipt required')
    need(receipt['plan'] == PLAN and len(receipt['rows']) == 364 and len(receipt['commands']) == 26, 'complete parallel grid')
    need([key(c) for c in receipt['commands']] == KEYS, 'native commands in declared case/K order')
    need(receipt['sources_before'] == receipt['sources_after'], 'parallel source closure')
    need(str(RUNNER) in receipt['sources_before'], 'active R2 runner source is pinned')
    need(receipt['sources_before'][str(HERE / 'benchmark_full_weighted.py')] == SERIAL_SHA, 'explicit serial port pin')
    need(receipt['root_policy'] == 'excluded_for_both' and receipt['gzip_compresslevel'] == 1 and
         receipt['worker_environment'] == ENVIRONMENT, 'parallel execution profile')
    need(all(receipt[name] is False for name in ('GCP_used', 'GPU_used', 'engine_modified')), 'local scope')
    need(receipt['approximate_postprocessing'] is True and receipt['selected_subset_after_previous_scores'] is True,
         'honest prototype scope')
    need(sha(HERE / 'post_audit.py') == ARITHMETIC_SHA, 'pinned reused arithmetic reader')
    pins = {str(path): sha(path), str(Path(__file__).resolve()): sha(__file__), str(HERE / 'post_audit.py'): ARITHMETIC_SHA}
    for name in ('sources_before', 'input_hashes', 'fixed_pins', 'artifacts'):
        pins_from(pins, receipt[name])
    expected_fixed = {receipt[name]: receipt[name + '_sha256'] for name in
                      ('native_binary', 'qualification', 'baseline_receipt', 'manifest')}
    need(receipt['fixed_pins'] == expected_fixed, 'exact fixed authority pins')
    need(receipt['baseline_receipt_sha256'] == arithmetic.BASELINE_SHA and receipt['manifest_sha256'] == arithmetic.MANIFEST_SHA,
         'frozen comparator/input authority')
    qualification = read(receipt['qualification'])
    need(qualification['schema'] == 'mhgp9_weighted_full_attachment_qualification_v1' and
         qualification['status'] == 'passed' and qualification['sources_before'] == qualification['sources_after'],
         'closed passed attachment qualification')
    need(qualification['native_binary_sha256'] == receipt['native_binary_sha256'], 'qualified native executable')
    pins_from(pins, qualification['sources_after'])
    for filename, digest in qualification['artifacts'].items():
        pin(pins, Path(receipt['qualification']).parent / filename, digest)
    baseline = read(receipt['baseline_receipt'])
    need(baseline['status'] == 'completed' and baseline['sources_before'] == baseline['sources_after'], 'closed comparator baseline')
    pins_from(pins, baseline['sources_after'])
    manifest = {row['id']: row for row in read(receipt['manifest'])['cases']}
    validate_inputs(receipt, manifest)
    context = {name: receipt[name] for name in CONTEXT_FIELDS}
    intent_path = capture / 'intent.json'; intent = read(intent_path)
    need(all(intent[name] == receipt[name] for name in (*CONTEXT_FIELDS, 'schema', 'plan', 'workers',
                                                      'worker_environment', 'reuse_verification')), 'parent intent binding')
    pin(pins, intent_path, sha(intent_path))
    rows = {row_key(row): row for row in receipt['rows']}
    expected_rows = {(c, k, m, z, method) for c, k in KEYS for m in (20, 50) for z in (1, 2)
                     for method in (arithmetic.METHOD, 'hgp_first_coverage', 'hdbscan_common')}
    expected_rows.update((c, k, m, 1, 'hdbscan_standard') for c, k in KEYS for m in (20, 50))
    need(len(rows) == 364 and set(rows) == expected_rows, 'exact unique row grid')
    units = {unit: [r for r in receipt['rows'] if key(r) == unit] for unit in KEYS}
    need([key(r) for r in receipt['rows']] == [unit for unit in KEYS for _ in range(14)], 'unit row order')
    commands = {key(row): row for row in receipt['commands']}
    reused = {unit for unit in KEYS if 'reused_from' in commands[unit]}
    fresh = set(KEYS) - reused
    proof = receipt['reuse_verification']; old = None
    if proof is not None:
        old_path = Path(proof['receipt']); pin(pins, old_path, proof['receipt_sha256'])
        old = read(old_path); validate_reuse_header(old, proof, receipt)
        for name in ('sources_before', 'input_hashes', 'artifacts'):
            pins_from(pins, old[name])
        old_commands = {key(c): c for c in old['commands']}
        need(len(old_commands) == len(old['commands']), 'unique interrupted native commands')
        for unit, command in old_commands.items():
            folder = old_path.parent / f'{unit[0]}_k{unit[1]}'
            need(read(folder / 'command.json') == command, 'all interrupted recorded commands retained')
            pin(pins, folder / 'command.json', sha(folder / 'command.json'))
            for suffix, filename in (('stdout', 'native.json'), ('stderr', 'native.stderr')):
                pin(pins, folder / filename, command[suffix + '_sha256'])
        complete_old = {unit for unit in KEYS if sum(key(row) == unit for row in old['rows']) == 14}
        need(reused == complete_old and proof['units'] == len(reused), 'reuse all and only complete original units')
    else:
        need(not reused, 'no tags without original-failure proof')
    inherited_rows = {row_key(row): {field: row[field] for field in arithmetic.ROW_FIELDS} for row in baseline['rows']}
    worker_commands = {key(row): row for row in receipt['worker_commands']}
    need(len(worker_commands) == len(receipt['worker_commands']) and set(worker_commands) == fresh, 'new/reused worker partition')
    for unit in KEYS:
        c, k = unit; directory = capture / f'{c}_k{k}'; tag = None
        if unit in reused:
            source_dir = old_path.parent / directory.name
            tag = dict(receipt=str(old_path), receipt_sha256=proof['receipt_sha256'], directory=str(source_dir.resolve()))
            need(directory.is_symlink() and directory.resolve() == source_dir.resolve(), 'reused immutable unit link')
            need(untag(commands[unit], tag) == old_commands[unit], 'reused command exactly inherited')
            previous = [row for row in old['rows'] if key(row) == unit]
            need([untag(row, tag) for row in units[unit]] == previous, 'whole 14-row reused unit')
            for name in payload_names():
                original = str(source_dir / name); local = str(directory / name)
                need(original in old['artifacts'] and receipt['artifacts'].get(local) == old['artifacts'][original], 'six original payloads, no partial reuse')
        else:
            worker_command = worker_commands[unit]
            need(worker_command['returncode'] == 0 and not worker_command.get('interrupted'), 'successful worker command')
            spec_path = capture / (directory.name + '.spec.json')
            need(str(spec_path) in receipt['artifacts'], 'recorded worker spec')
            spec = read(spec_path)
            need(spec == dict(context=context, case=manifest[c], k=k, directory=str(directory)), 'worker exact spec/context')
            argv = worker_command['argv']
            need(len(argv) == 5 and argv[0] == sys.executable and
                 argv[1:] == ['-B', str(RUNNER), '--worker-spec', str(spec_path)], 'worker CLI')
            result_path = directory / 'worker_receipt.json'
            need(str(result_path) in receipt['artifacts'], 'recorded worker receipt')
            result = read(result_path)
            need(result['status'] == 'completed' and result['worker_pid'] == worker_command['pid'] and
                 result['commands'] == [commands[unit]] and result['rows'] == units[unit], 'worker-to-parent exact handoff')
            need(all(receipt['artifacts'].get(p) == h for p, h in result['artifacts'].items()), 'worker artifact handoff')
            for name in payload_names():
                need(str(directory / name) in result['artifacts'], 'six new payloads')
            for suffix in ('stdout', 'stderr'):
                pin(pins, capture / (directory.name + '.worker.' + suffix), worker_command[suffix + '_sha256'])
            need(all('reused_from' not in row for row in units[unit]), 'fresh rows have no inherited tags')
        native_binding(directory, commands[unit], receipt, manifest[c], k, pins, tag)
        for row in units[unit]:
            need(all(row[name] == manifest[c][name] for name in ('n', 'regime', 'communities', 'separation', 'seed')), 'scene metadata')
            if row['method'] != arithmetic.METHOD:
                plain = untag(row, tag) if tag is not None else row
                need(plain == inherited_rows[row_key(row)], 'comparator row exactly copied')
    ledger_path = capture / 'ledger.jsonl'
    need(str(ledger_path) in receipt['artifacts'], 'recorded orchestration ledger')
    events = [json.loads(line) for line in ledger_path.read_text().splitlines()]
    maximum = validate_ledger(events, receipt['worker_commands'], fresh, receipt['workers'])
    groups_closed = validate_cleanup(receipt)
    check_pins(pins)
    orchestration = dict(reused_units=len(reused), newly_executed_units=len(fresh),
        native_commands=26, worker_commands=len(fresh), maximum_observed_workers=maximum,
        owned_groups_closed=groups_closed,
        original_failure_preserved=proof is not None,
        original_source_closure_missing=proof['missing_original_failure_source_closure'] if proof else None,
        resume_source_verification='new LIVE verification, not retroactive closure' if proof else None)
    return receipt, rows, manifest, pins, orchestration


def audit(capture):
    receipt, rows, manifest, pins, orchestration = validate_capture(capture)
    fractions = []; mass_error = Q(0); facet_exits = measures = 0
    for c, k in KEYS:
        print('AUDIT', c, k, file=sys.stderr, flush=True)
        truth = read(manifest[c]['labels_json']); folder = capture / f'{c}_k{k}'
        for z in (1, 2):
            model = read(folder / f'measure_z{z}.json.gz')
            units, denominator, error = arithmetic.inspect_measure(model, 1200, k)
            mass_error = max(mass_error, error); measures += 1
            for minimum in (20, 50):
                result = read(folder / f'weighted_m{minimum}_z{z}.json.gz')
                row = rows[c, k, minimum, z, arithmetic.METHOD]
                arithmetic.inspect_selection(result['selection'], model, units, denominator)
                arithmetic.inspect_vote(model, result['selection'], result['vote'], row)
                exact = arithmetic.independent_scores(truth, result['vote']['labels'], row)
                facet_exits += len(units)
                fractions.append(dict(case=c, k=k, min_cluster_size=minimum, exp_z=z, ari=exact))
    check_pins(pins)
    return dict(schema='mhgp9_weighted_parallel_post_capture_score_audit_v1', status='passed',
        capture=str(capture), receipt_sha256=sha(capture / 'receipt.json'), audit_source_sha256=sha(__file__),
        arithmetic_source_sha256=ARITHMETIC_SHA, weighted_rows=len(fractions), comparator_rows=260,
        total_rows=364, measures=measures, facet_exits_checked=facet_exits,
        mass_max_absolute_error_from_covered_points=[mass_error.numerator, mass_error.denominator],
        exact_ARI=fractions, pins_checked=len(pins), pins_sha256=pins, orchestration=orchestration,
        Hungarian_solver_shared=True, Hungarian_solver='scipy.optimize.linear_sum_assignment',
        scipy_version=importlib.metadata.version('scipy'), NMI_recomputed=False,
        geometry_or_EOM_rerun=False, GCP_used=False,
        limitations=['Post-capture addition; no qualification transferred by rewriting schema.',
          'Exact ARI is independent; Hungarian optimization uses the shared SciPy solver.',
          'Mass conservation concerns rounded input masses, not certified real weights.',
          'Resumed units retain the interrupted receipt and its missing original source closure.',
          'No serial speedup, statistical dominance, or nested point partition is inferred.'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    need(not args.output.exists(), 'NEW post-audit report required')
    result = audit(args.capture.resolve())
    with args.output.open('x') as stream:
        json.dump(result, stream, sort_keys=True, indent=2, allow_nan=False); stream.write('\n')
    print(json.dumps({key: value for key, value in result.items() if key not in
                     ('pins_sha256', 'exact_ARI', 'limitations')}, sort_keys=True))
