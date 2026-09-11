#!/usr/bin/env python3
"""Guest-only FULL CPU worker. Never starts/stops GCP or changes its guards.

ROOT must start with start_and_verify.sh, upload only after double_guard_verified,
retrieve this fresh output directory, then stop_and_verify.sh the same generation.
The only command deadline is the mandatory session deadline minus closing margin.
This is not the historical reduced-F worker, a completeness judge, or a GPU path.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import resource
import shlex
import shutil
import signal
import subprocess
import sys
import time
import urllib.request

PROBE = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp'
SCHEMA = 'mhgp7-full-gabriel-probe-v6'
CENSUS_ACCOUNTING = 'preflight_survivor_then_direct_census_v2'
SCHEDULE = Path('/run/systemd/shutdown/scheduled')


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def strict_json(raw):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            need(key not in result, 'duplicate JSON key: ' + key)
            result[key] = value
        return result
    def finite_float(text):
        value = float(text)
        need(math.isfinite(value), 'nonfinite JSON number')
        return value
    def reject_constant(_text):
        raise ValueError('nonfinite JSON constant')
    return json.loads(raw, object_pairs_hook=unique, parse_float=finite_float, parse_constant=reject_constant)


def aggregate_digest(input_hash, order_hashes):
    # Same wire formula as the portable FULL reader; no receipt/binary pins imported.
    value = hashlib.sha256()
    def number(item):
        value.update(item.to_bytes(8, 'little'))
    def tag(item):
        raw = item.encode('ascii')
        number(len(raw))
        value.update(raw)
    tag('mhgp7-full-semantic-v1:horizontal-orders')
    tag(input_hash)
    number(len(order_hashes))
    for index, item in enumerate(order_hashes, 1):
        number(index)
        tag(item)
    return value.hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def epoch(value):
    parsed = datetime.fromisoformat(value.replace('Z', '+00:00'))
    need(parsed.tzinfo is not None, 'UTC timestamp must be timezone-aware')
    return parsed.timestamp()


def fields(text):
    out = {}
    for line in text.splitlines():
        key, separator, value = line.partition('=')
        need(separator == '=' and key not in out, 'guard fields')
        out[key] = value
    return out


def guard_values(mark, schedule, target, generation, deadline, margin, now):
    """Pure checks of copied start evidence and the guest's current shutdown."""
    need(mark['schema'] == 'e-hgp.guard-mark.v1' and mark['mark'] == 'double_guard_verified', 'double guard mark')
    need(all(mark[key] == value for key, value in target.items()) and mark['generation'] == generation,
         'guard target/generation')
    duration = int(mark['max_run_seconds'])
    guest_minutes = int(mark['guest_shutdown_minutes'])
    need(30 <= duration <= 28800 and 1 <= guest_minutes <= 480 and guest_minutes * 60 + 900 <= duration,
         'guard duration')
    started, verified = epoch(generation), epoch(mark['date_utc'])
    need(started <= verified <= now + 5, 'guard chronology')
    need(schedule['MODE'] == 'poweroff' and re.fullmatch('[0-9]{1,18}', schedule['USEC']), 'guest poweroff')
    guest_deadline = int(schedule['USEC']) / 1000000
    safe_gce_deadline = started + duration - 300
    need(now < guest_deadline <= safe_gce_deadline and margin >= 300 and
         now + margin < deadline <= guest_deadline, 'session deadline and closing margin')
    return dict(guest_deadline_epoch=guest_deadline, safe_gce_deadline_epoch=safe_gce_deadline,
                work_deadline_epoch=deadline-margin, closing_margin_seconds=margin)


def metadata():
    result = {}
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    for key, path in {'project': 'project/project-id', 'zone': 'instance/zone',
                      'instance': 'instance/name', 'machine': 'instance/machine-type'}.items():
        request = urllib.request.Request('http://169.254.169.254/computeMetadata/v1/' + path,
                                         headers={'Metadata-Flavor': 'Google'})
        with opener.open(request, timeout=10) as response:
            result[key] = response.read(4096).decode().strip().rsplit('/', 1)[-1]
    return result


def scheduled_text():
    try:
        return SCHEDULE.read_text()
    except PermissionError:
        # Read-only control call; sudo -n cannot prompt. Never rearm/cancel shutdown.
        return subprocess.check_output(['sudo', '-n', 'cat', str(SCHEDULE)], text=True)


def source_map(root, manifest):
    need(type(manifest) is dict and PROBE in manifest and len(manifest) > 1, 'v7 manifest')
    result = {}
    for name, expected in manifest.items():
        path = root / name
        need(type(name) is str and (name.startswith('morsehgp3D_v7/src/') or name.startswith('morsehgp3D_v7/bench/')) and
             '..' not in Path(name).parts and not path.is_symlink() and path.resolve().is_relative_to(root) and
             type(expected) is str and re.fullmatch('[0-9a-f]{64}', expected), 'v7 source path/pin')
        result[name] = sha(path)
        need(result[name] == expected, 'source hash:' + name)
    return result


def probe_summary(raw, code, n, kmax):
    """Reported FULL status only; refusals/partial output cannot become success."""
    try:
        rows = [strict_json(line) for line in raw.splitlines()]
        count = min(n, kmax)
        need([row['type'] for row in rows] == ['configuration'] + ['order'] * count + ['terminal'], 'rows')
        config, orders, terminal = rows[0], rows[1:-1], rows[-1]
        need(all(row['schema'] == SCHEMA and row['pipeline_threads'] == 48 and row['full_order_builder_threads'] == 1 and
                 row['order_schedule'] == 'sequential_k1_to_kmax' for row in rows), 'FULL parallel schema')
        need(all(row['census_payload_accounting'] == CENSUS_ACCOUNTING for row in rows), 'census payload accounting')
        need(config['n'] == terminal['n'] == n and config['s'] == terminal['s'] == 8 and
             config['kmax_requested'] == terminal['kmax_requested'] == kmax and
             config['kmax_effective'] == terminal['kmax_effective'] == count, 'requested domain')
        need(code == 0 and terminal['exit_code'] == 0 and terminal['terminal_status'] == 'completed' and
             terminal['complete_requested_horizontal_orders'] is True and terminal['completed_orders_diagnostic'] == count and
             terminal['outcome'] == 'complete_relative' and terminal['public_status'] == 'not_claimed' and
             [row['k'] for row in orders] == list(range(1, count+1)) and
             all(row['outcome'] == 'complete_relative' for row in orders), 'complete reported orders')
        need(all(row['meb_proposal_budget_kind'] == 'unlimited' and row['max_meb_proposal_supports_per_order'] == (1 << 64)-1
                 and row['alias_policy'] == 'lazy_first_c_strict_resolutions_v1' and row['cache_entries'] == 1000000 for row in rows),
             'proposal and alias policy')
        hashes = [terminal['input_digest'], terminal['certificate_digest'], *[row['certificate_digest'] for row in orders]]
        need(all(type(value) is str and re.fullmatch('[0-9a-f]{64}', value) for value in hashes), 'digest format')
        need(terminal['certificate_digest'] == aggregate_digest(terminal['input_digest'], hashes[2:]), 'final digest binding')
        return dict(reported_complete=True, orders=count, terminal=terminal,
                    order_digests=[row['certificate_digest'] for row in orders])
    except (ValueError, KeyError, TypeError) as error:
        return dict(reported_complete=False, read_reason=type(error).__name__ + ': ' + str(error))


class SessionDeadline(Exception):
    pass


def bootstrap_commands(missing, as_root):
    # Install only missing CPU tools. No upgrade, CUDA, kernel install or reboot.
    # needrestart is list-only even if the guest's default mode is automatic.
    prefix = [] if as_root else ['sudo', '-n']
    environment = ['env', 'DEBIAN_FRONTEND=noninteractive', 'NEEDRESTART_MODE=l']
    return [('bootstrap_update', [*prefix, *environment, 'apt-get', 'update']),
            ('bootstrap_install', [*prefix, *environment, 'apt-get', 'install', '-y', '--no-install-recommends', *missing])]


class Worker:
    def __init__(self, output, deadline, schedule):
        self.output, self.deadline, self.schedule = output, deadline, schedule
        self.monotonic_deadline = time.monotonic() + deadline-time.time()
        self.commands = []

    def remaining(self):
        remaining = min(self.deadline-time.time(), self.monotonic_deadline-time.monotonic())
        if remaining <= 0:
            raise SessionDeadline('session work window ended; preserve closing margin')
        return remaining

    def command(self, name, argv):
        self.remaining()
        need(fields(scheduled_text()) == self.schedule, 'guest shutdown changed/disappeared')
        row = dict(argv=argv, cwd=str(self.output), started_utc=datetime.now(timezone.utc).isoformat(),
                   session_work_deadline_epoch=self.deadline, per_command_watchdog=None,
                   pid=None, exit_code=None, group_closed=False)
        save(self.output / (name + '.intent.json'), row)
        process = None
        start = time.monotonic()
        try:
            with (self.output / (name + '.stdout')).open('xb') as out, (self.output / (name + '.stderr')).open('xb') as err:
                process = subprocess.Popen(argv, cwd=self.output, stdin=subprocess.DEVNULL,
                                           stdout=out, stderr=err, start_new_session=True)
                row['pid'] = process.pid
                try:
                    row['exit_code'] = process.wait(timeout=self.remaining())
                except subprocess.TimeoutExpired as error:
                    row['session_deadline_reached'] = True
                    raise SessionDeadline('session deadline, not a scientific operation/time quota') from error
        finally:
            # Join/kill our exact process group before log writes, even on signal/disk error.
            if process is not None:
                try:
                    os.killpg(process.pid, 0)
                    row['residual_or_interrupted_group_killed'] = True
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                row['exit_code'] = process.wait()
                # Short cleanup allowance only, never a new computation budget.
                drain_end = time.monotonic() + 10
                while True:
                    try:
                        os.killpg(process.pid, 0)
                    except ProcessLookupError:
                        row['group_closed'] = True
                        break
                    if time.monotonic() >= drain_end:
                        break
                    time.sleep(0.05)
            row.update(ended_utc=datetime.now(timezone.utc).isoformat(), elapsed_seconds=time.monotonic()-start,
                       stdout_sha256=sha(self.output / (name + '.stdout')),
                       stderr_sha256=sha(self.output / (name + '.stderr')))
            self.commands.append(row)
            save(self.output / (name + '.command.json'), row)
        need(row['group_closed'] and not row.get('residual_or_interrupted_group_killed'), 'command group not normally closed')
        return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True, help='snapshot root containing morsehgp3D_v7/')
    parser.add_argument('--source-manifest', type=Path, required=True, help='JSON relative-path to SHA256 mapping')
    parser.add_argument('--source-manifest-sha256', required=True)
    parser.add_argument('--guard-mark', type=Path, required=True)
    parser.add_argument('--guard-mark-sha256', required=True)
    for name in ('project', 'zone', 'instance', 'generation'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--session-deadline-epoch', type=int, required=True, help='absolute epoch no later than guest poweroff')
    parser.add_argument('--closing-margin-seconds', type=int, default=300)
    parser.add_argument('--output', type=Path, required=True, help='new directory, never reused')
    parser.add_argument('--bootstrap', action='store_true', help='explicitly allow apt install g++/time if missing, after both guards')
    args = parser.parse_args()
    output, root = args.output.resolve(), args.source_root.resolve()
    need(not output.exists() and not output.is_relative_to(root), 'fresh output outside immutable snapshot')
    output.mkdir(parents=True)
    result = dict(status='failed', public_status='not_claimed', GPU_FULL_available=False, contract_certified=False,
                  worker_argv=list(sys.argv), guard_mark_sha256=args.guard_mark_sha256,
                  targeted_GCP_stop_required_by_ROOT=True, VM_shutdown_certified_by_worker=False)
    worker, before = None, None
    def interrupted(signum, _frame):
        raise InterruptedError('received signal ' + str(signum))
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    try:
        need(sha(args.guard_mark) == args.guard_mark_sha256, 'external guard mark pin')
        mark = fields(args.guard_mark.read_text())
        schedule = fields(scheduled_text())
        target = {key: getattr(args, key) for key in ('project', 'zone', 'instance')}
        guards = guard_values(mark, schedule, target, args.generation, args.session_deadline_epoch,
                              args.closing_margin_seconds, time.time())
        observed = metadata()
        need(all(observed[key] == value for key, value in target.items()) and observed['machine'] == 'g4-standard-48',
             'guest metadata target/type')
        boot_epoch = time.time() - float(Path('/proc/uptime').read_text().split()[0])
        need(abs(boot_epoch-epoch(args.generation)) <= 300, 'guest boot inconsistent with generation')
        cpus = sorted(os.sched_getaffinity(0))
        need(len(cpus) == 48, '48 available vCPUs required, no hidden affinity restriction')
        result.update(target=target, generation=args.generation, guards=guards, guest_metadata=observed,
                      available_cpus=cpus, source_manifest_sha256=args.source_manifest_sha256,
                      worker_sha256=sha(Path(__file__)), resource_limits={
                          'RLIMIT_AS': resource.getrlimit(resource.RLIMIT_AS), 'RLIMIT_CPU': resource.getrlimit(resource.RLIMIT_CPU)})
        save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule, metadata=observed, bounds=guards))
        need(sha(args.source_manifest) == args.source_manifest_sha256, 'external source manifest pin')
        manifest = strict_json(args.source_manifest.read_text())
        before = source_map(root, manifest)
        save(output / 'sources_before.json', before)
        for filename in ('/etc/os-release', '/proc/meminfo', '/proc/self/cgroup'):
            with (output / (Path(filename).name + '.txt')).open('x') as stream:
                stream.write(Path(filename).read_text())
        worker = Worker(output, guards['work_deadline_epoch'], schedule)
        missing = [package for package, exists in (('g++', shutil.which('g++') is not None),
                                                    ('time', Path('/usr/bin/time').is_file())) if not exists]
        if missing:
            need(args.bootstrap, 'missing tools: ' + ','.join(missing) + '; bootstrap not authorized')
            for name, argv in bootstrap_commands(missing, os.geteuid() == 0):
                row = worker.command(name, argv)
                need(row['exit_code'] == 0, name)
        compiler = shutil.which('g++')
        need(compiler and Path('/usr/bin/time').is_file(), 'compiler/GNU time missing')
        for name, argv in [('compiler_version', [compiler, '--version']), ('time_version', ['/usr/bin/time', '--version'])]:
            need(worker.command(name, argv)['exit_code'] == 0, name)
        binary, depfile = output / 'full_probe', output / 'full_probe.d'
        row = worker.command('compile', [compiler, '-O3', '-DNDEBUG', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic',
            '-Werror', '-pthread', '-MMD', '-MF', str(depfile), str(root / PROBE), '-o', str(binary)])
        need(row['exit_code'] == 0, 'strict O3 compilation failed')
        consumed = {}
        for name in shlex.split(depfile.read_text().replace('\\\n', ' ').split(':', 1)[1]):
            path = Path(name).resolve()
            relative = str(path.relative_to(root))
            need(relative in before and sha(path) == before[relative], 'unexpected or changed compilation dependency')
            consumed[relative] = before[relative]
        need(PROBE in consumed, 'depfile lacks probe')
        save(output / 'compiled_dependencies.json', consumed)
        binary_sha = sha(binary)
        result.update(binary_sha256=binary_sha, compiler_sha256=sha(compiler),
                      depfile_sha256=sha(depfile), compiled_dependency_count=len(consumed))
        def probe(name, n, kmax):
            argv = ['/usr/bin/time', '-v', str(binary), f'--n={n}', '--s=8', f'--kmax={kmax}', '--alias-policy=lazy',
                    '--cache-entries=1000000', '--meb-proposal-supports=unlimited', '--threads=48']
            row = worker.command(name, argv)
            summary = probe_summary((output / (name + '.stdout')).read_text(), row['exit_code'], n, kmax)
            summary.update(command_exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds'])
            need(sha(binary) == binary_sha, 'binary changed')
            save(output / (name + '.summary.json'), summary)
            return summary
        smoke = probe('smoke_n8_k10', 8, 10)
        need(smoke['reported_complete'], 'smoke failed; no heavy run')
        first = probe('n50000_k10', 50000, 10)
        fallback_needed = not first['reported_complete'] or first['elapsed_seconds'] > 1.0
        result.update(k10=first, fallback_needed=fallback_needed, fallback_executed=False)
        if fallback_needed:
            # A distinct fresh process, not a prefix extracted from the K10 run.
            worker.remaining()
            result['fallback_executed'] = True
            result['k5'] = probe('n50000_k5', 50000, 5)
        result['status'] = 'completed' if first['reported_complete'] and (
            not fallback_needed or result['k5']['reported_complete']) else 'failed'
        result['contract_certified'] = False
    except SessionDeadline as error:
        result.update(status='session_deadline', error=str(error))
    except BaseException as error:
        result.update(status='failed', error=type(error).__name__ + ': ' + str(error))
    finally:
        result['commands'] = worker.commands if worker else []
        if 'binary_sha256' in result:
            try:
                result['binary_sha256_after'] = sha(output / 'full_probe')
                if result['binary_sha256_after'] != result['binary_sha256']:
                    result.update(status='failed', binary_changed=True)
            except OSError as error:
                result.update(status='failed', binary_error=str(error))
        if before is not None:
            try:
                after = source_map(root, manifest)
                save(output / 'sources_after.json', after)
                result['sources_stable'] = before == after
            except Exception as error:
                result.update(status='failed', sources_stable=False, source_error=str(error))
        save(output / 'receipt.json', result)
        print(json.dumps(dict(status=result['status'], output=str(output),
                              targeted_GCP_stop_required_by_ROOT=True), sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
