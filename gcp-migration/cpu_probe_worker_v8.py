#!/usr/bin/env python3
"""Guest CPU candidate-stream pilot, inert without --execute; NOT GPU or FULL.

Explicit reuse of only guard/command primitives from the SHA-pinned v7 worker.
Its FULL producer, compilation recipe and result reader are never invoked.
No package installation, reboot, cloud mutation, CUDA or local ELF transport.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from copy import deepcopy
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import signal
import sys
import time
import types

TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b',
              instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
HELPER = 'gcp-migration/full_probe_worker_v7.py'
HELPER_SHA = 'da967163bdb7247bc6aad4df0c294cda1071076a0127cd5bd9f59bc0e4788439'
PLAN = 'data/session_plan.json'
LIBRARY = tuple('morsehgp3D_v8/src/' + name + '.cpp' for name in (
    'pipeline/local_credits', 'pipeline/axis_q2', 'pipeline/q2_census',
    'pipeline/q2_census_parallel', 'pipeline/prepared_cloud', 'wspd/front',
    'lanes/q4_family', 'lanes/exact_ball', 'lanes/q34_seed', 'lanes/edge_cover',
    'lanes/q34_cover', 'lanes/family_certificate', 'lanes/q34_pruning',
    'lanes/q34_collective', 'lanes/q4_positive_domain', 'lanes/q4_center_map',
    'lanes/q4_local_partition', 'lanes/q4_local', 'lanes/q4_shallow_set',
    'lanes/q4_shallow', 'lanes/q4_window', 'pipeline/wspd_q34'))
PROGRAMS = {'mhgp8_wspd_q34_gate': 'morsehgp3D_v8/tests/wspd_q34_gate.cpp',
            'mhgp8_wspd_q34_probe': 'morsehgp3D_v8/bench/wspd_q34_probe.cpp'}
# Included by the global probe as a library of readers/work tables, not a
# third executable. Keep its source pinned even when no stand-alone edge
# benchmark is requested. R1's GCC11 warning/failure remains archived.
SUPPORT_SOURCES = {'morsehgp3D_v8/bench/q4_lidar_probe.cpp'}


def gate_command(build):
    return [str(build / 'mhgp8_wspd_q34_gate'), '--selftest']


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def strict_json(raw):
    def pairs(items):
        out = {}
        for key, value in items:
            need(key not in out, 'duplicate JSON key')
            out[key] = value
        return out
    def invalid(_):
        raise ValueError('nonfinite JSON number')
    def finite(text):
        value = float(text)
        need(math.isfinite(value), 'nonfinite JSON number')
        return value
    return json.loads(raw, object_pairs_hook=pairs, parse_constant=invalid, parse_float=finite)


def safe_name(name):
    if type(name) is not str or not name:
        return False
    path = PurePosixPath(name)
    return (str(path) == name and not path.is_absolute() and
            all(part not in ('.', '..') for part in path.parts) and
            (name.startswith('morsehgp3D_v8/') or name.startswith('data/') or name == HELPER))


def validate_manifest(manifest):
    need(type(manifest) is dict and manifest, 'payload manifest object')
    for name, pin in manifest.items():
        need(safe_name(name) and type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin),
             'unsafe payload path/hash')
    need(set(LIBRARY) | set(PROGRAMS.values()) | SUPPORT_SOURCES | {HELPER, PLAN} <= set(manifest), 'required payload absent')
    need({name for name in manifest if name.startswith('morsehgp3D_v8/src/') and name.endswith('.cpp')} ==
         set(LIBRARY), 'library inventory differs from explicit 22-unit recipe')
    need(manifest[HELPER] == HELPER_SHA, 'legacy helper pin')


def validate_plan(plan, manifest):
    need(type(plan) is dict and set(plan) == {'schema', 'cases'} and
         plan['schema'] == 'mhgp8_cpu_probe_plan_v1' and type(plan['cases']) is list and plan['cases'], 'plan schema')
    for case in plan['cases']:
        need(type(case) is dict and set(case) == {'file', 'n', 'k', 's', 'mask', 'backend', 'workers'}, 'case fields')
        need(type(case['file']) is str and re.fullmatch(r'data/[A-Za-z0-9_.-]+\.u16le', case['file']) and
             case['file'] in manifest, 'case input not pinned u16le')
        need(all(type(case[key]) is int for key in ('n', 'k', 's', 'mask', 'backend', 'workers')), 'case integer types')
        need(case['n'] > 0 and case['k'] in (5, 10) and case['s'] in (8, 10, 12) and
             case['mask'] in (2, 4, 6) and case['backend'] in (28, 30) and 1 <= case['workers'] <= 48,
             'CPU pilot case domain')
    return plan['cases']


def source_map(root, manifest):
    validate_manifest(manifest)
    out = {}
    for name, pin in manifest.items():
        path = root / name
        need(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(root), 'payload file type')
        out[name] = sha(path)
        need(out[name] == pin, 'payload changed: ' + name)
    return out


def load_helper(root):
    path = root / HELPER
    raw = path.read_bytes()
    need(hashlib.sha256(raw).hexdigest() == HELPER_SHA, 'worker helper changed')
    module = types.ModuleType('mhgp8_pinned_worker_primitives')
    module.__file__ = str(path)
    exec(compile(raw, str(path), 'exec'), module.__dict__)
    return module


def validate_probe(value, case):
    need(type(value) is dict and value.get('schema') == 'mhgp8_wspd_q34_probe_v1' and
         value.get('status') == 'completed' and value.get('backend') == 'cpu_reference' and
         value.get('public_status') == 'not_claimed' and
         value.get('scope') == 'global_q3_q4_candidate_stream_not_catalogue_or_full' and
         value.get('front_mode') == 'samples' and value.get('output_mode') == 'digest', 'CPU candidate probe status')
    for key, field in [('n', 'n'), ('k', 'kmax'), ('s', 's'), ('mask', 'mask'),
                       ('backend', 'q4_backend'), ('workers', 'workers')]:
        need(type(value.get(field)) is int and value[field] == case[key], 'probe command/result mismatch')
    need(value['parallel']['completed_jobs'] == value['parallel']['jobs'] and
         value['output']['q3'] == value['work']['q3_emitted'] and
         value['output']['q4'] == value['work']['q4_emitted'] and
         value['output']['shell_ids'] == value['work']['payload_shell_ids'], 'probe completion ledger')


def logical_result(value):
    """Compare worker counts without treating private capacity as geometry."""
    out = {key: deepcopy(value[key]) for key in ('input_hash', 'output', 'front', 'work', 'cloud_work', 'index_work')}
    out['work']['peak_edge_buffer_bytes'] = 0
    out['work']['q3']['peak_shell_bytes'] = 0
    return out


def execute(args):
    root, output = args.source_root.resolve(), args.output.absolute()
    need(1 <= args.useful_budget_seconds <= 900 and args.closing_margin_seconds >= 300, 'cost/closing budget')
    need(not output.exists() and not output.is_symlink() and not output.resolve().is_relative_to(root), 'fresh output')
    output.mkdir(mode=0o700)
    helper = load_helper(root)
    result = dict(status='failed', backend='cpu_reference', scope='candidate_stream_not_FULL',
                  public_status='not_claimed', GPU_executed=False, FULL_executed=False,
                  contract_certified=False, targeted_GCP_stop_required_by_ROOT=True,
                  worker_argv=sys.argv, worker_sha256=sha(__file__))
    worker, manifest, before, binaries, consumed = None, None, None, {}, {}
    began = time.time()
    def interrupted(signum, _frame):
        raise InterruptedError('guest signal ' + str(signum))
    for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(signum, interrupted)
    try:
        need(sha(args.guard_mark) == args.guard_mark_sha256, 'guard pin')
        mark, schedule = helper.fields(args.guard_mark.read_text()), helper.fields(helper.scheduled_text())
        target = {key: getattr(args, key) for key in TARGET}
        need(target == TARGET and mark['max_run_seconds'] == '3600' and mark['guest_shutdown_minutes'] == '30',
             'fixed target and dual-guard duration')
        guards = helper.guard_values(mark, schedule, target, args.generation, args.session_deadline_epoch,
                                     args.closing_margin_seconds, time.time())
        observed = helper.metadata()
        need(all(observed[key] == value for key, value in TARGET.items()) and
             observed['machine'] == 'g4-standard-48', 'guest target/type')
        boot = time.time() - float(Path('/proc/uptime').read_text().split()[0])
        need(abs(boot-helper.epoch(args.generation)) <= 300, 'guest boot/generation')
        cpus = sorted(os.sched_getaffinity(0))
        need(len(cpus) == 48, '48 available vCPUs required')
        deadline = min(began + args.useful_budget_seconds, guards['work_deadline_epoch'])
        worker = helper.Worker(output, deadline, schedule)
        result.update(target=target, generation=args.generation, available_cpus=cpus, guards=guards,
                      useful_deadline_epoch=deadline, useful_budget_seconds=args.useful_budget_seconds)
        helper.save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule, metadata=observed))
        need(sha(args.source_manifest) == args.source_manifest_sha256, 'manifest pin')
        manifest = strict_json(args.source_manifest.read_text())
        result['source_manifest_sha256'] = args.source_manifest_sha256
        before = source_map(root, manifest)
        helper.save(output / 'sources_before.json', before)
        cases = validate_plan(strict_json((root / PLAN).read_text()), manifest)
        for case in cases:
            size = (root / case['file']).stat().st_size
            need(size % 6 == 0 and case['n'] <= size // 6, 'u16le payload/prefix size')
        compiler = shutil.which('g++')
        need(compiler and Path('/usr/bin/time').is_file() and
             Path('/usr/include/boost/multiprecision/cpp_int.hpp').is_file(),
             'g++, GNU time and system Boost required; no automatic installation')
        result['compiler_sha256'] = sha(compiler)
        for name, command in [('compiler', [compiler, '--version']), ('cpu', ['lscpu']),
                              ('time', ['/usr/bin/time', '--version'])]:
            need(worker.command(name, command)['exit_code'] == 0, name)
        build = output / 'build'
        build.mkdir(mode=0o700)
        flags = ['-O3', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                 '-pthread', '-I', str(root / 'morsehgp3D_v8/src')]
        objects = [build / (str(i) + '.o') for i in range(len(LIBRARY))]
        def compile_unit(item):
            i, source = item
            argv = [compiler, *flags, '-MD', '-MF', str(build / (str(i)+'.d')),
                    '-c', str(root / source), '-o', str(objects[i])]
            need(worker.command('compile_' + str(i), argv)['exit_code'] == 0, 'compile ' + source)
        # Eight compiler processes, never one compiler per 48 logical CPUs.
        # Every child uses the SAME total useful deadline and is joined.
        with ThreadPoolExecutor(max_workers=8) as pool:
            list(pool.map(compile_unit, enumerate(LIBRARY)))
        for name, source in PROGRAMS.items():
            binary = build / name
            argv = [compiler, *flags, '-MD', '-MF', str(build / (name+'.d')), str(root / source),
                    *map(str, objects), '-o', str(binary)]
            need(worker.command('link_' + name, argv)['exit_code'] == 0, 'link ' + name)
            binaries[name] = sha(binary)
        consumed = {}
        for dep in sorted(build.glob('*.d')):
            for name in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]):
                path = Path(name).resolve()
                if path.is_relative_to(root):
                    relative = str(path.relative_to(root))
                    need(relative in before and sha(path) == before[relative], 'unmanifested compilation dependency')
                consumed[str(path)] = sha(path)
        helper.save(output / 'compiled_dependencies.json', consumed)
        result['compiled_dependency_manifest_sha256'] = sha(output / 'compiled_dependencies.json')
        result['binaries'] = binaries
        gate = worker.command('gate92', gate_command(build))
        gate_json = strict_json((output / 'gate92.stdout').read_text())
        need(gate['exit_code'] == 0 and gate_json.get('schema') == 'mhgp8_wspd_q34_gate_v1' and
             gate_json.get('status') == 'PASS', 'independent gate92 failed; no large probe')
        paired = {}
        result['cross_worker_comparisons'] = 0
        for i, case in enumerate(cases):
            name = 'probe_' + str(i)
            argv = ['/usr/bin/time', '-v', str(build / 'mhgp8_wspd_q34_probe'), str(root / case['file']),
                    *[str(case[key]) for key in ('n', 'k', 's', 'mask', 'backend', 'workers')], 'samples', 'digest']
            row = worker.command(name, argv)
            need(row['exit_code'] == 0, 'probe failed: ' + name)
            value = strict_json((output / (name+'.stdout')).read_text())
            validate_probe(value, case)
            identity = (manifest[case['file']], *(case[key] for key in ('n', 'k', 's', 'mask', 'backend')))
            logical = logical_result(value)
            if identity in paired:
                need(logical == paired[identity], 'cross-worker payload or geometric work differs')
                result['cross_worker_comparisons'] += 1
            else:
                paired[identity] = logical
            helper.save(output / (name+'.summary.json'), dict(case=case, status='completed',
                        input_file_sha256=manifest[case['file']], output=value['output']))
        result['status'] = 'completed'
    except BaseException as error:
        result.update(error=type(error).__name__ + ': ' + str(error))
    finally:
        result['commands'] = worker.commands if worker else []
        result['elapsed_before_closure_seconds'] = time.time()-began
        try:
            if before is not None:
                after = source_map(root, manifest)
                helper.save(output / 'sources_after.json', after)
                result['sources_stable'] = before == after
            for name, pin in binaries.items():
                need(sha(output / 'build' / name) == pin, 'binary changed at closure')
            if 'compiler_sha256' in result:
                need(sha(compiler) == result['compiler_sha256'], 'compiler changed')
            for name, pin in consumed.items():
                need(sha(name) == pin, 'compiled dependency changed at closure')
            result['compiled_dependencies_stable'] = bool(consumed)
        except Exception as error:
            result.update(status='failed', closure_error=type(error).__name__ + ': ' + str(error))
        helper.save(output / 'receipt.json', result)
        print(json.dumps(dict(status=result['status'], targeted_GCP_stop_required_by_ROOT=True)))
    return 0 if result['status'] == 'completed' else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('source-root', 'source-manifest', 'guard-mark', 'output'):
        parser.add_argument('--'+key, type=Path)
    for key in ('source-manifest-sha256', 'guard-mark-sha256', 'project', 'zone', 'instance', 'generation'):
        parser.add_argument('--'+key)
    parser.add_argument('--session-deadline-epoch', type=int)
    parser.add_argument('--closing-margin-seconds', type=int, default=300)
    parser.add_argument('--useful-budget-seconds', type=int, default=900)
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps(dict(status='inert', target=TARGET, GPU_executed=False, FULL_executed=False)))
        return 0
    need(all(getattr(args, key.replace('-', '_')) is not None for key in (
        'source-root', 'source-manifest', 'guard-mark', 'output', 'source-manifest-sha256',
        'guard-mark-sha256', 'project', 'zone', 'instance', 'generation', 'session-deadline-epoch')), 'missing arguments')
    return execute(args)


if __name__ == '__main__':
    raise SystemExit(main())
