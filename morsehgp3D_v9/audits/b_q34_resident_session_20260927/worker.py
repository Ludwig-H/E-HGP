#!/usr/bin/env python3
# Explicit protocol port of b_q34_cuda_session_20260927 at a7e80d7f9.
# The pinned lifecycle helper, guards, fixed target and cooperative join are unchanged.
"""Guarded remote S2-only compilation and experiment. Never manages a VM."""
import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time

PREFIX = 'morsehgp3D_v9/audits/b_q34_resident_session_20260927'
SCHEMA = 'mhgp9_q34_resident_session_v1'
USEFUL_SECONDS = 360
GATE_POSITIVE = ('cases', 'portable_runs', 'cuda_runs', 'queries', 'survivors',
    'planned', 'fallbacks', 'empty', 'zero_output', 'reordered', 'pool_rejected',
    'pool_lane_reduced', 'fallback_survivors', 'closed', 'source_gaps', 'arena_gaps',
    'raw_holes', 'rectangle_waves', 'pair_waves', 'rejections')
TIMING_FIELDS = ('input', 'index', 'front', 'open', 'input_copy_validate', 'index_copy',
    'cuda_init', 'index_upload', 'rectangle_upload_allocate', 'rectangle_kernel',
    'rectangle_download_allocate', 'rectangle_release', 'compaction', 'input_release',
    'arena', 'consume', 'pair_upload_allocate', 'waves_including_count_download',
    'survivor_download_allocate', 'order_convert', 'pair_release', 'adapter_cleanup',
    'adapter', 'reference', 'comparison', 'destruction', 'total')
FRAME_UNSIGNED = ('physical_queries', 'Q_actual', 'waves', 'R', 'R_live', 'raw_pairs',
    'raw_q3', 'raw_q4', 'visits', 'rectangle_visits', 'rectangle_waves', 'planned',
    'fallbacks', 'decision_retained_bytes', 'prepared_retained_bytes',
    'before_arena_release_bytes', 'arena_owned_peak_bound', 'temporary_input_bytes',
    'resident_device_bytes', 'rectangle_device_bytes', 'pair_device_bytes',
    'rectangle_upload_bytes', 'rectangle_download_bytes', 'pair_upload_bytes', 'pair_download_bytes')


def recipes(root, build, c):
    binary = str(build / 'mhgp9_q34_filtered_resident')
    return dict(guest_schedule=['sudo', '-n', 'cat', '/run/systemd/shutdown/scheduled'],
        compiler=['g++', '--version'], cmake=['cmake', '--version'],
        nvcc=['/usr/local/cuda/bin/nvcc', '--version'], gpu_inventory=['nvidia-smi'], lscpu=['lscpu'],
        configure=['cmake', '-S', str(root / c.PROTOTYPE), '-B', str(build),
            '-DCMAKE_BUILD_TYPE=Release', '-DMHGP9_RESIDENT_ENABLE_CUDA=ON',
            '-DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc', '-DCMAKE_CUDA_ARCHITECTURES=120'],
        build=['cmake', '--build', str(build), '--target', 'mhgp9_q34_filtered_resident', '-j8'],
        device_gate=[binary, '--gate', '--cuda'],
        ng00_w4=[binary, '--frame', str(root / c.DATA), '--cuda', '--qr', '262144',
            '--q', '262144', '--workers', '4', '--k', '5', '--s', '8'],
        ng00_w48=[binary, '--frame', str(root / c.DATA), '--cuda', '--qr', '262144',
            '--q', '262144', '--workers', '48', '--k', '5', '--s', '8'])


def need(value, reason):
    if not value:
        raise ValueError(reason)


def validate_probe(value, mode, workers=4):
    need(mode in ('gate', 'frame') and type(value) is dict and value.get('schema') == 'mhgp9_q34_filtered_resident_v1' and
         value.get('status') == 'passed' and value.get('mode') == mode and
         value.get('cuda_executed') is True, 'CUDA probe verdict and scope')
    if mode == 'gate':
        for key in GATE_POSITIVE:
            need(type(value.get(key)) is int and value[key] > 0, 'positive device gate: ' + key)
        need(value['portable_runs'] == 3 * value['cases'] and
             2 * value['cases'] <= value['cuda_runs'] <= 3 * value['cases'], 'device gate run matrix')
    else:
        need(type(workers) is int and workers in (4, 48), 'qualified preparation widths')
        expected = dict(n=39885, k=5, s=8, Q_requested=262144, Qr_requested=262144,
                        input_hash_u64=9245360528374966039, P=23686751, E=9122704, S=2043612,
                        P3=17732794, P4=23446295, E3=6667094, E4=8403884,
                        digest_u64=5324876275161635233,
                        workers_arena=workers, workers_front=1, reference_workers=4)
        need(all(type(value.get(key)) is int and value[key] == item for key, item in expected.items()),
             'complete ng00 case binding and native masses')
        for key in FRAME_UNSIGNED:
            need(type(value.get(key)) is int and 0 <= value[key] < 2**64, 'unsigned counter: ' + key)
        need(type(value.get('device')) is str and value['device'], 'actual device name')
        need(value['physical_queries'] == value['E'] and 0 < value['Q_actual'] <= value['Q_requested'] and
             value['waves'] == (value['E'] + value['Q_actual'] - 1) // value['Q_actual'], 'exhaustive waves')
        need(0 < value['R_live'] <= value['R'] and value['planned'] + value['fallbacks'] == value['R_live'] and
             value['rectangle_waves'] == (value['R'] + value['Qr_requested'] - 1) // value['Qr_requested'],
             'exhaustive rectangle waves and compact partition')
        need(0 <= value['S'] <= value['E'] <= value['P'], 'P/E/S order')
        need(value['P'] <= value['raw_pairs'] and value['raw_pairs'] <= value['n'] * (value['n'] - 1) // 2,
             'native WSPD raw disjoint mass')
        for lane in ('3', '4'):
            need(0 <= value['E' + lane] <= value['P' + lane] <= value['P'] and
                 value['P' + lane] <= value['raw_q' + lane] <= value['raw_pairs'], 'lane masses')
        need(type(value.get('times_ms')) is dict and set(TIMING_FIELDS) == set(value['times_ms']), 'timing scope')
        for field, duration in value['times_ms'].items():
            need(type(duration) in (int, float) and math.isfinite(duration) and duration >= 0,
                 'finite timing: ' + field)
        t = value['times_ms']
        # Nonoverlapping outer phases only; nested timings must not be summed twice.
        need(t['open'] + t['arena'] + t['consume'] + t['adapter_cleanup'] <= t['adapter'] + 1e-6 and
             t['input'] + t['index'] + t['front'] + t['adapter'] + t['reference'] +
             t['comparison'] + t['destruction'] <= t['total'] + 1e-6, 'paid adapter and harness phases')
    return value


def run_probes(command, commands, c):
    """Ordered, fail-fast device qualification then both explicitly bound widths."""
    gate = validate_probe(json.loads(command('device_gate', commands['device_gate']),
                                    object_pairs_hook=c.unique), 'gate')
    measures = {}
    for workers in (4, 48):
        name = 'ng00_w' + str(workers)
        measures[str(workers)] = validate_probe(json.loads(command(name, commands[name]),
            object_pairs_hook=c.unique), 'frame', workers)
    return gate, measures


def dependencies(build, root, before, c):
    result, local = {}, set()
    depfiles = sorted(build.glob('CMakeFiles/*.dir/**/*.o.d'))
    need(depfiles, 'compiler dependency files')
    for path in depfiles:
        text = path.read_text().replace('\\\n', ' ')
        need(':' in text, 'dependency syntax')
        for name in shlex.split(text.split(':', 1)[1]):
            dependency = (Path(name) if Path(name).is_absolute() else build / name).resolve()
            pin = c.sha(dependency)
            if dependency.is_relative_to(root):
                relative = str(dependency.relative_to(root))
                need(before.get(relative) == pin, 'unmanifested/changed compiled source')
                local.add(relative)
            result[str(dependency)] = pin
    need({c.PROTOTYPE + '/probe.cpp', c.PROTOTYPE + '/device_cuda.cu',
          'morsehgp3D_v9/src/gpu/witness_filter.hpp'} <= local, 'actual probe and device predicate dependencies')
    return result


def execute(args):
    root, output = args.source_root.resolve(), args.output.absolute()
    manifest_raw = args.source_manifest.read_bytes()
    need(hashlib.sha256(manifest_raw).hexdigest() == args.source_manifest_sha256, 'manifest pin')
    manifest = json.loads(manifest_raw)
    common_path = root / PREFIX / 'common.py'
    need(hashlib.sha256(common_path.read_bytes()).hexdigest() == manifest[PREFIX + '/common.py'], 'common pin')
    spec = importlib.util.spec_from_file_location('cuda_waves_remote_common', common_path)
    c = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(c)
    manifest = c.read(args.source_manifest)
    need(c.sha(__file__) == manifest[PREFIX + '/worker.py'], 'worker pin')
    need(dict(project=args.project, zone=args.zone, instance=args.instance) == c.TARGET, 'fixed target')
    need(args.closing_margin_seconds >= 300 and not output.exists() and
         not output.is_symlink() and not output.resolve().is_relative_to(root), 'fresh output and margin')
    output.mkdir(mode=0o700)
    legacy = c.load_legacy(root)
    collector = legacy.Commands(output, dict(os.environ))
    state = dict(schema=SCHEMA, status='failed', scope='S2_only_no_FULL', FULL_executed=False,
                 public_status='not_claimed', contract_certified=False, target=c.TARGET,
                 generation=args.generation, useful_budget_seconds=USEFUL_SECONDS,
                 worker_sha256=c.sha(__file__), source_manifest_sha256=args.source_manifest_sha256,
                 CUDA_installation_attempted=False, commands=[])
    build = output.parent / 'resident_build'
    commands = recipes(root, build, c)
    before, consumed, binary_pin, binary = {}, {}, None, None
    previous = {}
    useful_end = time.monotonic() + USEFUL_SECONDS

    def remaining():
        value = min(useful_end - time.monotonic(), args.session_deadline_epoch -
                    args.closing_margin_seconds - time.time())
        need(value > 0, 'global useful/deadline budget exhausted')
        return value

    def command(name, argv):
        code, stdout, _ = collector.run(name, argv, timeout=remaining())
        need(code == 0, 'command failed: ' + name)
        return stdout

    def interrupted(signum, _frame):
        raise InterruptedError('worker signal ' + str(signum))

    try:
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            previous[sig] = signal.signal(sig, interrupted)
        need(c.sha(args.guard_mark) == args.guard_mark_sha256, 'guard mark pin')
        mark = legacy.fields(args.guard_mark.read_text())
        schedule = legacy.fields(command('guest_schedule', commands['guest_schedule']))
        deadline = legacy.guard_deadline(mark, schedule, args.generation, time.time())
        need(mark['max_run_seconds'] == '3600' and deadline == args.session_deadline_epoch, 'same guard/deadline')
        c.save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule))
        before = {name: c.sha(root / name) for name in manifest}
        need(before == manifest and c.sha(root / c.DATA) == c.DATA_PIN, 'source/input inventory')
        c.save(output / 'sources_before.json', before)
        state['provenance'] = c.read(root / c.PROVENANCE)
        for name in ('compiler', 'cmake', 'nvcc', 'gpu_inventory', 'lscpu'):
            command(name, commands[name])
        need(not build.exists(), 'fresh remote build')
        command('configure', commands['configure'])
        command('build', commands['build'])
        binary = build / 'mhgp9_q34_filtered_resident'
        binary_pin = c.sha(binary)
        consumed = dependencies(build, root, before, c)
        c.save(output / 'compiled_dependencies.json', consumed)
        state['compiled_dependencies_sha256'] = c.sha(output / 'compiled_dependencies.json')
        state['binary_sha256'] = binary_pin
        state['gate'], state['measures'] = run_probes(command, commands, c)
        state['status'] = 'completed'
    except BaseException as error:
        state['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        try:
            after = {name: c.sha(root / name) for name in manifest}
            c.save(output / 'sources_after.json', after)
            state['sources_stable'] = bool(before) and before == after == manifest
            state['compiled_dependencies_stable'] = bool(consumed) and all(c.sha(path) == pin for path, pin in consumed.items())
            state['binary_stable'] = binary is not None and c.sha(binary) == binary_pin
            if state['status'] == 'completed':
                need(all(state[key] for key in ('sources_stable', 'compiled_dependencies_stable', 'binary_stable')),
                     'closure changed')
        except BaseException as error:
            state['status'] = 'failed'
            state['closure_error'] = type(error).__name__ + ': ' + str(error)
        state['commands'] = collector.rows
        state['useful_elapsed_seconds'] = time.monotonic() - (useful_end - USEFUL_SECONDS)
        c.save(output / 'receipt.json', state)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    print(json.dumps(dict(status=state['status'], schema=SCHEMA, FULL_executed=False), sort_keys=True))
    return 0 if state['status'] == 'completed' else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source-root', 'source-manifest', 'guard-mark', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ('source-manifest-sha256', 'guard-mark-sha256', 'generation', 'project', 'zone', 'instance'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--session-deadline-epoch', type=float, required=True)
    parser.add_argument('--closing-margin-seconds', type=int, required=True)
    raise SystemExit(execute(parser.parse_args()))
