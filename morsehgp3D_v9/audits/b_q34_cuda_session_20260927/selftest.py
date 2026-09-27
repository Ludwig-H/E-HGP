#!/usr/bin/env python3
"""Offline protocol tests. Synthetic receipts are never measurement evidence.

Temporary archives use the existing pinned input, never a new tracked LiDAR
payload. Git streams and process operations are mocked; cloud calls forbidden.
"""
from contextlib import redirect_stdout
from copy import deepcopy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import signal
import subprocess
import sys
import tarfile
import tempfile
from unittest.mock import patch
sys.dont_write_bytecode = True
import common as c
import package
import session
import worker


class Checks:
    def __init__(self):
        self.positive = 0
        self.rejected = 0
        self.labels = []

    def yes(self, condition, label):
        c.need(condition, label)
        self.positive += 1

    def no(self, function, label):
        try:
            function()
        except (ValueError, KeyError, TypeError, OSError, tarfile.TarError):
            self.rejected += 1
            self.labels.append(label)
        else:
            raise ValueError('negative admitted: ' + label)


def encoded(value):
    return (json.dumps(value, sort_keys=True, allow_nan=False) + '\n').encode()


def pins(files):
    return {name: hashlib.sha256(raw).hexdigest() for name, raw in files.items()}


def archive(path, files, extra=()):
    with tarfile.open(path, 'w:gz') as target:
        for name, raw in files.items():
            item = tarfile.TarInfo(name)
            item.size = len(raw)
            target.addfile(item, io.BytesIO(raw))
        for item, raw in extra:
            target.addfile(item, io.BytesIO(raw))


def artifact_tests(directory, checks):
    commit, tree = 'a'*40, 'b'*40
    data = (c.ROOT / c.DATA_SOURCE).read_bytes()
    files = {name: (c.ROOT / name).read_bytes() for name in (
        c.HELPER, c.PREFIX + '/common.py', c.PREFIX + '/worker.py',
        c.PREFIX + '/session.py', c.PREFIX + '/package.py', c.PROTOTYPE + '/CMakeLists.txt')}
    files[c.DATA] = data
    files[c.PROVENANCE] = encoded(dict(schema=c.SCHEMA, scope='S2_only_no_FULL',
        protocol_source='commit', commit=commit, tree=tree))
    target = directory / 'positive.tar.gz'
    archive(target, files)
    manifest = pins(files)
    parsed, provenance = c.unpack_readonly(target, manifest)
    checks.yes(parsed == files and provenance['commit'] == commit, 'private snapshot positive')

    # A synthetic Git batch is byte-framed and content-addressed exactly as
    # git cat-file, but no subprocess or repository write is made.
    tracked = dict(files)
    tracked[c.DATA_SOURCE] = tracked.pop(c.DATA)
    tracked.pop(c.PROVENANCE)
    blobs = {hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest(): raw
             for raw in tracked.values()}
    entries = {name: hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
               for name, raw in tracked.items()}
    calls = []

    def git(*args, stdin=None):
        calls.append(args)
        if args == ('rev-parse', '--verify', commit + '^{commit}'):
            return (commit + '\n').encode()
        if args == ('rev-parse', '--verify', commit + '^{tree}'):
            return (tree + '\n').encode()
        if args[:4] == ('ls-tree', '-r', '-z', '--full-tree'):
            return b''.join(('100644 blob ' + oid + '\t' + name + '\0').encode() for name, oid in entries.items())
        if args == ('cat-file', '--batch'):
            return b''.join(oid.encode() + b' blob ' + str(len(blobs[oid])).encode() + b'\n' + blobs[oid] + b'\n'
                            for oid in stdin.decode().splitlines())
        raise ValueError('unexpected fake git command')

    with patch.object(package, 'git', side_effect=git):
        collected = package.collect(commit)
        checks.yes(collected == files and len(calls) == 4, 'reproducible Git object stream')
    with patch.object(package, 'collect', return_value=files):
        checks.yes(package.verify_committed(target, manifest) == provenance, 'committed package positive')
        changed = dict(files)
        changed[c.PROTOTYPE + '/CMakeLists.txt'] += b'# tamper\n'
        with patch.object(package, 'collect', return_value=changed):
            checks.no(lambda: package.verify_committed(target, manifest), 'Git source reproduction')
    checks.no(lambda: package.collect('HEAD'), 'full commit required')
    bad_manifest = dict(manifest)
    bad_manifest[c.DATA] = '0'*64
    checks.no(lambda: c.unpack_readonly(target, bad_manifest), 'member digest')
    checks.no(lambda: c.unpack_readonly(target, manifest | {'ghost': '0'*64}), 'exhaustive manifest')
    checks.no(lambda: c.unpack_readonly(target, {k: v for k, v in manifest.items() if k != c.DATA}), 'missing manifest member')
    for label, transform in (
        ('short frame', lambda f: f.__setitem__(c.DATA, data[:-12])),
        ('wrong helper', lambda f: f.__setitem__(c.HELPER, b'not the pinned helper')),
        ('uncommitted provenance', lambda f: f.__setitem__(c.PROVENANCE, encoded(dict(provenance, protocol_source='worktree')))),
        ('FULL provenance', lambda f: f.__setitem__(c.PROVENANCE, encoded(dict(provenance, scope='FULL')))),
        ('duplicate provenance', lambda f: f.__setitem__(c.PROVENANCE, b'{"schema":0,"schema":1}')),
        ('required missing', lambda f: f.pop(c.PREFIX + '/worker.py'))):
        changed = dict(files)
        transform(changed)
        path = directory / (label.replace(' ', '_') + '.tar.gz')
        archive(path, changed)
        checks.no(lambda: c.unpack_readonly(path, pins(changed)), label)
    for ordinal, name in enumerate(('/absolute', '../escape', c.PREFIX + '/../escape',
            './' + c.PREFIX + '/file', c.PREFIX + '//file', 'unknown.py', c.DATA)):
        item = tarfile.TarInfo(name)
        item.size = 1
        path = directory / ('path_' + str(ordinal) + '.tar.gz')
        archive(path, files, [(item, b'x')])
        checks.no(lambda: c.unpack_readonly(path, manifest | {name: hashlib.sha256(b'x').hexdigest()}), 'unsafe path/' + name)
    for ordinal, kind in enumerate((tarfile.SYMTYPE, tarfile.LNKTYPE, tarfile.CHRTYPE, tarfile.DIRTYPE)):
        item = tarfile.TarInfo(c.PREFIX + '/link')
        item.type, item.linkname = kind, '/outside'
        path = directory / ('kind_' + str(ordinal) + '.tar.gz')
        archive(path, files, [(item, b'')])
        checks.no(lambda: c.unpack_readonly(path, manifest), 'nonregular archive/' + str(ordinal))
    duplicate = directory / 'duplicate.json'
    duplicate.write_bytes(b'{"a":{"x":1,"x":2}}')
    checks.no(lambda: c.read(duplicate), 'nested duplicate JSON')
    return files, manifest


def probe_values():
    gate = c.read(c.ROOT / 'morsehgp3D_v9/receipts/q34_cuda_waves_20260927/r2/gate_release.stdout')
    gate['cuda_executed'] = True  # Synthetic protocol input, NOT device evidence.
    frame = dict(schema='mhgp9_q34_cuda_waves_v1', status='passed', mode='frame', cuda_executed=True,
        n=39885, k=5, s=8, Q_requested=262144, Q_actual=262144, input_hash_u64=9245360528374966039,
        P=23686751, P3=17732794, P4=23446295, E=9122704, E3=6667094, E4=8403884, S=2043612,
        digest_u64=123, physical_queries=9122704, waves=35, visits=50000000,
        preparation_workers=4, reference_workers=4, device='SYNTHETIC_NOT_A_MEASUREMENT',
        prepared_retained_bytes=100, snapshot_array_bytes=100, device_bytes=100,
        upload_bytes=100, download_bytes=100)
    frame['times_ms'] = {key: 1.0 for key in ('input', 'index', 'front', 'preparation', 'snapshot',
        'upload_allocate', 'waves_including_count_download', 'survivor_download_allocate', 'order_convert',
        'runner_release', 'reference', 'comparison', 'destruction')}
    frame['times_ms'].update(cuda_runner=10.0, total=100.0)
    return gate, frame


def probe_tests(checks, gate, frame):
    for mode, original in (('gate', gate), ('frame', frame)):
        checks.yes(worker.validate_probe(original, mode) == original, 'synthetic valid ' + mode)
        for field, wrong in (('schema', 'FULL'), ('status', 'failed'), ('mode', 'other'),
                             ('cuda_executed', False), ('cuda_executed', 1)):
            bad = deepcopy(original)
            bad[field] = wrong
            checks.no(lambda: worker.validate_probe(bad, mode), mode + '/' + field)
    for field in ('cases', 'runs', 'queries', 'survivors', 'planned', 'fallbacks', 'empty',
                  'zero_output', 'reordered', 'mixed_masks', 'pool_rejected', 'pool_lane_reduced', 'fallback_survivors'):
        bad = dict(gate, **{field: 0})
        checks.no(lambda: worker.validate_probe(bad, 'gate'), 'vacuous gate/' + field)
    for field in ('n', 'k', 's', 'Q_requested', 'input_hash_u64', 'P', 'E', 'S', 'physical_queries', 'waves'):
        bad = dict(frame, **{field: frame[field]+1})
        checks.no(lambda: worker.validate_probe(bad, 'frame'), 'wrong frame/' + field)
    for value in (-1.0, float('nan'), float('inf'), True):
        bad = deepcopy(frame)
        bad['times_ms']['cuda_runner'] = value
        checks.no(lambda: worker.validate_probe(bad, 'frame'), 'invalid timing/' + str(value))
    for field, wrong in (('Q_actual', 0), ('Q_actual', 262145), ('E3', frame['P3']+1),
                         ('E4', -1), ('P4', frame['P']+1), ('n', 39885.0)):
        bad = dict(frame, **{field: wrong})
        checks.no(lambda: worker.validate_probe(bad, 'frame'), 'invalid numeric/' + field)


def forbidden(*_args, **_kwargs):
    raise ValueError('selftest forbids every real subprocess')


def legacy_test(checks):
    path = c.ROOT / 'gcp-migration/selftest_full_probe_session_v7.py'
    spec = importlib.util.spec_from_file_location('cuda_waves_legacy_selftest', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    output = io.StringIO()
    with redirect_stdout(output):
        module.main()
    value = json.loads(output.getvalue())
    checks.yes(value['status'] == 'passed' and value['real_subprocesses'] == 0 and
               value['mocked_commands'] == 3 and value['GCP_used'] is False, 'legacy cleanup mocks')
    return value


def received_tests(directory, checks, input_files, gate, frame):
    host = directory / 'host'
    output = host / 'received/output'
    output.mkdir(parents=True)
    (host / 'guardmarks').mkdir()
    files = dict(input_files)
    required = (c.PROTOTYPE + '/probe.cpp', c.PROTOTYPE + '/runner.cu',
                'morsehgp3D_v9/src/gpu/witness_filter.hpp')
    for name in required:
        files[name] = (c.ROOT / name).read_bytes()
    manifest = pins(files)
    archive(host / 'snapshot.tar.gz', files)
    c.save(host / 'source_manifest.json', manifest)
    remote = '/tmp/ehgp-full-v7-0123456789abcdef.ABCdef1234'
    root, build = Path(remote) / 'source', Path(remote) / 'cuda_waves_build'
    c.save(host / 'receipt.json', dict(remote_directory=remote))
    generation = '2026-09-27T10:00:00.123456Z'
    legacy = c.load_legacy()
    mark = dict(c.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified',
        generation=generation, max_run_seconds='3600', guest_shutdown_minutes='30',
        date_utc='2026-09-27T10:02:00Z')
    schedule = dict(MODE='poweroff', USEC=str(int((legacy.epoch(generation)+1900)*1000000)))
    fields = lambda value: ''.join(key+'='+item+'\n' for key, item in value.items()).encode()
    (host / 'guardmarks/double_guard_verified').write_bytes(fields(mark))
    (host / 'guest_schedule.stdout').write_bytes(fields(schedule))
    c.save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule))
    c.save(output / 'sources_before.json', manifest)
    c.save(output / 'sources_after.json', manifest)
    dependencies = {str(root / name): manifest[name] for name in required}
    dependencies['/usr/include/stdint.h'] = 'e'*64
    c.save(output / 'compiled_dependencies.json', dependencies)
    commands = []
    for ordinal, (name, argv) in enumerate(worker.recipes(root, build, c).items()):
        raw = encoded(gate if name == 'device_gate' else frame) if name in ('device_gate', 'ng00') else b'mocked command\n'
        (output / (name + '.stdout')).write_bytes(raw)
        (output / (name + '.stderr')).write_bytes(b'')
        row = dict(name=name, argv=argv, exit_code=0, group_closed=True,
            started_epoch=ordinal, ended_epoch=ordinal+0.5,
            stdout_sha256=c.sha(output / (name + '.stdout')), stderr_sha256=c.sha(output / (name + '.stderr')))
        commands.append(row)
        c.save(output / (name + '.command.json'), row)
        c.save(output / (name + '.intent.json'), dict(name=name, argv=argv, started_epoch=ordinal))
    receipt = dict(schema=c.SCHEMA, status='completed', scope='S2_only_no_FULL', FULL_executed=False,
        contract_certified=False, public_status='not_claimed', target=c.TARGET, generation=generation,
        useful_budget_seconds=worker.USEFUL_SECONDS, CUDA_installation_attempted=False,
        worker_sha256=manifest[c.PREFIX+'/worker.py'], source_manifest_sha256=c.sha(host/'source_manifest.json'),
        sources_stable=True, compiled_dependencies_stable=True, binary_stable=True,
        provenance=json.loads(files[c.PROVENANCE]), compiled_dependencies_sha256=c.sha(output/'compiled_dependencies.json'),
        binary_sha256='f'*64, commands=commands, gate=gate, measure=frame)
    c.save(output/'receipt.json', receipt)
    check = lambda: session.validate_received(host, manifest, generation)
    checks.yes(check() == receipt, 'synthetic fully bound received receipt')
    baseline = {path: path.read_bytes() for path in host.rglob('*') if path.is_file()}

    def trial(label, mutate):
        mutate()
        checks.no(check, 'received/' + label)
        for path, raw in baseline.items():
            path.write_bytes(raw)

    def change_receipt(key, value):
        (output/'receipt.json').write_bytes(encoded(dict(receipt, **{key: value})))

    for key, value in (('FULL_executed', True), ('contract_certified', True), ('status', 'failed'),
                       ('target', dict(c.TARGET, instance='different')), ('generation', 'other'),
                       ('sources_stable', False), ('compiled_dependencies_stable', False),
                       ('worker_sha256', '0'*64), ('binary_sha256', ''), ('provenance', {})):
        trial(key, lambda key=key, value=value: change_receipt(key, value))
    trial('incomplete command inventory', lambda: change_receipt('commands', commands[:-1]))
    trial('manifest closure', lambda: (output/'sources_after.json').write_bytes(encoded({})))
    trial('stream changed', lambda: (output/'ng00.stdout').write_bytes(b'{}'))
    trial('remote traversal', lambda: (host/'receipt.json').write_bytes(encoded(dict(remote_directory=remote+'/../other'))))

    def command_mutation(field, value):
        changed = deepcopy(receipt)
        row = changed['commands'][-1]
        row[field] = value
        (output/'receipt.json').write_bytes(encoded(changed))
        (output/'ng00.command.json').write_bytes(encoded(row))
        (output/'ng00.intent.json').write_bytes(encoded(dict(name=row['name'], argv=row['argv'], started_epoch=row['started_epoch'])))
    trial('recipe changed even with matching intent', lambda: command_mutation('argv', commands[-1]['argv'][:-1]+['--s=10']))
    trial('failed command', lambda: command_mutation('exit_code', 2))
    trial('unclosed group', lambda: command_mutation('group_closed', False))

    def dependency_mutation(missing=False):
        changed = dict(dependencies)
        key = str(root / (c.PROTOTYPE+'/runner.cu'))
        if missing:
            changed.pop(key)
        else:
            changed[key] = '0'*64
        (output/'compiled_dependencies.json').write_bytes(encoded(changed))
        change_receipt('compiled_dependencies_sha256', c.sha(output/'compiled_dependencies.json'))
    trial('compiled source mismatch with repinned inventory', dependency_mutation)
    trial('device dependency missing with repinned inventory', lambda: dependency_mutation(True))

    def guard_mutation():
        bad = dict(mark, mark='guest_guard_pending')
        (host/'guardmarks/double_guard_verified').write_bytes(fields(bad))
        (output/'guard_evidence.json').write_bytes(encoded(dict(mark=bad, schedule=schedule)))
    trial('matching but uncertified guards', guard_mutation)
    checks.yes(check() == receipt, 'baseline restored after receipt mutations')


def wait_tests(directory, checks):
    for kind in ('success', 'timeout', 'pid_write_failure', 'handler_install_failure', 'wait_interrupt'):
        calls = []
        class Process:
            pid = 123456
            done = False
            def poll(self):
                return 0 if self.done else None
            def wait(self, timeout=None):
                calls.append(('wait', timeout))
                if timeout is not None and kind == 'timeout':
                    raise subprocess.TimeoutExpired('mocked-controller', timeout)
                if timeout is not None and kind == 'wait_interrupt':
                    raise InterruptedError('mocked interruption')
                self.done = True
                return 0
            def send_signal(self, signum):
                calls.append(('signal', signum))
        process = Process()
        installed = []
        def signal_install(signum, handler):
            installed.append((signum, handler))
            if kind == 'handler_install_failure' and len(installed) == 2:
                raise OSError('mocked signal installation')
            return signal.SIG_DFL
        def save(_path, _value):
            if kind == 'pid_write_failure':
                raise OSError('mocked PID write failure')
        error = False
        with patch.object(session.signal, 'signal', side_effect=signal_install), patch.object(session.c, 'save', side_effect=save):
            try:
                code = session.wait_owned(process, directory)
                checks.yes(code == 0 and kind in ('success', 'timeout'), 'wait returned ' + kind)
            except (OSError, InterruptedError):
                error = True
        checks.yes(process.done and any(name == 'wait' for name, _ in calls), 'controller joined ' + kind)
        checks.yes(error == (kind in ('pid_write_failure', 'handler_install_failure', 'wait_interrupt')), 'error preserved ' + kind)
        signals = [value for name, value in calls if name == 'signal']
        checks.yes(signals == ([] if kind == 'success' else [signal.SIGINT]), 'only cooperative SIGINT ' + kind)


def main():
    checks = Checks()
    paths = [c.HERE/name for name in ('common.py', 'package.py', 'worker.py', 'session.py', 'selftest.py')]
    paths += [c.ROOT/c.HELPER, c.ROOT/'gcp-migration/selftest_full_probe_session_v7.py']
    before = {str(path): c.sha(path) for path in paths}
    with tempfile.TemporaryDirectory(prefix='mhgp9-cuda-session-selftest-') as temporary, \
         patch.object(subprocess, 'run', side_effect=forbidden), patch.object(subprocess, 'Popen', side_effect=forbidden):
        directory = Path(temporary)
        files, _manifest = artifact_tests(directory, checks)
        gate, frame = probe_values()
        probe_tests(checks, gate, frame)
        received_tests(directory, checks, files, gate, frame)
        wait_tests(directory, checks)
        legacy = legacy_test(checks)
    checks.yes(before == {str(path): c.sha(path) for path in paths}, 'source stability during offline selftest')
    print(json.dumps(dict(schema=c.SCHEMA, status='passed', positive=checks.positive,
        rejected=checks.rejected, mutation_labels=checks.labels, controller_wait_scenarios=5,
        legacy=legacy, GCP_used=False, real_subprocesses=0, measured_data=False,
        source_sha256=before), sort_keys=True))


if __name__ == '__main__':
    main()
