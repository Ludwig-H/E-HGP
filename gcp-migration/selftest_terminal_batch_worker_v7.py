#!/usr/bin/env python3
"""Synthetic parser/policy boundaries only; never a geometric/device result."""
import copy
import importlib.util
import itertools
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('tested_terminal_worker', Path(__file__).with_name('terminal_batch_worker_v7.py'))
w = importlib.util.module_from_spec(spec)
spec.loader.exec_module(w)
checks = 0
rejections = 0


def need(ok, why):
    global checks
    checks += 1
    if not ok:
        raise ValueError(why)


def reject(function, why):
    global rejections
    try:
        function()
    except (ValueError, KeyError, TypeError, OSError):
        rejections += 1
    else:
        raise ValueError('mutant survived: ' + why)


def synthetic_probe(batch):
    row = {key: 0 for key in w.IDENTICAL + w.CAPACITIES}
    row.update(schema='mhgp7-full-ball-tower-probe-v1', status='completed_relative', public_status='not_claimed',
        backend=w.BACKEND if batch else 'cpu_reference', contract_qualified=False,
        n=200, s=8, kmax=10, orders=10, threads=48, static_threads=1, seed=3, coord=65536, balls=1,
        input_digest='a' * 64, payload_digest='b' * 64, resolver_supports_by_size=[0, 0, 9, 0, 0],
        resolver_supports_tested=9, resolver_meb_calls=9, resolver_materializations=9, resolver_power_tests=9,
        residence_accounting='release_dead_construction_before_population_copy_v1',
        resolver_cache_accounting='static_exact_sort_unique_complete_population_seeds_after_exchange_v2',
        static_worker_threads_created_in_completed_pools=0, static_lanes_used=0,
        static_orders=[dict(K=k, requests=int(k > 1), unique=int(k > 1), seeded_unique=0,
                           post_seed_queries=0, post_seed_hits=0, post_seed_terminals=0) for k in range(1, 11)])
    row.update({key: 0.1 for key in w.TIMES})
    row.update(tower_s=2.0, total_s=3.0)
    trace = {key: 0 for key in w.TRACE_COUNTS}
    trace.update({key: 0.0 for key in w.TRACE_TIMES})
    trace.update(selected=bool(batch), transport_kind='device_transport', device_executed=bool(batch),
        callback_work_scope='terminal_requests_only', builder_remainder_is_pure_calendar=False,
        close_certified=True, capacity_is_VRAM_peak=False, work_known=True, builder_s=1.0,
        callbacks_s=0.5 if batch else 0.0, builder_excluding_callbacks_s=0.5 if batch else 1.0)
    if batch:
        trace.update(context_capture_count=1, close_calls=1, launches=9, synchronizations=9,
                     allocations=1, allocation_bytes_total=1, free_attempts=1, certified_free_bytes=1,
                     h2d_bytes=1, d2h_bytes=1, initialization_bytes=1)
    trace['callback_orders'] = [dict(K=k, callbacks=int(batch and k > 1), callback_s=0.0,
        work_known=True, meb_calls=int(batch and k > 1), powers=int(batch and k > 1),
        materializations=int(batch and k > 1), supports_by_size=[0, 0, int(batch and k > 1), 0, 0]) for k in range(1, 11)]
    row['batch_geometry'] = trace
    return row


def tool_discovery_fixtures():
    """Synthetic readers only; no host tools, permissions or process queried."""
    def discover(paths=None, files=None, executable=None, resolved=None):
        paths = dict({'nvcc': '/path/nvcc', 'g++': '/usr/bin/g++', 'nvidia-smi': '/path/nvidia-smi'}
                     if paths is None else paths)
        files = set(['/usr/bin/time'] if files is None else files)
        executable = set([] if executable is None else executable)
        resolved = {} if resolved is None else resolved
        return w.discover_existing_tools(which=paths.get, is_file=lambda path: path in files,
                                        executable=lambda path: path in executable,
                                        resolve=lambda path: resolved.get(path, path))

    nominal = discover()
    need(w.require_existing_tools(nominal) == ('/path/nvcc', '/usr/bin/g++', '/path/nvidia-smi')
         and nominal['missing'] == [] and nominal['version_checks_performed'] is False
         and nominal['installation_attempted'] is False, 'tool discovery nominal, no versions or install')
    need(nominal['cuda_fallback_candidates'] == [dict(path=path, checked=False, is_file=None, executable=None)
                                              for path in w.CUDA_PATHS], 'both historical CUDA candidates retained')
    all_files = {'/usr/bin/time', *w.CUDA_PATHS}
    priority = discover(files=all_files, executable=set(w.CUDA_PATHS))
    need(priority['selected']['nvcc'] == '/path/nvcc', 'nvcc PATH wins over both fallbacks')
    no_path_nvcc = {'g++': '/usr/bin/g++', 'nvidia-smi': '/path/nvidia-smi'}
    for index, path in enumerate(w.CUDA_PATHS):
        observation = discover(paths=no_path_nvcc, files=all_files, executable=set(w.CUDA_PATHS[index:]))
        need(w.require_existing_tools(observation)[0] == path, 'ordered usable CUDA fallback')
    alias = discover(paths=dict(no_path_nvcc, nvcc='/path/nvcc', **{'g++': '/alias/g++'}),
                     resolved={'/alias/g++': '/compiler/g++12', '/usr/bin/g++': '/compiler/g++12'})
    need(w.require_existing_tools(alias)[1] == '/alias/g++', 'same canonical compiler accepted')
    missing_cases = []
    for name in ('nvcc', 'g++', 'nvidia-smi'):
        paths = {'nvcc': '/path/nvcc', 'g++': '/usr/bin/g++', 'nvidia-smi': '/path/nvidia-smi'}
        del paths[name]
        missing_cases.append((name, discover(paths=paths)))
    missing_cases.append(('/usr/bin/time', discover(files=set())))
    for name, observation in missing_cases:
        retained = dict(tool_discovery=observation)
        try:
            w.require_existing_tools(retained['tool_discovery'])
        except ValueError as error:
            need(str(error) == 'existing tools required, no installation; missing: ' + name
                 and retained['tool_discovery']['missing'] == [name], 'precise missing tool retained before refusal')
        else:
            raise ValueError('missing tool admitted: ' + name)
        reject(lambda observation=observation: w.require_existing_tools(observation), 'missing tool ' + name)
    all_missing = discover(paths={}, files=set())
    need(all_missing['missing'] == ['nvcc', 'g++', 'nvidia-smi', '/usr/bin/time'], 'all missing tools enumerated')
    reject(lambda: w.require_existing_tools(all_missing), 'all tools absent')
    for files, access in ((all_files, set()), ({'/usr/bin/time'}, set(w.CUDA_PATHS))):
        observation = discover(paths=no_path_nvcc, files=files, executable=access)
        need(observation['missing'] == ['nvcc'], 'fallback still requires file and executable')
        reject(lambda observation=observation: w.require_existing_tools(observation), 'unusable CUDA fallback')
    mismatch = discover(paths=dict(no_path_nvcc, nvcc='/path/nvcc', **{'g++': '/other/g++'}))
    need(mismatch['missing'] == [] and mismatch['compiler_binding']['resolved'] == '/other/g++',
         'compiler mismatch recorded separately from absence')
    reject(lambda: w.require_existing_tools(mismatch), 'different strict adapter compiler')
    # Exhaust every availability/binding combination against the original
    # 043197 boolean admission and nvcc priority, not a relaxed new policy.
    combinations = 0
    for nvcc, compiler, smi, time_file, file1, exec1, file2, exec2, same_compiler in itertools.product((False, True), repeat=9):
        paths = dict(nvcc='/path/nvcc' if nvcc else None, **{'g++': '/path/g++' if compiler else None,
                     'nvidia-smi': '/path/nvidia-smi' if smi else None})
        files = ({'/usr/bin/time'} if time_file else set()) | ({w.CUDA_PATHS[0]} if file1 else set()) | \
                ({w.CUDA_PATHS[1]} if file2 else set())
        accesses = ({w.CUDA_PATHS[0]} if exec1 else set()) | ({w.CUDA_PATHS[1]} if exec2 else set())
        resolution = {'/path/g++': '/compiler/g++' if same_compiler else '/other/g++',
                      '/usr/bin/g++': '/compiler/g++'}
        observation = discover(paths=paths, files=files, executable=accesses, resolved=resolution)
        expected_nvcc = paths['nvcc'] or (w.CUDA_PATHS[0] if file1 and exec1 else
                                        w.CUDA_PATHS[1] if file2 and exec2 else None)
        expected_admission = bool(expected_nvcc and compiler and smi and time_file and same_compiler)
        try:
            w.require_existing_tools(observation)
            admitted = True
        except ValueError:
            admitted = False
        w.need(admitted == expected_admission and observation['selected']['nvcc'] == expected_nvcc,
               'historical 043197 tool admission differs')
        combinations += 1
    need(combinations == 512, 'complete finite admission differential')


def main():
    root = Path(__file__).resolve().parents[1]
    need(w.sha(root / 'gcp-migration/full_probe_worker_v7.py') == w.SUPPORT_SHA256, 'support pin')
    need(w.sha(root / w.STRICT_HOST) == w.STRICT_HOST_SHA256, 'adapter pin')
    cpu, gpu = synthetic_probe(0), synthetic_probe(1)
    parse = lambda row, batch=1, static_threads=1: w.probe_summary(json.dumps(row), 0, 200, 10, 8, batch,
                                                                 static_threads=static_threads)
    need(parse(gpu) == gpu and parse(cpu, 0) == cpu, 'synthetic nominal CPU/device wires')
    need(w.compare(cpu, gpu)['equal_RUSQHT'], 'synthetic paired work')
    cpu48 = copy.deepcopy(cpu)
    cpu48['static_threads'] = 48
    need(parse(cpu48, 0, 48) == cpu48 and w.compare(cpu48, gpu)['status'] == 'passed', 'CPU48/GPU1 distinct configuration')
    for field in gpu:
        bad = copy.deepcopy(gpu)
        del bad[field]
        reject(lambda bad=bad: parse(bad), 'missing top field ' + field)
    for field, value in [('status', 'partial'), ('backend', 'cpu_reference'), ('contract_qualified', True),
                         ('n', True), ('threads', 1), ('static_threads', 48), ('payload_digest', 'bad'),
                         ('resolver_meb_calls', 10), ('resolver_supports_tested', 8), ('total_s', 0),
                         ('tower_s', -1), ('resolver_supports_by_size', [True, 0, 9, 0, 0])]:
        bad = copy.deepcopy(gpu)
        bad[field] = value
        reject(lambda bad=bad: parse(bad), 'bad top field')
    for field, value in [('device_executed', False), ('device_executed', 1), ('transport_kind', 'host_emulated_transport'),
                         ('work_known', False), ('close_certified', False), ('free_failures', 1),
                         ('certified_free_bytes', 0), ('capacity_is_VRAM_peak', True),
                         ('builder_remainder_is_pure_calendar', True), ('launches', 0), ('h2d_bytes', 0)]:
        bad = copy.deepcopy(gpu)
        bad['batch_geometry'][field] = value
        reject(lambda bad=bad: parse(bad), 'false/partial GPU or cleanup')
    for field in gpu['batch_geometry']:
        bad = copy.deepcopy(gpu)
        del bad['batch_geometry'][field]
        reject(lambda bad=bad: parse(bad), 'missing trace field')
    for field in w.IDENTICAL:
        bad = copy.deepcopy(gpu)
        value = bad[field]
        bad[field] = value + 1 if type(value) is int else None
        reject(lambda bad=bad: w.compare(cpu, bad), 'deterministic mismatch ' + field)
    for target in ('static_orders', 'callback_orders'):
        bad = copy.deepcopy(gpu)
        orders = bad[target] if target == 'static_orders' else bad['batch_geometry'][target]
        orders[-1]['K'] = 9
        reject(lambda bad=bad: parse(bad), 'duplicate K')
    other_s = copy.deepcopy(gpu)
    other_s.update(s=10, raw=123, unique=456)
    need(w.compare_s(gpu, other_s)['status'] == 'passed', 'candidate counts independent across s')
    other_s['payload_digest'] = 'c' * 64
    reject(lambda: w.compare_s(gpu, other_s), 'inter-s payload mismatch')
    reject(lambda: w.compare_s(gpu, gpu), 'inter-s identical s')
    raw = json.dumps(gpu)
    for malformed in ('', raw + '\n' + raw, '[]', 'null', raw.replace('{', '{"n":0,', 1),
                      raw.replace('{', '{"nested":{"a":0,"a":1},', 1)):
        reject(lambda malformed=malformed: w.probe_summary(malformed, 0, 200, 10, 8, 1), 'JSON shape')
    for value in ('NaN', 'Infinity', '-Infinity', '1e999'):
        reject(lambda value=value: w.strict_json('{"x":' + value + '}'), 'nonfinite')
    for code in (1, 2, -9, False, None):
        reject(lambda code=code: w.probe_summary(raw, code, 200, 10, 8, 1), 'failed process')
    expected = [(0, 0, 0), (201, 43, 7), (515, 120, 48), (820, 150, 42), (984, 204, 48),
                (1224, 336, 84), (1482, 402, 72), (1590, 438, 120), (1752, 546, 150), (1758, 672, 144)]
    gate = dict(status='passed', scope='bounded_real_census_batch_FULL', backend=w.BACKEND,
        device_executed=True, public_status='not_claimed', checks=372537, fixtures=10, physical_pairs=20,
        nodes_compared=46732, contributions_compared=28666, contexts=9, closed_contexts=9,
        zero_context_cases=1, launches=59, batches=59, q3_rows=7808, q4_rows=5976, extra_shell_rows=7,
        per_k=[dict(K=k, direct_terminals=d, Q=q, H=h) for k, (d, q, h) in enumerate(expected, 1)])
    need(w.gate_summary(json.dumps(gate), 0) == gate, 'synthetic gate wire, CUDA adds one check')
    for field in gate:
        bad = copy.deepcopy(gate)
        del bad[field]
        reject(lambda bad=bad: w.gate_summary(json.dumps(bad), 0), 'missing gate field')
    for field, value in [('backend', 'stub'), ('device_executed', False), ('checks', 372536),
                         ('launches', 58), ('q4_rows', 0), ('closed_contexts', 8), ('fixtures', True)]:
        bad = copy.deepcopy(gate)
        bad[field] = value
        reject(lambda bad=bad: w.gate_summary(json.dumps(bad), 0), 'false/incomplete gate')
    bad = copy.deepcopy(gate)
    bad['per_k'][9]['direct_terminals'] -= 1
    reject(lambda: w.gate_summary(json.dumps(bad), 0), 'K10 missing terminal')
    need(w.gpu_inventory('NVIDIA RTX PRO 6000 Blackwell Server Edition, 580.126.09, 97887, 12.0\n')['memory_mib'] == 97887,
         'G4 inventory format')
    for text in ('RTX PRO 6000, 580.1, 97887, 8.6', 'RTX PRO 6000, 570.1, 97887, 12.0',
                 'RTX PRO 6000, 580.1, 24000, 12.0', 'RTX PRO 6000, 580.1, 97887, 12.0\nsecond'):
        reject(lambda text=text: w.gpu_inventory(text), 'wrong GPU inventory')
    for kind in ('gate', 'probe'):
        argv = w.compile_command('nvcc', Path('/snapshot'), Path('/output'), Path('/tooling'), kind)
        for flag in ('-x', 'cu', '-O3', '-DNDEBUG', '-std=c++20', '-arch=sm_120', '-fmad=false',
                     '--Werror=cross-execution-space-call', '-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror,-pthread'):
            need(flag in argv, 'strict CUDA flags')
        need(argv[-1] == '/tooling/' + kind and not any('FAKE_DEVICE' in x or 'FULL_BALL_CUDA' in x for x in argv),
             'real CUDA binary outside output')
    reject(lambda: w.compile_command('nvcc', Path('/s'), Path('/o'), Path('/t'), 'other'), 'unknown build')
    need(w.WORK_SECONDS == 900, 'economic work window')
    w.need(checks == 29 and rejections == 231, 'historical pure selftest nonvacuity')
    tool_discovery_fixtures()
    w.need(checks == 44 and rejections == 239, 'extended pure selftest nonvacuity')
    print(json.dumps(dict(status='passed', checks=checks, rejections=rejections, synthetic_only=True,
                          tool_admission_combinations=512, subprocess_invoked=False,
                          device_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
