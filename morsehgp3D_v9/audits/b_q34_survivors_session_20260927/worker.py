#!/usr/bin/env python3
"""Explicit resident-protocol port: paired S2 only, never a VM manager."""
import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import signal
import sys
import time

PREFIX = 'morsehgp3D_v9/audits/b_q34_survivors_session_20260927'
SCHEMA = 'mhgp9_q34_survivors_session_v1'
USEFUL_SECONDS = 450
FRAME_KEYS = ('w4_baseline', 'w4_candidate', 'w48_baseline', 'w48_candidate')
GATE_EXACT = dict(cases=52, operators=208, queries=26576, S=22408, planned=440,
                  fallbacks=11872, reordered=224, empty=8, first_cuda_calls=1, first_implementation_calls=2)
MASS_FIELDS = ('R R_live raw_pairs raw_q3 raw_q4 P P3 P4 E E3 E4 S S3 S4 digest_u64 '
               'pair_q3_rejected pair_q4_rejected rectangle_visits rectangle_waves planned fallbacks Q_actual').split()
COUNTER_FIELDS = 'queries q3 q4 rejected3 rejected4 visits waves out_of_order'.split()
GEOMETRY_FIELDS = ('factor_sites selection_visits selection_tests selection_shifts selected_sites '
    'anchor_visits witness_attempts self_skips q3_credits q4_credits grouping_visits grouping_sort_tests '
    'occupied_classes class_slots cells_tested descriptors predicates_point_tests predicates_universal_queries '
    'predicates_q2_axis_terms predicates_corner_tests predicates_block_bound_tests predicates_negative_probes').split()
ARENA_FIELDS = ('requests factors anchor_jobs max_factor grouping_reads scatter_writes histogram_slots class_visits '
    'band_passes band_classes_b band_classes_a band_rows band_binary_tests prefix_entries emitted_bands').split()
MEMORY_FIELDS = ('decision_retained_bytes prepared_retained_bytes before_arena_release_bytes arena_owned_peak_bound '
    'temporary_input_bytes resident_device_bytes rectangle_device_bytes pair_device_bytes rectangle_upload_bytes '
    'rectangle_download_bytes pair_upload_bytes pair_download_bytes native_retained_bytes').split()
OUTPUT_MEMORY_FIELDS = ['output_' + p for p in ('size capacity growths growth_copy_bytes append_copy_bytes edge_peak_bytes '
    'array_peak_bytes sort_scratch_bytes host_payload_bytes debug_key_bytes host_conversion_peak_bytes').split()]
TIMING_FIELDS = ('open input_copy_validate index_copy cuda_init index_upload rectangle_upload_allocate rectangle_kernel '
    'rectangle_download_allocate rectangle_release compaction input_release arena consume consume_outer pair_upload_allocate '
    'waves_including_count_download survivor_download_allocate order_convert pair_release adapter_cleanup adapter comparison '
    'native_output_release adapter_plus_native_release_noncontiguous operation_observed diagnostic_overhead').split()
OUTPUT_TIMING_FIELDS = ['output_' + p for p in 'append split sort validate download release'.split()]
HIGH_FIELDS = ('cases refused items empty single empty_waves high32 high63 boundary_inversions inversions growths '
               'growth_copy_bytes upload_bytes download_bytes peak_bytes').split()


def need(value, reason):
    if not value:
        raise ValueError(reason)


def recipes(root, build, c):
    binary = str(build / 'mhgp9_survivors_compare')
    result = dict(guest_schedule=['sudo', '-n', 'cat', '/run/systemd/shutdown/scheduled'],
        compiler=['g++', '--version'], cmake=['cmake', '--version'],
        nvcc=['/usr/local/cuda/bin/nvcc', '--version'], gpu_inventory=['nvidia-smi'], lscpu=['lscpu'],
        configure=['cmake', '-S', str(root / c.PROTOTYPE), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release',
            '-DMHGP9_COMPARE_ENABLE_CUDA=ON', '-DMHGP9_GEN_LIBRARY=', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            '-DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc', '-DCMAKE_CUDA_ARCHITECTURES=120'],
        prepin=['python3', '-B', str(root / PREFIX / 'compile_contract.py'), '--source-root', str(root),
            '--build', str(build), '--manifest', str(root.parent / 'source_manifest.json'),
            '--output', str(root.parent / 'output/compile_before.json')],
        build=['cmake', '--build', str(build), '--target', 'mhgp9_survivors_compare', 'mhgp9_survivors_device_gate', '-j8'],
        high_keys_gate=[str(build / 'device_gate/mhgp9_survivors_device_gate'), '--cuda'],
        device_gate=[binary, '--gate', '--cuda'])
    for width in (4, 48):
        for first in ('baseline', 'candidate'):
            result['ng00_w' + str(width) + '_' + first] = [binary, '--frame', str(root / c.DATA), '--cuda',
                '--workers', str(width), '--first', first]
    return result


def unsigned_map(value, fields, label):
    need(type(value) is dict and set(value) == set(fields), label + ' fields')
    need(all(type(v) is int and 0 <= v < 2**64 for v in value.values()), label + ' unsigned counters')


def durations(value, fields, label):
    need(type(value) is dict and set(value) == set(fields), label + ' fields')
    need(all(type(v) in (int, float) and math.isfinite(v) and v >= 0 for v in value.values()), label + ' finite durations')


def validate_high_gate(value):
    need(type(value) is dict and value.get('schema') == 'mhgp9_survivors_device_gate_v1' and
         value.get('status') == 'passed' and value.get('mode') == 'cuda' and value.get('cuda_executed') is True,
         'real high-key CUDA gate verdict')
    need(set(value) == set(HIGH_FIELDS) | {'schema','status','mode','cuda_executed'}, 'exact high-key gate fields')
    unsigned_map({k: value.get(k) for k in HIGH_FIELDS}, HIGH_FIELDS, 'high-key coverage')
    need(value['cases'] == 24 and value['refused'] == 6 and value['empty'] == 6 and value['single'] == 6 and
         all(value[k] > 0 for k in HIGH_FIELDS), 'high-key CUDA nonvacuity')
    return value


def validate_probe(value, mode, workers=4, first='baseline'):
    need(mode in ('gate', 'frame') and type(value) is dict and value.get('schema') == 'mhgp9_survivors_compare_v1' and
         value.get('status') == 'passed' and value.get('mode') == mode and value.get('cuda_executed') is True,
         'CUDA comparison verdict and scope')
    if mode == 'gate':
        need(set(value) == set(GATE_EXACT) | {'schema','status','mode','cuda_executed'}, 'exact comparison gate fields')
        need(all(type(value.get(k)) is int and value[k] == v for k, v in GATE_EXACT.items()), 'exact comparison gate')
        return value
    need(type(workers) is int and workers in (4, 48) and first in ('baseline', 'candidate'), 'qualified pair recipe')
    fixed = dict(n=39885, input_hash_u64=9245360528374966039, k=5, s=8, Q=262144, Qr=262144,
                 workers_arena=workers, workers_front=1, reference_workers=4)
    need(set(value) == set(fixed) | {'schema','status','mode','cuda_executed','first','sequence','common_times_ms','runs'}, 'exact frame fields')
    need(all(type(value.get(k)) is int and value[k] == v for k, v in fixed.items()), 'complete ng00 input binding')
    need(value.get('first') == first and value.get('sequence') == ('ABBA' if first == 'baseline' else 'BAAB'), 'paired order')
    runs = value.get('runs')
    need(type(runs) is list and len(runs) == 4, 'four paired operations')
    durations(value.get('common_times_ms'), 'input index front reference common_release total'.split(), 'common times')
    for position, record in enumerate(runs):
        candidate = (first == 'candidate') if position in (0, 3) else (first == 'baseline')
        need(type(record) is dict and set(record) == {'implementation','position','first_cuda_call','first_implementation_call',
            'device','output_memory_available','masses','counters','geometry','arena','memory','times_ms'}, 'exact operation fields')
        need(type(record) is dict and type(record.get('position')) is int and record['position'] == position and
             record.get('implementation') == ('survivors_6af40d886' if candidate else 'resident_af369c44') and
             record.get('first_cuda_call') is (position == 0) and
             record.get('first_implementation_call') is (position < 2) and
             record.get('output_memory_available') is candidate, 'exact implementation and first-call tags')
        need(type(record.get('device')) is str and record['device'], 'actual device')
        for name, fields in (('masses', MASS_FIELDS), ('counters', COUNTER_FIELDS), ('geometry', GEOMETRY_FIELDS),
                             ('arena', ARENA_FIELDS), ('memory', MEMORY_FIELDS + (OUTPUT_MEMORY_FIELDS if candidate else []))):
            unsigned_map(record.get(name), fields, name)
        m, w, mem = record['masses'], record['counters'], record['memory']
        expected = dict(P=23686751, P3=17732794, P4=23446295, E=9122704, E3=6667094,
                        E4=8403884, S=2043612, digest_u64=5324876275161635233,
                        R=3133819, R_live=1128166, raw_pairs=103861099, rectangle_visits=229928699)
        need(all(m[k] == v for k, v in expected.items()), 'native exact ng00 masses')
        need(w['visits'] == 537798656, 'native exact pair visits')
        need(w['queries'] == m['E'] and w['q3'] == m['E3'] and w['q4'] == m['E4'] and
             0 < m['Q_actual'] <= value['Q'] and w['waves'] == (m['E'] + m['Q_actual'] - 1) // m['Q_actual'], 'physical waves')
        need(0 < m['R_live'] <= m['R'] and m['planned'] + m['fallbacks'] == m['R_live'] and
             m['rectangle_waves'] == (m['R'] + value['Qr'] - 1) // value['Qr'], 'rectangle partition')
        need(m['S'] <= m['E'] <= m['P'] <= m['raw_pairs'] <= value['n'] * (value['n'] - 1) // 2, 'physical/logical mass order')
        for lane in ('3', '4'):
            need(m['S'+lane] <= m['S'] and m['S'+lane] <= m['E'+lane] <= m['P'+lane] <= m['P'] and
                 m['P'+lane] <= m['raw_q'+lane] <= m['raw_pairs'] and
                 m['S'+lane] == m['P'+lane] - m['pair_q'+lane+'_rejected'] == m['E'+lane] - w['rejected'+lane], 'lane identity')
        durations(record.get('times_ms'), TIMING_FIELDS + (OUTPUT_TIMING_FIELDS if candidate else []), 'operator times')
        t = record['times_ms']
        need(t['open'] + t['arena'] + t['consume_outer'] + t['adapter_cleanup'] <= t['adapter'] + 1e-6 and
             t['consume'] <= t['consume_outer'] + 1e-6 and
             abs(t['adapter_plus_native_release_noncontiguous'] - t['adapter'] - t['native_output_release']) <= 1e-6 and
             abs(t['operation_observed'] - t['adapter'] - t['comparison'] - t['native_output_release'] - t['diagnostic_overhead']) <= 1e-6,
             'paid outer intervals, no double counting nested timings')
        if candidate:
            need(mem['output_size'] == m['S'] and m['S'] <= mem['output_capacity'] <= 2*m['S'] + m['Q_actual'] and
                 mem['output_append_copy_bytes'] == 24*m['S'] and mem['output_debug_key_bytes'] == 0 and
                 mem['output_host_payload_bytes'] >= 12*m['S'], 'candidate retained output and payload-only transport')
        if position:
            need(all(record[k] == runs[0][k] for k in ('masses', 'counters', 'geometry', 'arena', 'device')), 'paired identical exact work')
    common = value['common_times_ms']
    need(sum(common[k] for k in ('input', 'index', 'front', 'reference', 'common_release')) +
         sum(r['times_ms']['operation_observed'] for r in runs) <= common['total'] + 1e-6, 'complete paid common harness')
    return value


def run_probes(command, commands, c):
    high = validate_high_gate(json.loads(command('high_keys_gate', commands['high_keys_gate']), object_pairs_hook=c.unique))
    gate = validate_probe(json.loads(command('device_gate', commands['device_gate']), object_pairs_hook=c.unique), 'gate')
    measures = {}
    for width in (4, 48):
        for first in ('baseline', 'candidate'):
            key = 'w' + str(width) + '_' + first
            measures[key] = validate_probe(json.loads(command('ng00_' + key, commands['ng00_' + key]),
                object_pairs_hook=c.unique), 'frame', width, first)
    return high, gate, measures


def execute(args):
    root, output = args.source_root.resolve(), args.output.absolute()
    raw = args.source_manifest.read_bytes()
    need(hashlib.sha256(raw).hexdigest() == args.source_manifest_sha256, 'manifest pin')
    manifest = json.loads(raw)
    for name in ('common.py', 'compile_contract.py'):
        need(hashlib.sha256((root / PREFIX / name).read_bytes()).hexdigest() == manifest[PREFIX+'/'+name], 'protocol dependency pin')
    sys.path.insert(0, str(root / PREFIX))
    import common as c
    import compile_contract as cc
    manifest = c.read(args.source_manifest)
    need(c.sha(__file__) == manifest[PREFIX+'/worker.py'], 'worker pin')
    need(dict(project=args.project, zone=args.zone, instance=args.instance) == c.TARGET, 'fixed target')
    need(args.closing_margin_seconds >= 300 and not output.exists() and not output.is_symlink() and
         not output.resolve().is_relative_to(root), 'fresh output and closing margin')
    need(args.source_manifest.resolve() == root.parent/'source_manifest.json' and output == root.parent/'output', 'fixed remote layout')
    output.mkdir(mode=0o700)
    legacy = c.load_legacy(root)
    collector = legacy.Commands(output, dict(os.environ))
    state = dict(schema=SCHEMA, status='failed', scope='S2_only_no_FULL', FULL_executed=False,
        public_status='not_claimed', contract_certified=False, target=c.TARGET, generation=args.generation,
        useful_budget_seconds=USEFUL_SECONDS, worker_sha256=c.sha(__file__), source_manifest_sha256=args.source_manifest_sha256,
        CUDA_installation_attempted=False, commands=[])
    build = output.parent/'survivors_build'
    commands = recipes(root, build, c)
    before, compiled_before, compiled_after, previous = {}, None, None, {}
    useful_end = time.monotonic() + USEFUL_SECONDS

    def remaining():
        seconds = min(useful_end-time.monotonic(), args.session_deadline_epoch-args.closing_margin_seconds-time.time())
        need(seconds > 0, 'global useful/deadline budget exhausted')
        return seconds

    def command(name, argv):
        code, stdout, stderr = collector.run(name, argv, timeout=remaining())
        need(code == 0, 'command failed: '+name)
        if name in ('high_keys_gate', 'device_gate') or name.startswith('ng00_'):
            need(not stderr, 'clean probe stderr: '+name)
        return stdout

    def interrupted(signum, _frame):
        raise InterruptedError('worker signal '+str(signum))

    try:
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            previous[sig] = signal.signal(sig, interrupted)
        need(c.sha(args.guard_mark) == args.guard_mark_sha256, 'guard mark pin')
        mark = legacy.fields(args.guard_mark.read_text())
        schedule = legacy.fields(command('guest_schedule', commands['guest_schedule']))
        deadline = legacy.guard_deadline(mark, schedule, args.generation, time.time())
        need(mark['max_run_seconds'] == '3600' and deadline == args.session_deadline_epoch, 'same guard/deadline')
        c.save(output/'guard_evidence.json', dict(mark=mark, schedule=schedule))
        before = {name:c.sha(root/name) for name in manifest}
        need(before == manifest and c.sha(root/c.DATA) == c.DATA_PIN, 'source/input inventory')
        c.save(output/'sources_before.json', before)
        state['provenance'] = c.read(root/c.PROVENANCE)
        for name in ('compiler', 'cmake', 'nvcc', 'gpu_inventory', 'lscpu'):
            command(name, commands[name])
        need(not build.exists(), 'fresh remote build')
        command('configure', commands['configure'])
        command('prepin', commands['prepin'])
        compiled_before = c.read(output/'compile_before.json')
        state['compile_before_sha256'] = c.sha(output/'compile_before.json')
        command('build', commands['build'])
        remaining()
        compiled_after = cc.close(root, build, before, compiled_before)
        cc.validate_closed(compiled_before, compiled_after, root, build, before)
        c.save(output/'compile_after.json', compiled_after)
        state['compile_after_sha256'] = c.sha(output/'compile_after.json')
        state['high_gate'], state['gate'], state['measures'] = run_probes(command, commands, c)
        remaining()
        state['status'] = 'completed'
    except BaseException as error:
        state['error'] = type(error).__name__+': '+str(error)
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        try:
            after = {name:c.sha(root/name) for name in manifest}
            c.save(output/'sources_after.json', after)
            state['sources_stable'] = bool(before) and before == after == manifest
            state['compilation_stable'] = compiled_after is not None and cc.close(root, build, before, compiled_before) == compiled_after
            if state['status'] == 'completed':
                need(state['sources_stable'] and state['compilation_stable'], 'closure changed')
        except BaseException as error:
            state['status'] = 'failed'
            state['closure_error'] = type(error).__name__+': '+str(error)
        state['commands'] = collector.rows
        state['useful_elapsed_seconds'] = time.monotonic()-(useful_end-USEFUL_SECONDS)
        c.save(output/'receipt.json', state)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    print(json.dumps(dict(status=state['status'], schema=SCHEMA, FULL_executed=False), sort_keys=True))
    return 0 if state['status'] == 'completed' else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source-root', 'source-manifest', 'guard-mark', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    for name in ('source-manifest-sha256', 'guard-mark-sha256', 'generation', 'project', 'zone', 'instance'):
        parser.add_argument('--'+name, required=True)
    parser.add_argument('--session-deadline-epoch', type=float, required=True)
    parser.add_argument('--closing-margin-seconds', type=int, required=True)
    raise SystemExit(execute(parser.parse_args()))
