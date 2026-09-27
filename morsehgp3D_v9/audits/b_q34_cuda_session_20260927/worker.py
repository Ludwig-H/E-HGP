#!/usr/bin/env python3
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

PREFIX = 'morsehgp3D_v9/audits/b_q34_cuda_session_20260927'
SCHEMA = 'mhgp9_cuda_waves_session_v1'
USEFUL_SECONDS = 360


def recipes(root, build, c):
    binary = str(build / 'mhgp9_q34_cuda_waves')
    return dict(guest_schedule=['sudo', '-n', 'cat', '/run/systemd/shutdown/scheduled'],
        compiler=['g++', '--version'], cmake=['cmake', '--version'],
        nvcc=['/usr/local/cuda/bin/nvcc', '--version'], gpu_inventory=['nvidia-smi'], lscpu=['lscpu'],
        configure=['cmake', '-S', str(root / c.PROTOTYPE), '-B', str(build),
            '-DCMAKE_BUILD_TYPE=Release', '-DMHGP9_CUDA_WAVES_ENABLE_CUDA=ON',
            '-DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc', '-DCMAKE_CUDA_ARCHITECTURES=120'],
        build=['cmake', '--build', str(build), '--target', 'mhgp9_q34_cuda_waves', '-j8'],
        device_gate=[binary, '--gate', '--cuda'],
        ng00=[binary, '--frame', str(root / c.DATA), '--cuda', '--q=262144', '--k=5', '--s=8'])


def need(value, reason):
    if not value:
        raise ValueError(reason)


def validate_probe(value, mode):
    need(type(value) is dict and value.get('schema') == 'mhgp9_q34_cuda_waves_v1' and
         value.get('status') == 'passed' and value.get('mode') == mode and
         value.get('cuda_executed') is True, 'CUDA probe verdict and scope')
    if mode == 'gate':
        for key in ('cases', 'runs', 'queries', 'survivors', 'planned', 'fallbacks', 'empty',
                    'zero_output', 'reordered', 'mixed_masks', 'pool_rejected',
                    'pool_lane_reduced', 'fallback_survivors'):
            need(type(value.get(key)) is int and value[key] > 0, 'positive device gate: ' + key)
    else:
        expected = dict(n=39885, k=5, s=8, Q_requested=262144,
                        input_hash_u64=9245360528374966039, P=23686751, E=9122704, S=2043612,
                        preparation_workers=4, reference_workers=4)
        need(all(type(value.get(key)) is int and value[key] == item for key, item in expected.items()),
             'complete ng00 case binding and native masses')
        for key in ('physical_queries', 'Q_actual', 'waves', 'P3', 'P4', 'E3', 'E4', 'digest_u64'):
            need(type(value.get(key)) is int and 0 <= value[key] < 2**64, 'unsigned counter: ' + key)
        need(type(value.get('device')) is str and value['device'], 'actual device name')
        need(value['physical_queries'] == value['E'] and 0 < value['Q_actual'] <= value['Q_requested'] and
             value['waves'] == (value['E'] + value['Q_actual'] - 1) // value['Q_actual'], 'exhaustive waves')
        need(0 <= value['S'] <= value['E'] <= value['P'], 'P/E/S order')
        for lane in ('3', '4'):
            need(0 <= value['E' + lane] <= value['P' + lane] <= value['P'], 'lane masses')
        need(type(value.get('times_ms')) is dict and {'input', 'index', 'front', 'preparation',
             'snapshot', 'cuda_runner', 'upload_allocate', 'waves_including_count_download',
             'survivor_download_allocate', 'order_convert', 'runner_release', 'reference',
             'comparison', 'destruction', 'total'} == set(value['times_ms']), 'timing scope')
        for field, duration in value['times_ms'].items():
            need(type(duration) in (int, float) and math.isfinite(duration) and duration >= 0,
                 'finite timing: ' + field)
    return value


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
    need({c.PROTOTYPE + '/probe.cpp', c.PROTOTYPE + '/runner.cu',
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
    build = output.parent / 'cuda_waves_build'
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
        binary = build / 'mhgp9_q34_cuda_waves'
        binary_pin = c.sha(binary)
        consumed = dependencies(build, root, before, c)
        c.save(output / 'compiled_dependencies.json', consumed)
        state['compiled_dependencies_sha256'] = c.sha(output / 'compiled_dependencies.json')
        state['binary_sha256'] = binary_pin
        state['gate'] = validate_probe(json.loads(command('device_gate', commands['device_gate']), object_pairs_hook=c.unique), 'gate')
        state['measure'] = validate_probe(json.loads(command('ng00', commands['ng00']), object_pairs_hook=c.unique), 'frame')
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
