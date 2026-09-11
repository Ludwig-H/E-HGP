#!/usr/bin/env python3
"""Guest-only validation of a CUDA local-MEB batch, under guarded lifecycle.

No FULL tower, terminal resolver, performance contract or VM mutation. The
unchanged pinned session support owns command groups and the closing deadline.
Binaries stay outside output/ so the controller never retrieves executable ELF.
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
DEVICE_GATE = 'morsehgp3D_v7/tests/anchor_meb_route_device_gate.cu'
DEVICE_FIXTURE = 'morsehgp3D_v7/tests/anchor_meb_fixtures.inc'
STRICT_HOST = 'morsehgp3D_v7/bench/nvcc_strict_host.py'
STRICT_HOST_SHA256 = '994d9e6970797594efa2d333275594ef2085cdd166b345b0000135e94e395fb7'
SCOPE = 'local_meb_batch_only'
KEY_BASE = 'morsehgp3D_v7/bench/ball_key_device_private/'
KEY_GATE = KEY_BASE + 'device_gate.cu'
KEY_VECTORS = KEY_BASE + 'vectors.txt'
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
    """Admit only the named device gate/fixture, never a general tests tree."""
    need(type(manifest) is dict and {SUPPORT, LEGACY_PROBE, DEVICE_GATE, DEVICE_FIXTURE, STRICT_HOST} <= set(manifest), 'source inventory')
    if any(name.startswith(KEY_BASE) for name in manifest if type(name) is str):
        need({KEY_GATE, KEY_VECTORS, KEY_BASE + 'wire.cuh', KEY_BASE + 'ball_key_device.cuh'} <= set(manifest),
             'complete optional arithmetic gate inputs')
    result = {}
    for name, pin in manifest.items():
        need(type(name) is str and (name.startswith('morsehgp3D_v7/src/') or
             name.startswith('morsehgp3D_v7/bench/') or name in (DEVICE_GATE, DEVICE_FIXTURE)), 'source scope')
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


def compile_command(nvcc, root, output, tooling, kind='gate'):
    need(kind in ('gate', 'key_gate'), 'known primitive gate')
    entry = DEVICE_GATE if kind == 'gate' else KEY_GATE
    return [nvcc, '-O3', '-DNDEBUG', '-std=c++20', '-arch=sm_120', '-fmad=false',
            '--expt-relaxed-constexpr', '-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror,-pthread',
            '-ccbin', str(output / 'nvcc_strict_host.py'), '-MMD', '-MF',
            str(output / (kind + '.d')), str(root / entry),
            '-o', str(tooling / ('anchor_meb_gate' if kind == 'gate' else 'ball_key_gate'))]


def strict_json(raw):
    def finite(value):
        result = float(value)
        need(math.isfinite(result), 'nonfinite JSON float')
        return result

    def invalid(_value):
        raise ValueError('nonfinite JSON constant')

    return json.loads(raw, object_pairs_hook=unique, parse_float=finite, parse_constant=invalid)


def device_gate_summary(raw, code):
    lines = raw.splitlines()
    need(type(code) is int and code == 0 and len(lines) == 1, 'single successful CUDA gate result')
    row = strict_json(lines[0])
    need(type(row) is dict and row.get('status') == 'passed' and row.get('backend') == 'CUDA'
         and row.get('sm') == '12.0' and row.get('scope') == SCOPE
         and row.get('device_executed') is True and row.get('gcp_used') is False,
         'actual SM120 local-MEB device authority')
    need(type(row.get('device_name')) is str and 0 < len(row['device_name']) <= 512,
         'device name')
    counters = ('checks', 'compared', 'q1', 'q2', 'q3', 'q4', 'extra_shells', 'rejections', 'failures',
                'causal_flags', 'abi_version', 'request_bytes', 'selection_bytes', 'positions',
                'resident_h2d_bytes', 'batch_h2d_bytes', 'batch_d2h_bytes', 'batch_launches',
                'host_validation_powers', 'host_materializations', 'reported_selection_powers')
    need(all(type(row.get(key)) is int and row[key] >= 0 for key in counters), 'integer gate counters')
    need(row['compared'] >= 605 and row['checks'] >= 6 * row['compared']
         and row['extra_shells'] >= 197 and row['rejections'] >= 20 and row['failures'] == 0
         and all(row[key] > 0 for key in ('q1', 'q2', 'q3', 'q4'))
         and sum(row[key] for key in ('q1', 'q2', 'q3', 'q4')) == row['compared'],
         'nonvacuous complete support comparison')
    need((row['abi_version'], row['request_bytes'], row['selection_bytes']) == (1, 72, 112)
         and row['causal_flags'] >= 4 and row['positions'] > 0
         and row['resident_h2d_bytes'] == 24 * row['positions']
         and row['batch_h2d_bytes'] == 184 * row['compared']
         and row['batch_d2h_bytes'] == 112 * row['compared'] and row['batch_launches'] == 1
         and row['host_materializations'] == row['compared']
         and row['host_validation_powers'] > 0 and row['reported_selection_powers'] > 0,
         'MEB wire ABI, causal flags and separately accounted nominal batch')
    # gcp_used above describes the executable (no cloud API); this worker is a
    # GCP guest, explicitly identified in its own receipt. Never a FULL claim.
    return row


def key_gate_summary(raw, code, vectors_sha256):
    need(type(code) is int and code == 0 and len(raw.splitlines()) == 1, 'single successful arithmetic device gate')
    row = strict_json(raw)
    need(type(row) is dict and row.get('status') == 'passed' and row.get('backend') == 'CUDA'
         and row.get('device_executed') is True and row.get('public_status') == 'not_claimed'
         and row.get('vectors_sha256') == vectors_sha256
         and re.fullmatch('[0-9a-f]{64}', vectors_sha256), 'arithmetic device identity')
    keys = ('cases', 'checked_words', 'raw_cases', 'gcd_cases', 'division_cases', 'support_cases',
            'rejected_cases', 'input_bytes', 'output_bytes', 'alignment', 'device_arch')
    need(all(type(row.get(key)) is int and row[key] >= 0 for key in keys), 'arithmetic counters')
    need(row['raw_cases'] >= 4000 and row['gcd_cases'] >= 2000 and row['division_cases'] >= 4000
         and row['support_cases'] >= 3000 and row['rejected_cases'] >= 20
         and row['cases'] == sum(row[key] for key in ('raw_cases', 'gcd_cases', 'division_cases', 'support_cases'))
         and row['checked_words'] == row['cases'] * 24, 'whole arithmetic corpus compared twice')
    need((row['input_bytes'], row['output_bytes'], row['alignment'], row['device_arch']) == (112, 96, 8, 1200),
         'arithmetic wire ABI and actual SM120')
    return row


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
    tooling = output.parent / 'anchor_meb_tooling'
    need(not output.exists() and not output.is_relative_to(root), 'fresh output outside snapshot')
    need(not tooling.exists() and not tooling.is_relative_to(root), 'fresh private tooling outside snapshot')
    output.mkdir(parents=True)
    tooling.mkdir(mode=0o700)
    support, worker, before, binaries = None, None, None, {}
    result = dict(status='failed', public_status='not_claimed', contract_qualified=False,
                  backend_scope=SCOPE, GCP_used=True, FULL_GPU_available=False, worker_argv=list(sys.argv),
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
        kinds = ('gate', 'key_gate') if KEY_GATE in manifest else ('gate',)
        result['selected_gates'] = list(kinds)
        for kind in kinds:
            command = compile_command(nvcc, root, output, tooling, kind)
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
            need((DEVICE_GATE if kind == 'gate' else KEY_GATE) in consumed, 'depfile lacks entry source')
            support.save(output / (kind + '_compiled_dependencies.json'), dict(project=consumed, external=external))
            binaries[kind] = dict(path=str(binary), sha256=sha(binary), depfile_sha256=sha(depfile))
            if kind == 'gate':
                row = worker.command('device_gate', [str(binary), '--selftest'])
                result['device_gate'] = device_gate_summary((output / 'device_gate.stdout').read_text(), row['exit_code'])
                support.save(output / 'device_gate.summary.json', result['device_gate'])
            else:
                arguments = [str(binary), '--selftest', str(root / KEY_VECTORS),
                             '--expected-sha=' + before[KEY_VECTORS]]
                row = worker.command('key_gate', arguments)
                result['key_gate'] = key_gate_summary((output / 'key_gate.stdout').read_text(),
                                                      row['exit_code'], before[KEY_VECTORS])
                support.save(output / 'key_gate.summary.json', result['key_gate'])
                for injection in ('skip-write', 'corrupt-word'):
                    name = 'key_gate_' + injection.replace('-', '_')
                    mutant = worker.command(name, [*arguments, '--inject=' + injection])
                    need(mutant['exit_code'] == 1 and not (output / (name + '.stdout')).read_bytes()
                         and (output / (name + '.stderr')).read_text().strip() == 'FAIL backend.expected_word',
                         'arithmetic wire causal mutant must fail its comparison')
        result['binaries'] = binaries

        bad_argument = worker.command('device_gate_bad_argument', [binaries['gate']['path'], '--unknown'])
        need(bad_argument['exit_code'] == 2 and not (output / 'device_gate_bad_argument.stdout').read_bytes(),
             'bad argument must refuse with code 2 and no success output')
        result['status'] = 'completed'
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
