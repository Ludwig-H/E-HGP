#!/usr/bin/env python3
"""Offline publication refusals and arithmetic tests. Never a G4 receipt."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import publish_closed as pub
import readback as read
import recover_closed as recovery_runner


class Guards(unittest.TestCase):
    def test_redaction(self):
        out = pub.redact(b'person@example.test\nssh-ed25519 AAAAExample user\nTERMINATED\n')
        self.assertNotIn(b'person@example', out)
        self.assertNotIn(b'AAAAExample', out)
        self.assertIn(b'TERMINATED', out)

    def test_private_marker(self):
        with self.assertRaisesRegex(ValueError, 'private key'):
            pub.redact(b'-----BEGIN OPENSSH PRIVATE KEY-----')

    def test_live_host(self):
        for receipt in ({'status': 'completed', 'targeted_shutdown_certified': False},
                        {'status': 'worker_returned', 'targeted_shutdown_certified': True}):
            with tempfile.TemporaryDirectory() as temp:
                root = Path(temp); host = root / 'tower_v9_host'; host.mkdir()
                (host / 'receipt.json').write_text(json.dumps(receipt))
                with self.assertRaisesRegex(ValueError, 'closed completed'):
                    pub.publish(host, root / 'package', root / 'after', root / 'out')
                self.assertFalse((root / 'out').exists())

    def test_parent_or_symlink(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with self.assertRaisesRegex(ValueError, 'exact host'):
                pub.publish(root, root / 'package', root / 'after', root / 'out')
            (root / 'tower_v9_host').symlink_to(root, target_is_directory=True)
            with self.assertRaisesRegex(ValueError, 'exact host'):
                pub.publish(root / 'tower_v9_host', root / 'package', root / 'after', root / 'out')

    def test_vm_input_forbidden(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'receipt.json').write_text('{"commands": []}')
            (root / 'scene_00.u32le').write_bytes(b'not for publication')
            with self.assertRaisesRegex(ValueError, 'unexpected VM'):
                pub.vm_files(root)

    def test_fake_preflight_forbidden(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'receipt.json').write_text('{"commands": []}')
            (root / read.worker.PREFLIGHT_FILE).write_bytes(b'not a synthetic fixture')
            with self.assertRaisesRegex(ValueError, 'synthetic fixture'):
                pub.vm_files(root)

    def test_historical_vm_allowlist_layout(self):
        # Read-only layout check, NOT a v30 qualification of this v28 receipt.
        old = read.ROOT / 'morsehgp3D_v9/receipts/g4_tower_r24b_20260926/vm'
        paths = pub.vm_files(old)
        self.assertGreater(len(paths), 100)
        self.assertTrue(all(p.parent == old for p in paths))

    def test_inventory(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); (root / 'data.json').write_text('{}')
            (root / 'SHA256SUMS').write_text(read.sha(root / 'data.json') + '  data.json\n')
            read.check_inventory(root)
            (root / 'extra').write_text('unexpected')
            with self.assertRaisesRegex(ValueError, 'inventory differs'):
                read.check_inventory(root)


class Arithmetic(unittest.TestCase):
    def pair(self):
        levers = {name: False for name in read.worker.LEVER_NAMES}
        case = dict(scene='fixture', file='fixture.u32le', k=5, s=8, n=100, workers=48, static_threads=48,
                    frames=3, repeat=0, levers=levers)
        on = copy.deepcopy(case); on['levers']['q3_interior_payload'] = True
        catalogue = dict(unique_keys=10, balls=10, euler={}, q2_presentations=3, q3_presentations=5,
                         q4_presentations=2, census_nodes=100, census_leaf_tests=60,
                         payload_keys=0, payload_ids=0, payload_fallback_keys=0)
        value = dict(input={'hash': 'fixture', 'sites': 100}, options={'K_effective': 5}, catalogue=catalogue,
                     generator={'q2_accepted_pairs': 3, 'q3_emitted': 5, 'q4_emitted': 2}, ledger={'same': 7}, orders=[],
                     tower_digest='1'*16, catalogue_digest='2'*16, presentation_digest='3'*16,
                     frames={name: [20., 10., 12.] for name in read.worker.FRAME_LISTS},
                     times_ms={'chain_total': 20., 'census': 10.}, q34_batch={}, tower_phases_ms={})
        new = copy.deepcopy(value)
        new['catalogue'].update(census_nodes=40, census_leaf_tests=20, payload_keys=5, payload_ids=9)
        new['times_ms'].update(chain_total=15., census=5.)
        commands = {i: {'exit_code': 0, 'elapsed_seconds': 1.} for i in range(2)}
        return [case, on], [{'outcome': 'complete_relative'}]*2, {0: value, 1: new}, commands

    def test_paired_medians(self):
        args = self.pair()
        with patch.object(read.worker, 'validate_probe', return_value='complete_relative'):
            rows, pairs = read.analyze(*args)
        self.assertEqual(len(rows), 2)
        self.assertEqual(pairs[0]['paired_processes'][0]['chain_delta_ms'], -5)
        self.assertEqual(pairs[0]['timings']['on']['warm_process_median_ms']['chain_total_ms']['median'], 11)
        self.assertEqual(pairs[0]['timings']['on']['processes'], 1)

    def test_ledger_drift_rejected(self):
        args = self.pair(); args[2][1]['ledger']['same'] += 1
        with patch.object(read.worker, 'validate_probe', return_value='complete_relative'), self.assertRaisesRegex(ValueError, 'paired object'):
            read.analyze(*args)

    def test_repeat_mismatch_rejected(self):
        args = self.pair(); args[0][1]['repeat'] = 1
        with patch.object(read.worker, 'validate_probe', return_value='complete_relative'), self.assertRaisesRegex(ValueError, 'per ON repeat'):
            read.analyze(*args)


class Recovery(unittest.TestCase):
    def test_recovery_guard_deadline(self):
        generation = '2026-09-26T20:00:00Z'
        now = read.session.epoch(generation) + 30
        mark = dict(read.worker.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified',
                    generation=generation, date_utc='2026-09-26T20:00:20Z', guest_shutdown_minutes='10', max_run_seconds='3600')
        schedule = {'MODE': 'poweroff', 'USEC': str(int(read.session.epoch(generation) + 600) * 1000000)}
        self.assertEqual(recovery_runner.recovery_deadline(mark, schedule, generation, now),
                         read.session.epoch(generation) + 600)
        for key, value in [('guest_shutdown_minutes', '40'), ('generation', '2026-09-26T19:00:00Z'), ('instance', 'wrong')]:
            bad = dict(mark); bad[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                recovery_runner.recovery_deadline(bad, schedule, generation, now)
        for changed in ({'MODE': 'poweroff', 'USEC': 'oops'},
                        {'MODE': 'poweroff', 'USEC': str(int(now + 300) * 1000000)}):
            with self.subTest(schedule=changed), self.assertRaises(ValueError):
                recovery_runner.recovery_deadline(mark, changed, generation, now)

    def fixture(self, root):
        host, recovery, vm = (root / name for name in ('host', 'recovery', 'vm'))
        for directory in (host, recovery, vm):
            directory.mkdir()
        original_gen, new_gen = '2026-09-26T20:00:00Z', '2026-09-26T20:10:00Z'
        pub.save(host / 'receipt.json', dict(generation=original_gen, status='capture_failed',
                 FULL_executed=False, GPU_executed=False, worker_exit_code=0, targeted_shutdown_certified=True))
        pub.save(vm / 'receipt.json', {'fixture': 'only tests lifecycle arithmetic, never an actual G4 replay'})
        def state(generation, stop):
            target = read.worker.TARGET
            return dict(name=target['instance'], zone='zones/' + target['zone'],
                        selfLink='/projects/' + target['project'] + '/zones/' + target['zone'] + '/instances/' + target['instance'],
                        status='TERMINATED', lastStartTimestamp=generation, lastStopTimestamp=stop,
                        machineType='machineTypes/g4-standard-48', labels={'project': 'e-hgp'},
                        scheduling=dict(provisioningModel='SPOT', instanceTerminationAction='STOP', onHostMaintenance='TERMINATE',
                                        automaticRestart=False, maxRunDuration={'seconds': '3600'}))
        pub.save(recovery / 'original_after_stop.json', state(original_gen, '2026-09-26T20:02:00Z'))
        pub.save(recovery / 'after_stop.json', state(new_gen, '2026-09-26T20:12:00Z'))
        for suffix in ('stdout', 'stderr'):
            (recovery / ('guarded_stop.' + suffix)).write_text('synthetic closed guard log\n')
        stop = dict(name='guarded_stop', argv=['stop', '--expected-last-start-timestamp', new_gen],
                    exit_code=0, group_closed=True, **{suffix + '_sha256': read.sha(recovery / ('guarded_stop.' + suffix))
                                                     for suffix in ('stdout', 'stderr')})
        value = dict(schema='mhgp9_capture_recovery_v1', status='recovered_capture', benchmark_executed=False,
                     targeted_shutdown_certified=True, target=read.worker.TARGET, replayed_original_status='completed',
                     original_generation=original_gen, original_host_receipt_sha256=read.sha(host / 'receipt.json'),
                     original_worker_receipt_sha256=read.sha(vm / 'receipt.json'), generation=new_gen,
                     original_after_stop_observed_at='2026-09-26T20:03:00Z', commands=[stop],
                     original_after_stop_sha256=read.sha(recovery / 'original_after_stop.json'),
                     after_stop_sha256=read.sha(recovery / 'after_stop.json'),
                     verified_guard={'mark': dict(read.worker.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified',
                                      generation=new_gen, date_utc='2026-09-26T20:10:30Z',
                                      guest_shutdown_minutes='10', max_run_seconds='3600'),
                                     'schedule': {'MODE': 'poweroff', 'USEC': str(int(read.session.epoch(new_gen) + 600) * 1000000)}})
        pub.save(recovery / 'receipt.json', value)
        return host, recovery, vm, value

    def test_two_allocations_sum(self):
        with tempfile.TemporaryDirectory() as temp:
            host, recovery, vm, _ = self.fixture(Path(temp))
            original_bytes = (host / 'receipt.json').read_bytes()
            _, cost = read.recovery_evidence(recovery, host, vm, False)
            self.assertEqual(cost['vm_elapsed_seconds'], 240)
            self.assertEqual([s['vm_elapsed_seconds'] for s in cost['sessions']], [120, 120])
            self.assertIsNone(cost['billed_cost_usd'])
            self.assertEqual(original_bytes, (host / 'receipt.json').read_bytes())

    def test_recovery_corruptions(self):
        for key, value, error in (
                ('status', 'started', 'closed copy-only'),
                ('benchmark_executed', True, 'closed copy-only'),
                ('targeted_shutdown_certified', False, 'closed copy-only'),
                ('original_host_receipt_sha256', '0'*64, 'original identity'),
                ('original_worker_receipt_sha256', '0'*64, 'original identity'),
                ('original_after_stop_observed_at', '2026-09-26T20:11:00Z', 'observed before'),
                ('original_after_stop_observed_at', '2026-09-26T20:01:00Z', 'observed before'),
                ('after_stop_sha256', '0'*64, 'raw state hash')):
            with self.subTest(key=key, value=value), tempfile.TemporaryDirectory() as temp:
                host, recovery, vm, receipt = self.fixture(Path(temp))
                receipt[key] = value
                pub.save(recovery / 'receipt.json', receipt)
                with self.assertRaisesRegex(ValueError, error):
                    read.recovery_evidence(recovery, host, vm, False)

    def test_wrong_stop_generation(self):
        with tempfile.TemporaryDirectory() as temp:
            host, recovery, vm, receipt = self.fixture(Path(temp))
            receipt['commands'][0]['argv'][-1] = receipt['original_generation']
            pub.save(recovery / 'receipt.json', receipt)
            with self.assertRaisesRegex(ValueError, 'targeted stop invocation'):
                read.recovery_evidence(recovery, host, vm, False)

    def test_original_not_promoted(self):
        value = dict(status='capture_failed', targeted_shutdown_certified=True, FULL_executed=False,
                     GPU_executed=False, worker_exit_code=0)
        read.closed_host(value, True)
        for key, changed in [('status', 'completed'), ('FULL_executed', True), ('GPU_executed', True)]:
            mutated = dict(value); mutated[key] = changed
            with self.subTest(key=key), self.assertRaises(ValueError):
                read.closed_host(mutated, True)

    def test_safe_state_projection(self):
        self.assertEqual(pub.projection({'status': 'TERMINATED', 'metadata': {'ssh-keys': 'never publish'},
                                         'networkInterfaces': ['private'], 'serviceAccounts': ['private']}),
                         {'status': 'TERMINATED'})

if __name__ == '__main__':
    unittest.main()
