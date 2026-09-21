#!/usr/bin/env python3
"""Spatial34 CPU candidate-stream worker, inert without --execute; NOT GPU/FULL.

Explicit port of cpu_probe_worker_v8.py SHA
b8385feb20c82485fc4347c45157e58abff786060cbd57a1a73355e3c6f073cd.
The 24-unit library and schema4 are new; lifecycle/command primitives stay pinned.

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
import struct
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
    'lanes/q4_shallow', 'lanes/q4_window', 'pipeline/wspd_q34',
    'lanes/q34_witness_search', 'lanes/q3_ball_census'))
GATES = ('mhgp8_q4_seed_cells_gate', 'mhgp8_wspd_q34_gate', 'mhgp8_q4_local_gate', 'mhgp8_q34_pair_bounds_gate')
PROGRAMS = {name: 'morsehgp3D_v8/tests/' + name.removeprefix('mhgp8_') + '.cpp' for name in GATES}
PROGRAMS['mhgp8_wspd_q34_probe'] = 'morsehgp3D_v8/bench/wspd_q34_probe.cpp'
PROTOCOL_NAMES = frozenset('gcp-migration/q34_spatial_' + name + '_v8.py' for name in
    ('worker', 'session', 'snapshot', 'selftest'))
SPATIAL_READER = 'morsehgp3D_v8/bench/run_q34_spatial.py'
PREPARER = 'morsehgp3D_v8/bench/prepare_lidar_spatial.py'
PREPARATIONS = 'data/preparations.json'
AUTHORITY_MANIFEST = 'data/native_authority_manifest.json'
AUTHORITY_COMPLETION = 'data/native_authority_completion.json'
AUTHORITY_PINS = {
    AUTHORITY_MANIFEST: 'b4682fd3f59c9cb22266f8135c0f8ce08d256fc77c7c986eb8d3c580a1d1b0f0',
    AUTHORITY_COMPLETION: '282f048be8a8d0025f456a70b551e04dadcacb9411a0914ca7d1d104bcfae80f'}
DATASETS = ('full', 'half_x_neg', 'half_x_nonneg', 'quarter_x_neg_y_neg',
    'quarter_x_neg_y_nonneg', 'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
# Included by the global probe as a library of readers/work tables, not a
# third executable. Keep its source pinned even when no stand-alone edge
# benchmark is requested. R1's GCC11 warning/failure remains archived.
SUPPORT_SOURCES = {'morsehgp3D_v8/bench/q4_lidar_probe.cpp'}


def gate_command(build, name):
    need(name in GATES, 'unknown spatial gate')
    return [str(build / name), '--selftest']


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
            (name.startswith('morsehgp3D_v8/') or name.startswith('data/') or name == HELPER or name in PROTOCOL_NAMES))


def validate_manifest(manifest):
    need(type(manifest) is dict and manifest, 'payload manifest object')
    for name, pin in manifest.items():
        need(safe_name(name) and type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin),
             'unsafe payload path/hash')
    need(set(LIBRARY) | set(PROGRAMS.values()) | SUPPORT_SOURCES | PROTOCOL_NAMES |
         {HELPER, PLAN, PREPARATIONS, SPATIAL_READER, PREPARER, *AUTHORITY_PINS} <= set(manifest), 'required payload absent')
    need({name for name in manifest if name.startswith('morsehgp3D_v8/src/') and name.endswith('.cpp')} ==
         set(LIBRARY), 'library inventory differs from explicit 24-unit recipe')
    need(manifest[HELPER] == HELPER_SHA, 'legacy helper pin')
    need(all(manifest[name] == pin for name, pin in AUTHORITY_PINS.items()), 'closed native216 authority pins')


def validate_plan(plan, manifest):
    need(type(plan) is dict and set(plan) == {'schema', 'cases'} and
         plan['schema'] == 'mhgp8_q34_spatial_plan_v1' and type(plan['cases']) is list and plan['cases'], 'spatial plan schema')
    seen = set()
    for case in plan['cases']:
        need(type(case) is dict and set(case) == {'scene', 'dataset', 'file', 'n', 'input_hash', 'k', 's', 'workers', 'repeat'},
             'spatial case fields')
        need(type(case['scene']) is str and re.fullmatch(r'[A-Za-z0-9_-]+', case['scene']) and
             type(case['dataset']) is str and case['dataset'] in DATASETS and
             case['file'] == 'data/' + case['scene'] + '/' + case['dataset'] + '.u16le' and case['file'] in manifest,
             'spatial scene/dataset identity')
        need(all(type(case[key]) is int for key in ('n', 'input_hash', 'k', 's', 'workers', 'repeat')), 'case integer types')
        need(0 <= case['n'] < (1 << 64) and 0 <= case['input_hash'] < (1 << 64) and case['k'] in (5,10) and
             case['s'] in (8,10,12) and 0 < case['workers'] < (1 << 64) and 0 <= case['repeat'] < (1 << 64),
             'spatial case domain')
        identity = tuple(case[key] for key in ('scene', 'dataset', 'k', 's', 'workers', 'repeat'))
        need(identity not in seen, 'duplicate case needs an explicit distinct repetition')
        seen.add(identity)
    return plan['cases']


def input_hash(raw):
    need(len(raw) % 6 == 0, 'u16le incomplete coordinate record')
    h = 14695981039346656037
    def word(value):
        nonlocal h
        for byte in value.to_bytes(8, 'little'):
            h = ((h ^ byte) * 1099511628211) & ((1 << 64)-1)
    word(len(raw)//6)
    for point in struct.iter_unpack('<HHH', raw):
        for coordinate in point:
            word(coordinate)
    return h


def validate_data(cases, read_bytes):
    for case in cases:
        raw = read_bytes(case['file'])
        need(len(raw) == 6*case['n'] and input_hash(raw) == case['input_hash'],
             'whole spatial physical length/FNV differs; prefixes forbidden')


def validate_authority(manifest, read_bytes):
    for name, pin in AUTHORITY_PINS.items():
        need(hashlib.sha256(read_bytes(name)).hexdigest() == pin, 'native authority artifact changed')
    before, after = (strict_json(read_bytes(name)) for name in (AUTHORITY_MANIFEST, AUTHORITY_COMPLETION))
    native = before['source_sha256']
    need(len(native) == 216 and after['status'] == 'passed' and after['closing_errors'] == [] and
         after['source_sha256_after'] == native and after['manifest_sha256'] == AUTHORITY_PINS[AUTHORITY_MANIFEST] and
         all(manifest.get(name) == pin for name,pin in native.items()), 'native216 authority/source coupling')


def validate_runtime(manifest):
    # The executable is uploaded as remote/worker.py, outside source/. Pin
    # that actual code as well as the archived source copy.
    need(sha(__file__) == manifest.get('gcp-migration/q34_spatial_worker_v8.py'),
         'executing worker differs from transported worker source')


def load_validator(root=None, manifest=None):
    root = Path(root) if root is not None else Path(__file__).resolve().parent.parent
    if manifest is not None:
        for name, pin in manifest.items():
            if name.startswith('morsehgp3D_v8/') and name.endswith('.py'):
                need(sha(root / name) == pin, 'local validation dependency differs: ' + name)
    sys.path.insert(0, str(root / 'morsehgp3D_v8/bench'))
    import run_q34_spatial
    return run_q34_spatial


def validate_preparations(manifest, cases, read_bytes, validator):
    """Reconstruct copied raw scenes; original absolute provenance is not rewritten.

    This is an explicit relocation reader, not preparation.read() with a fake
    local path. The original manifest, completion and all 15 binary payloads
    must equal the exact raw reconstruction with the pinned preparer.
    """
    from datetime import datetime, timezone
    prep = validator.preparation
    table = strict_json(read_bytes(PREPARATIONS))
    need(type(table) is dict and set(table) == {'schema', 'scenes'} and
         table['schema'] == 'mhgp8_q34_spatial_preparations_v1' and
         type(table['scenes']) is list and table['scenes'], 'preparation table schema')
    seen = set()
    for entry in table['scenes']:
        need(type(entry) is dict and set(entry) == {'scene', 'original_directory'} and
             type(entry['scene']) is str and re.fullmatch(r'[A-Za-z0-9_-]+', entry['scene']) and
             entry['scene'] not in seen and type(entry['original_directory']) is str and
             Path(entry['original_directory']).is_absolute(), 'scene provenance')
        scene = entry['scene']
        seen.add(scene)
        prefix = 'data/' + scene + '/'
        raw = read_bytes(prefix + 'RAW.bin')
        metadata, payloads = prep.reconstruct(raw)
        manifest_raw = read_bytes(prefix + 'MANIFEST.json')
        m = strict_json(manifest_raw)
        need(type(m.get('raw')) is dict and type(m['raw'].get('path')) is str and
             Path(m['raw']['path']).is_absolute(), 'raw original path')
        expected = prep._manifest(Path(m['raw']['path']), raw, manifest[PREPARER], metadata)
        need(manifest_raw == prep.canonical_json(expected), 'relocated preparation manifest differs')
        for name, value in payloads.items():
            need(read_bytes(prefix + name) == value, 'relocated payload differs: ' + name)
        c_raw = read_bytes(prefix + 'COMPLETION.json')
        c = strict_json(c_raw)
        started, finished = (datetime.fromisoformat(c[key]) for key in ('started_utc', 'finished_utc'))
        need(started.utcoffset() == finished.utcoffset() == timezone.utc.utcoffset(None) and
             started <= finished, 'preparation UTC ordering')
        expected_c = dict(schema=prep.COMPLETION_SCHEMA, status='passed', started_utc=c['started_utc'],
            finished_utc=c['finished_utc'], error=None, manifest_sha256=prep.sha256(manifest_raw),
            raw_sha256_after=prep.sha256(raw), script_sha256_after=manifest[PREPARER],
            output_sha256={name: prep.sha256(value) for name,value in payloads.items()})
        need(c_raw == prep.canonical_json(expected_c), 'relocated preparation closure differs')
        names = {prefix + name for name in (*payloads, 'MANIFEST.json', 'COMPLETION.json', 'RAW.bin')}
        need({name for name in manifest if name.startswith(prefix)} == names, 'scene artifact inventory')
    need({case['scene'] for case in cases} <= seen, 'case has no verified whole-scene preparation')


def probe_command(build, root, case):
    return [str(build / 'mhgp8_wspd_q34_probe'), str(root / case['file']), str(case['n']),
        str(case['k']), str(case['s']), '6', '28', str(case['workers']), 'samples', 'digest',
        'rectangle-pair', 'boxes', 'affine', 'live', '64']


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


def validate_probe(value, case, command, validator):
    need(command[1].endswith('/' + case['file']) and command[2:] ==
         [str(case['n']), str(case['k']), str(case['s']), '6', '28', str(case['workers']),
          'samples', 'digest', 'rectangle-pair', 'boxes', 'affine', 'live', '64'], 'exact spatial probe invocation')
    validator.validate_row(value, command, dict(name=case['dataset'], source=command[1],
        n=case['n'], input_hash=case['input_hash']))


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
        validate_runtime(manifest)
        result['source_manifest_sha256'] = args.source_manifest_sha256
        before = source_map(root, manifest)
        helper.save(output / 'sources_before.json', before)
        validate_authority(manifest, lambda name: (root / name).read_bytes())
        cases = validate_plan(strict_json((root / PLAN).read_text()), manifest)
        validate_data(cases, lambda name: (root / name).read_bytes())
        validator = load_validator(root, manifest)
        validate_preparations(manifest, cases, lambda name: (root / name).read_bytes(), validator)
        result.update(plan_schema='mhgp8_q34_spatial_plan_v1', cases=cases, empty_cases=[],
                      witness_mode='rectangle-pair', q3_census_mode='boxes', witness_bounds_mode='affine',
                      q4_seed_mode='live', q4_seed_block_size=64, completed_case_indices=[])
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
        for gate_index, gate_name in enumerate(GATES):
            label = 'gate_' + str(gate_index)
            gate = worker.command(label, gate_command(build, gate_name))
            need(gate['exit_code'] == 0, 'native gate failed before scientific probes')
            validator.checks.validate_gate(strict_json((output / (label + '.stdout')).read_text()), gate_name)
        paired = {}
        result['cross_worker_comparisons'] = 0
        for i, case in enumerate(cases):
            name = 'probe_' + str(i)
            if case['n'] == 0:
                result['empty_cases'].append(i)
                continue
            native_command = probe_command(build, root, case)
            argv = ['/usr/bin/time', '-v', *native_command]
            row = worker.command(name, argv)
            need(row['exit_code'] == 0, 'probe failed: ' + name)
            value = strict_json((output / (name+'.stdout')).read_text())
            validate_probe(value, case, native_command, validator)
            result['completed_case_indices'].append(i)
            identity = (manifest[case['file']], *(case[key] for key in ('n', 'k', 's')))
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
