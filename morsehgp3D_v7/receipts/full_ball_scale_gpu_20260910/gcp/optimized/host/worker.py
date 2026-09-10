#!/usr/bin/env python3
"""Guest-only CPU/CUDA-census FULL pairs under the unchanged guarded controller.

No VM lifecycle mutation, CUDA installation or reboot. The copied support worker
is pinned and imported by exact path only after its manifest has been verified.
This worker reports relative producer observations, never a contract certificate.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import resource
import shlex
import shutil
import signal
import sys
import time

SUPPORT = 'morsehgp3D_v7/bench/session_support/full_probe_worker_v7.py'
SUPPORT_SHA256 = 'da967163bdb7247bc6aad4df0c294cda1071076a0127cd5bd9f59bc0e4788439'
LEGACY_PROBE = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp'
PROBE = 'morsehgp3D_v7/bench/full_ball_tower_probe.cpp'
DEVICE_GATE = 'morsehgp3D_v7/tests/census_route_device_gate.cu'
STRICT_HOST = 'morsehgp3D_v7/bench/nvcc_strict_host.py'
STRICT_HOST_SHA256 = '994d9e6970797594efa2d333275594ef2085cdd166b345b0000135e94e395fb7'
SCHEMA = 'mhgp7-full-ball-tower-probe-v1'
BACKENDS = {'cpu': 'cpu_reference', 'gpu': 'cuda_census_cpu_full'}
COUNTERS = ('raw', 'unique', 'balls', 'nodes', 'parent_refs', 'contributions', 'vertical_refs',
            'extra_records', 'anchor_blocks', 'representatives', 'anchor_hits', 'intruder_queries',
            'same_radius_steps', 'validation_meb_calls', 'resolver_meb_calls', 'resolver_cache_queries',
            'resolver_cache_hits', 'resolver_cache_stores', 'resolver_cache_evictions', 'resolver_cache_seed_stores',
            'resolver_cache_slots', 'resolver_cache_reset_slots', 'resolver_cache_bytes', 'resolver_cache_released_slots',
            'resolver_supports_tested')
TIMINGS = ('index_s', 'generate_s', 'sort_s', 'prefilter_s', 'census_s', 'tower_s', 'digest_s', 'total_s')
CUDA_PATHS = ('/usr/local/cuda/bin/nvcc', '/usr/local/cuda-12.9/bin/nvcc', '/usr/local/cuda-13.0/bin/nvcc')


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate JSON field: ' + key)
        result[key] = value
    return result


def source_map(root, manifest):
    """No tests tree admission: only the one named CUDA gate is permitted."""
    need(type(manifest) is dict and {SUPPORT, LEGACY_PROBE, PROBE, DEVICE_GATE, STRICT_HOST} <= set(manifest), 'source inventory')
    result = {}
    for name, pin in manifest.items():
        need(type(name) is str and (name.startswith('morsehgp3D_v7/src/') or
             name.startswith('morsehgp3D_v7/bench/') or name == DEVICE_GATE), 'source scope')
        relative = Path(name)
        need(not relative.is_absolute() and '..' not in relative.parts and str(relative) == name,
             'source relative path')
        path = root / relative
        need(not path.is_symlink() and path.is_file() and path.resolve().is_relative_to(root) and
             type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin), 'source path/hash')
        result[name] = sha(path)
        need(result[name] == pin, 'source changed: ' + name)
    need(result[SUPPORT] == SUPPORT_SHA256, 'reviewed support pin')
    need(result[STRICT_HOST] == STRICT_HOST_SHA256, 'reviewed strict host adapter pin')
    return result


def load_support(root, manifest):
    # Executable imported bytes have a FIXED reviewed digest, not merely a
    # caller-supplied path/hash. No sys.path insertion or adjacent module lookup.
    need(manifest.get(SUPPORT) == SUPPORT_SHA256 and sha(root / SUPPORT) == SUPPORT_SHA256, 'support import pin')
    spec = importlib.util.spec_from_file_location('mhgp7_reviewed_full_session_support', root / SUPPORT)
    need(spec is not None and spec.loader is not None, 'support module unavailable')
    module = importlib.util.module_from_spec(spec)
    sys.dont_write_bytecode = True
    spec.loader.exec_module(module)
    return module


def compile_commands(compiler, nvcc, root, output):
    cpu_flags = ['-O3', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread']
    gpu_flags = ['-O3', '-DNDEBUG', '-std=c++20', '-arch=sm_120', '-fmad=false', '--expt-relaxed-constexpr',
                 '-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror,-pthread', '-ccbin', str(output / 'nvcc_strict_host.py')]
    return {
        'cpu': [compiler, *cpu_flags, '-MMD', '-MF', str(output / 'full_probe.d'),
                str(root / PROBE), '-o', str(output / 'full_probe')],
        'gate': [nvcc, *gpu_flags, '-MMD', '-MF', str(output / 'device_gate.d'),
                 str(root / DEVICE_GATE), '-o', str(output / 'device_gate')],
        'gpu': [nvcc, *gpu_flags, '-DMHGP7_FULL_BALL_CUDA=1', '-x', 'cu', '-MMD', '-MF',
                str(output / 'full_probe_gpu.d'), str(root / PROBE), '-o', str(output / 'full_probe_gpu')],
    }


def probe_summary(raw, code, n, kmax, s, backend, strict_json):
    try:
        rows = [strict_json(line) for line in raw.splitlines()]
        need(len(rows) == 1 and type(rows[0]) is dict, 'single complete JSON result required')
        row = rows[0]
        need(code == 0 and row['schema'] == SCHEMA and row['status'] == 'completed_relative' and
             row['public_status'] == 'not_claimed' and row['contract_qualified'] is False and
             row['backend'] == BACKENDS[backend], 'relative CPU/GPU outcome')
        expected = dict(n=n, s=s, kmax=min(n, kmax), threads=48, seed=3, coord=65536, orders=min(n, kmax))
        need(all(type(row[key]) is int and row[key] == value for key, value in expected.items()), 'requested tower')
        need(all(type(row[key]) is int and row[key] >= 0 for key in COUNTERS), 'integer counters')
        need(row['raw'] >= row['unique'] >= row['balls'] > 0 and row['nodes'] >= n and
             row['contributions'] >= n, 'nonvacuous retained tower')
        need(all(type(row[key]) in (float, int) and math.isfinite(row[key]) and row[key] >= 0
                 for key in TIMINGS), 'finite stage times')
        need(sum(row[key] for key in TIMINGS[:-1]) <= row['total_s'] + 0.000001, 'total time includes stages')
        need(all(type(row[key]) is str and re.fullmatch('[0-9a-f]{64}', row[key])
                 for key in ('input_digest', 'payload_digest')), 'digest fields')
        signature = {key: row[key] for key in (*expected, *COUNTERS, 'input_digest', 'payload_digest')}
        return dict(reported_complete=True, row=row, signature=signature)
    except (ValueError, KeyError, TypeError) as error:
        return dict(reported_complete=False, reason=type(error).__name__ + ': ' + str(error))


def pair_summary(cpu, gpu):
    if not cpu['reported_complete'] or not gpu['reported_complete']:
        return dict(status='incomplete', matched=False, cpu_complete=cpu['reported_complete'],
                    gpu_complete=gpu['reported_complete'], contract_qualified=False)
    equal = cpu['signature'] == gpu['signature']
    return dict(status='matched' if equal else 'diverged', matched=equal,
                cpu_signature=cpu['signature'], gpu_signature=gpu['signature'], contract_qualified=False)


def device_gate_summary(raw, code):
    lines = raw.splitlines()
    need(code == 0 and len(lines) == 2 and
         re.fullmatch(r'backend=CUDA device=.+ sm=12\.0', lines[0]), 'actual SM120 CUDA gate required')
    match = re.fullmatch(r'census_route_gate=passed backend=CUDA checks=([0-9]+) compared=([0-9]+) '
        r'extra_shells=([0-9]+) q2=([0-9]+) q3=([0-9]+) q4=([0-9]+) rejections=17 failures=0', lines[1])
    need(match is not None, 'complete successful device gate')
    values = list(map(int, match.groups()))
    need(values[0] > 0 and values[1] > 100 and values[2] >= 3 and all(v > 0 for v in values[3:]),
         'device gate nonvacuity')
    return dict(status='passed', backend='CUDA', sm='12.0', checks=values[0], compared=values[1],
                extra_shells=values[2], q2=values[3], q3=values[4], q4=values[5], rejections=17)


def additional_s_plan(remaining, baseline_seconds, s):
    need(type(remaining) in (int, float) and math.isfinite(remaining) and remaining > 0 and
         type(baseline_seconds) in (int, float) and math.isfinite(baseline_seconds) and baseline_seconds >= 0
         and s in (10, 12), 'additional s planning inputs')
    # A conservative launch heuristic, NOT a complexity bound or command quota.
    required = max(120.0, 3.0 * baseline_seconds + 30.0)
    return dict(s=s, remaining_work_seconds=remaining, required_estimate_seconds=required,
                estimator='max_120s_3x_observed_s8_four_processes_plus_30s_v1',
                status='planned' if remaining > required else 'not_attempted',
                reason='observed_cost_reserve' if remaining > required else 'closing_window_reserve')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source-root', 'source-manifest', 'guard-mark', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ('source-manifest-sha256', 'guard-mark-sha256', 'project', 'zone', 'instance', 'generation'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--session-deadline-epoch', type=int, required=True)
    parser.add_argument('--closing-margin-seconds', type=int, default=300)
    parser.add_argument('--bootstrap', action='store_true')
    args = parser.parse_args()
    root, output = args.source_root.resolve(), args.output.resolve()
    need(not output.exists() and not output.is_relative_to(root), 'fresh output outside snapshot')
    output.mkdir(parents=True)
    support, worker, before, binaries = None, None, None, {}
    result = dict(status='failed', public_status='not_claimed', contract_qualified=False,
                  backend_scope='CPU_WSPD_sort_FULL_and_CUDA_census_only', worker_argv=list(sys.argv),
                  worker_sha256=sha(__file__), targeted_GCP_stop_required_by_ROOT=True,
                  VM_shutdown_certified_by_worker=False)

    def interrupted(signum, _frame):
        raise InterruptedError('received signal ' + str(signum))

    handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        need(sha(args.source_manifest) == args.source_manifest_sha256, 'external source manifest pin')
        manifest = json.loads(args.source_manifest.read_text(), object_pairs_hook=unique)
        before = source_map(root, manifest)
        support = load_support(root, manifest)
        support.save(output / 'sources_before.json', before)
        need(sha(args.guard_mark) == args.guard_mark_sha256, 'external guard mark pin')
        mark = support.fields(args.guard_mark.read_text())
        schedule = support.fields(support.scheduled_text())
        target = {key: getattr(args, key) for key in ('project', 'zone', 'instance')}
        guards = support.guard_values(mark, schedule, target, args.generation, args.session_deadline_epoch,
                                      args.closing_margin_seconds, time.time())
        observed = support.metadata()
        need(all(observed[key] == value for key, value in target.items()) and observed['machine'] == 'g4-standard-48',
             'guest metadata target/type')
        boot_epoch = time.time() - float(Path('/proc/uptime').read_text().split()[0])
        need(abs(boot_epoch - support.epoch(args.generation)) <= 300, 'guest boot/generation')
        cpus = sorted(os.sched_getaffinity(0))
        need(len(cpus) == 48, '48 available vCPUs required')
        result.update(target=target, generation=args.generation, guards=guards, guest_metadata=observed,
                      available_cpus=cpus, source_manifest_sha256=args.source_manifest_sha256,
                      support_sha256=SUPPORT_SHA256, resource_limits={
                          'RLIMIT_AS': resource.getrlimit(resource.RLIMIT_AS), 'RLIMIT_CPU': resource.getrlimit(resource.RLIMIT_CPU)})
        support.save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule, metadata=observed, bounds=guards))
        worker = support.Worker(output, guards['work_deadline_epoch'], schedule)
        nvcc = shutil.which('nvcc') or next((p for p in CUDA_PATHS if Path(p).is_file() and os.access(p, os.X_OK)), None)
        smi = shutil.which('nvidia-smi')
        need(nvcc and smi, 'existing NVCC/NVIDIA tools required; no CUDA installation')
        result['CUDA_installation_attempted'] = False
        missing = [name for name, present in (('g++', shutil.which('g++') is not None),
                                               ('time', Path('/usr/bin/time').is_file())) if not present]
        if missing:
            need(args.bootstrap, 'CPU tools missing; bootstrap not authorized')
            for name, argv in support.bootstrap_commands(missing, os.geteuid() == 0):
                need(worker.command(name, argv)['exit_code'] == 0, name)
        compiler = shutil.which('g++')
        need(compiler and nvcc and smi and Path('/usr/bin/time').is_file(), 'existing compiler/NVCC/NVIDIA tools required')
        need(Path(compiler).resolve() == Path('/usr/bin/g++').resolve(), 'strict NVCC adapter compiler binding')
        strict_host = output / 'nvcc_strict_host.py'
        shutil.copyfile(root / STRICT_HOST, strict_host)
        strict_host.chmod(0o700)
        need(sha(strict_host) == before[STRICT_HOST], 'strict NVCC adapter copy')
        result['strict_host_adapter'] = dict(source=STRICT_HOST, sha256=before[STRICT_HOST],
                                            compiler='/usr/bin/g++', generated_preprocess_only=True)
        for name, argv in [('compiler_version', [compiler, '--version']), ('nvcc_version', [nvcc, '--version']),
                           ('time_version', ['/usr/bin/time', '--version']),
                           ('gpu_inventory', [smi, '--query-gpu=name,driver_version,memory.total,compute_cap', '--format=csv,noheader']),
                           ('cpu_inventory', ['lscpu'])]:
            need(worker.command(name, argv)['exit_code'] == 0, name)
        for filename in ('/etc/os-release', '/proc/meminfo', '/proc/self/cgroup'):
            (output / (Path(filename).name + '.txt')).write_text(Path(filename).read_text())
        plans = compile_commands(compiler, nvcc, root, output)
        for kind in ('gate', 'cpu', 'gpu'):
            command = plans[kind]
            need(sha(strict_host) == before[STRICT_HOST], 'strict NVCC adapter before compile')
            need(worker.command('compile_' + kind, command)['exit_code'] == 0, 'strict compile: ' + kind)
            need(sha(strict_host) == before[STRICT_HOST], 'strict NVCC adapter after compile')
            binary = Path(command[-1])
            depfile = Path(command[command.index('-MF') + 1])
            consumed = {}
            external = {}
            for spelling in shlex.split(depfile.read_text().replace('\\\n', ' ').split(':', 1)[1]):
                path = Path(spelling).resolve()
                if path.is_relative_to(root):
                    relative = path.relative_to(root).as_posix()
                    need(relative in before and sha(path) == before[relative], 'unexpected/changed source dependency')
                    consumed[relative] = before[relative]
                else:
                    # NVCC may list its own runtime headers despite -MMD.
                    # Capture, never silently treat them as project sources.
                    need(path.is_file(), 'external compilation dependency missing')
                    external[str(path)] = sha(path)
            need((DEVICE_GATE if kind == 'gate' else PROBE) in consumed, 'depfile lacks entry source')
            support.save(output / (kind + '_compiled_dependencies.json'), dict(project=consumed, external=external))
            binaries[kind] = dict(path=str(binary), sha256=sha(binary), depfile_sha256=sha(depfile))
            if kind == 'gate':
                row = worker.command('device_gate', [str(binary), '--selftest'])
                result['device_gate'] = device_gate_summary((output / 'device_gate.stdout').read_text(), row['exit_code'])
                support.save(output / 'device_gate.summary.json', result['device_gate'])
        result['binaries'] = binaries

        def pair(n, kmax, s):
            summaries = {}
            elapsed = 0.0
            for kind in ('cpu', 'gpu'):
                name = f'{kind}_n{n}_k{kmax}_s{s}'
                binary = binaries[kind]
                row = worker.command(name, ['/usr/bin/time', '-v', binary['path'], f'--n={n}', f'--s={s}',
                                            f'--kmax={kmax}', '--threads=48'])
                summary = probe_summary((output / (name + '.stdout')).read_text(), row['exit_code'],
                                        n, kmax, s, kind, support.strict_json)
                summary.update(command_exit_code=row['exit_code'], process_wall_seconds=row['elapsed_seconds'])
                if summary['reported_complete']:
                    need(summary['row']['total_s'] <= row['elapsed_seconds'] + 0.05, 'process time shorter than probe total')
                need(sha(binary['path']) == binary['sha256'], 'binary changed after execution')
                support.save(output / (name + '.summary.json'), summary)
                summaries[kind] = summary
                elapsed += row['elapsed_seconds']
            comparison = pair_summary(summaries['cpu'], summaries['gpu'])
            comparison.update(n=n, kmax_requested=kmax, s=s, summed_process_wall_seconds=elapsed)
            support.save(output / f'pair_n{n}_k{kmax}_s{s}.json', comparison)
            need(comparison['status'] != 'diverged', 'CPU/CUDA geometry or FULL payload divergence')
            return comparison

        result['smoke'] = pair(8, 10, 8)
        need(result['smoke']['matched'], 'smoke CPU/GPU failed; no heavy run')
        result['s8'] = [pair(50000, kmax, 8) for kmax in (10, 5)]
        complete = all(row['matched'] for row in result['s8'])
        result['additional_s'] = []
        if complete:
            baseline_seconds = sum(row['summed_process_wall_seconds'] for row in result['s8'])
            for s in (10, 12):
                plan = additional_s_plan(worker.remaining(), baseline_seconds, s)
                support.save(output / f's{s}.plan.json', plan)
                observation = dict(plan=plan, pairs=[])
                if plan['status'] == 'planned':
                    observation['pairs'] = [pair(50000, kmax, s) for kmax in (10, 5)]
                    complete = complete and all(row['matched'] for row in observation['pairs'])
                result['additional_s'].append(observation)
                if not complete:
                    break
        result['status'] = 'completed' if complete else 'failed'
    except BaseException as error:
        result.update(status='session_deadline' if support and isinstance(error, support.SessionDeadline) else 'failed',
                      error=type(error).__name__ + ': ' + str(error))
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        result['commands'] = worker.commands if worker else []
        result['binaries'] = binaries
        if before is not None:
            try:
                after = source_map(root, manifest)
                need(before == after, 'source snapshot changed')
                result['sources_stable'] = True
                if support:
                    support.save(output / 'sources_after.json', after)
            except Exception as error:
                result.update(status='failed', sources_stable=False, source_error=str(error))
        for binary in binaries.values():
            try:
                need(sha(binary['path']) == binary['sha256'], 'binary changed')
                binary['sha256_after'] = sha(binary['path'])
            except Exception as error:
                result.update(status='failed', binary_error=str(error))
        try:
            with (output / 'receipt.json').open('x') as stream:
                json.dump(result, stream, indent=2, sort_keys=True)
                stream.write('\n')
        finally:
            for sig, handler in handlers.items():
                signal.signal(sig, handler)
        print(json.dumps(dict(status=result['status'], output=str(output),
                              targeted_GCP_stop_required_by_ROOT=True), sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
