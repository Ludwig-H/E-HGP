#!/usr/bin/env python3
"""Pure/local tests: cloud commands, SSH, compilers and VM starts are mocked.

These tests qualify adapter predicates/closure only, not a G4 benchmark.
"""
from contextlib import redirect_stdout
from copy import deepcopy
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile
import tempfile
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import cpu_probe_session_v8 as session
import cpu_probe_worker_v8 as worker


def target(status='TERMINATED', generation=None):
    result = dict(name=worker.TARGET['instance'], zone=worker.TARGET['zone'], status=status,
                  selfLink='https://compute.googleapis.com/compute/v1/projects/' + worker.TARGET['project'] +
                  '/zones/' + worker.TARGET['zone'] + '/instances/' + worker.TARGET['instance'],
                  labels={'project': 'e-hgp'}, machineType='g4-standard-48',
                  scheduling=dict(provisioningModel='SPOT', instanceTerminationAction='STOP',
                      onHostMaintenance='TERMINATE', automaticRestart=False, maxRunDuration={'seconds': '3600'}))
    if generation is not None:
        result['lastStartTimestamp'] = generation
    return result


def fixture(directory, extra=None):
    plan = dict(schema='mhgp8_cpu_probe_plan_v1', cases=[dict(file='data/scan0.u16le', n=2,
                        k=10, s=8, mask=6, backend=28, workers=48)])
    files = {name: b'// predicate fixture, never compiled\n' for name in (*worker.LIBRARY, *worker.PROGRAMS.values())}
    files[worker.HELPER] = (Path(__file__).parent / 'full_probe_worker_v7.py').read_bytes()
    files[worker.PLAN] = json.dumps(plan).encode()
    files['data/scan0.u16le'] = bytes(12)
    manifest = {name: hashlib.sha256(raw).hexdigest() for name, raw in files.items()}
    archive = directory / 'snapshot.tar.gz'
    with tarfile.open(archive, 'w:gz') as tar:
        for name, raw in files.items():
            member = tarfile.TarInfo(name)
            member.size = len(raw)
            tar.addfile(member, io.BytesIO(raw))
        if extra is not None:
            tar.addfile(extra)
    manifest_path = directory / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest))
    return archive, manifest_path, manifest, plan


class Predicates(unittest.TestCase):
    def test_gate_invocation(self):
        self.assertEqual(worker.gate_command(Path('/private/build')),
                         ['/private/build/mhgp8_wspd_q34_gate', '--selftest'])

    def test_cross_worker_projection(self):
        value = dict(input_hash=1, output={'q3': 2}, front={'a': 3}, cloud_work={}, index_work={},
                     work={'peak_edge_buffer_bytes': 10, 'q3': {'peak_shell_bytes': 20, 'seeds': 3}})
        other = deepcopy(value)
        other['work']['peak_edge_buffer_bytes'] = 100
        other['work']['q3']['peak_shell_bytes'] = 200
        self.assertEqual(worker.logical_result(value), worker.logical_result(other))
        other['work']['q3']['seeds'] += 1
        self.assertNotEqual(worker.logical_result(value), worker.logical_result(other))

    def test_inert_without_cloud(self):
        with patch('subprocess.Popen', side_effect=RuntimeError('no subprocess permitted')), redirect_stdout(io.StringIO()):
            with patch.object(sys, 'argv', ['controller']):
                self.assertEqual(session.main(), 0)
            with patch.object(sys, 'argv', ['worker']):
                self.assertEqual(worker.main(), 0)

    def test_target_mutations(self):
        value = target()
        session.validate_target(value, 'TERMINATED')
        mutations = [('name', 'other'), ('status', 'RUNNING'), ('zone', 'other'),
                     ('selfLink', 'wrong-project'), ('machineType', 'n2-standard-48')]
        for field, replacement in mutations:
            bad = deepcopy(value)
            bad[field] = replacement
            with self.subTest(field=field), self.assertRaises(ValueError):
                session.validate_target(bad, 'TERMINATED')
        for field, replacement in [('provisioningModel', 'STANDARD'), ('instanceTerminationAction', 'DELETE'),
                                   ('onHostMaintenance', 'MIGRATE'), ('automaticRestart', True),
                                   ('maxRunDuration', {'seconds': '3601'}), ('maxRunDuration', {'seconds': '3600', 'nanos': 1})]:
            bad = deepcopy(value)
            bad['scheduling'][field] = replacement
            with self.subTest(field=field), self.assertRaises(ValueError):
                session.validate_target(bad, 'TERMINATED')
        with self.assertRaises(ValueError):
            session.validate_target(target('RUNNING', 'first'), 'RUNNING', 'other')

    def test_plan_and_snapshot(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            archive, _, manifest, plan = fixture(directory)
            session.validate_snapshot(archive, manifest)
            for key, value in [('n', 0), ('n', True), ('k', 11), ('s', 9), ('mask', 1),
                               ('backend', 99), ('workers', 49), ('file', '../scan.u16le')]:
                bad = deepcopy(plan)
                bad['cases'][0][key] = value
                with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                    worker.validate_plan(bad, manifest)
            bad_manifest = dict(manifest)
            del bad_manifest[worker.LIBRARY[0]]
            with self.assertRaises(ValueError):
                worker.validate_manifest(bad_manifest)
            for name, kind in [('data/../bad', tarfile.REGTYPE), ('data/link', tarfile.SYMTYPE),
                               ('/data/bad', tarfile.REGTYPE), ('morsehgp3D_v7/x', tarfile.REGTYPE),
                               (worker.PLAN, tarfile.REGTYPE)]:
                member = tarfile.TarInfo(name)
                member.type = kind
                fixture(directory, member)
                with self.subTest(name=name), self.assertRaises(ValueError):
                    session.validate_snapshot(archive, manifest)

    def test_exact_guard_deadline(self):
        now = int(time.time())
        generation = datetime.fromtimestamp(now-30, timezone.utc).isoformat()
        mark = dict(worker.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified',
                    generation=generation, max_run_seconds='3600', guest_shutdown_minutes='30', date_utc=generation)
        schedule = {'MODE': 'poweroff', 'USEC': str((now+1800)*1000000)}
        self.assertEqual(session.guard_deadline(mark, schedule, generation, now), now+1800)
        for field, value in [('max_run_seconds', '7200'), ('guest_shutdown_minutes', '31'), ('generation', 'other')]:
            bad = dict(mark)
            bad[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                session.guard_deadline(bad, schedule, generation, now)
        with self.assertRaises(ValueError):
            session.closure_generation(generation, generation.replace('+00:00', '+01:00'))

    def test_failure_stops_exact_generation(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            archive, manifest_path, _, _ = fixture(directory)
            private = directory / 'session'
            private.mkdir(mode=0o700)
            key = directory / 'key'
            key.write_text('fixture-key-never-used')
            key.chmod(0o600)
            Path(str(key)+'.pub').write_text('fixture-public-key')
            generation = datetime.now(timezone.utc).isoformat()
            calls = []
            class MockCommands:
                def __init__(self, host, _env):
                    self.host, self.rows = host, []
                def run(self, name, argv, **_kwargs):
                    calls.append((name, list(map(str, argv))))
                    if name == 'before_start':
                        return 0, json.dumps(target()), ''
                    if name == 'guarded_start':
                        handoff = dict(worker.TARGET, schema='e-hgp.start-handoff.v3', last_start_timestamp=generation)
                        (self.host/'handoff.json').write_text(json.dumps(handoff))
                        mark = dict(worker.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified',
                                    generation=generation, max_run_seconds='3600', guest_shutdown_minutes='30', date_utc=generation)
                        (self.host/'guardmarks/double_guard_verified').write_text(''.join(k+'='+v+'\n' for k,v in mark.items()))
                        expiration = datetime.fromtimestamp(time.time()+4200, timezone.utc).strftime('%Y-%m-%dT%H:%M:%S.%fZ')
                        return 0, 'expiration fixe='+expiration, ''
                    if name == 'before_upload':
                        return 0, json.dumps(target('RUNNING', generation)), ''
                    if name == 'guest_schedule':
                        return 0, 'MODE=poweroff\nUSEC='+str((int(time.time())+1800)*1000000)+'\n', ''
                    if name == 'remote_mkdir':
                        template = str(argv[-1]).split('mktemp -d ', 1)[1]
                        return 0, template[:-10]+'ABCDEFGHIJ\n', ''
                    if name == 'upload':
                        return 1, '', 'intentional failure after guarded start'
                    if name == 'guarded_stop':
                        self_generation = str(argv[-1])
                        if self_generation != generation or argv[-2] != '--expected-last-start-timestamp':
                            raise ValueError('unversioned/wrong-generation stop')
                    return 0, '', ''
            args = SimpleNamespace(session_dir=private, ssh_key=key,
                expected_controller_sha256=worker.sha(session.__file__), worker=Path(worker.__file__),
                worker_sha256=worker.sha(worker.__file__), snapshot=archive, snapshot_sha256=worker.sha(archive),
                manifest=manifest_path, manifest_sha256=worker.sha(manifest_path), gcloud=Path('/never-run/gcloud'))
            with patch.object(session, 'Commands', MockCommands), redirect_stdout(io.StringIO()):
                self.assertEqual(session.run_session(args), 1)
            receipt = json.loads((private/'cpu_v8_host/receipt.json').read_text())
            self.assertTrue(receipt['targeted_shutdown_certified'])
            self.assertEqual(receipt['generation'], generation)
            self.assertEqual([name for name, _ in calls].count('guarded_stop'), 1)
            self.assertNotIn('worker', [name for name, _ in calls])


if __name__ == '__main__':
    unittest.main()
