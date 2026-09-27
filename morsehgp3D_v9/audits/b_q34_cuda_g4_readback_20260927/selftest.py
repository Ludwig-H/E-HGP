#!/usr/bin/env python3
"""Temporary synthetic round trips, not a GPU measurement or cloud operation."""
from copy import deepcopy
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
from unittest.mock import patch
import readback as r


def main():
    checks = dict(positive=0, refused=0)
    def yes(condition, label):
        r.need(condition, label)
        checks['positive'] += 1
    def no(fn, label):
        try:
            fn()
        except (ValueError, KeyError, TypeError, OSError):
            checks['refused'] += 1
        else:
            raise ValueError('negative admitted: '+label)
    def forbidden(*_a, **_kw):
        raise ValueError('real process forbidden')
    def put(path, value):
        path.write_bytes(r.encode(value))
    spec = importlib.util.spec_from_file_location('cuda_g4_fixture_builder', r.PROTOCOL/'selftest.py')
    fixtures = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixtures)
    with tempfile.TemporaryDirectory(prefix='mhgp9-cuda-g4-reader-test-') as tmp, \
         patch.object(subprocess, 'run', side_effect=forbidden), patch.object(subprocess, 'Popen', side_effect=forbidden):
        temp = Path(tmp)
        fixture_checks = fixtures.Checks()
        files, _ = fixtures.artifact_tests(temp, fixture_checks)
        gate, frame = fixtures.probe_values()
        fixtures.received_tests(temp, fixture_checks, files, gate, frame)
        original = temp/'private_session'
        original.mkdir()
        host = original/'full_host'
        (temp/'host').rename(host)
        vm = host/'received/output'
        manifest = r.read(host/'source_manifest.json')
        snapshot_files, provenance = r.c.unpack_readonly(host/'snapshot.tar.gz', manifest)
        packaged = temp/'package'
        packaged.mkdir()
        for name in ('source_manifest.json', 'snapshot.tar.gz'):
            (packaged/name).write_bytes((host/name).read_bytes())
        metadata = dict(snapshot_sha256=r.sha(packaged/'snapshot.tar.gz'),
            manifest_sha256=r.sha(packaged/'source_manifest.json'), provenance=provenance,
            worker_sha256=manifest[r.c.PREFIX+'/worker.py'])
        put(packaged/'PACKAGE.json', metadata)
        generation = r.read(vm/'receipt.json')['generation']
        for name in r.c.load_legacy().GUARDS:
            (host/name).write_bytes((r.c.ROOT/'gcp-migration'/name).read_bytes())
        row = dict(name='guarded_stop', argv=[str(host/'stop_and_verify.sh'), '--yes', '--expected-last-start-timestamp', generation],
            exit_code=0, group_closed=True, started_epoch=1.0, ended_epoch=2.0)
        for suffix, raw in (('stdout', b'Instance external IP is 203.0.113.1\naccount@example.net\nSTOPPED\n'), ('stderr', b'')):
            (host/('guarded_stop.'+suffix)).write_bytes(raw)
            row[suffix+'_sha256'] = r.sha(host/('guarded_stop.'+suffix))
        put(host/'guarded_stop.command.json', row)
        put(host/'guarded_stop.intent.json', dict(name=row['name'], argv=row['argv'], started_epoch=row['started_epoch']))
        start_row = dict(name='guarded_start', argv=[str(host/'start_and_verify.sh'), '--yes', '--guest-shutdown-minutes', '30',
            '--handoff-file', str(host/'handoff.json'), '--lifecycle-state-file', str(host/'lifecycle.txt'),
            '--guard-mark-dir', str(host/'guardmarks')], exit_code=0, group_closed=True, started_epoch=0.0, ended_epoch=0.5)
        for suffix in ('stdout', 'stderr'):
            (host/('guarded_start.'+suffix)).write_bytes(b'')
            start_row[suffix+'_sha256'] = r.sha(host/('guarded_start.'+suffix))
        put(host/'guarded_start.command.json', start_row)
        put(host/'guarded_start.intent.json', dict(name=start_row['name'], argv=start_row['argv'], started_epoch=start_row['started_epoch']))
        receipt = dict(r.read(host/'receipt.json'), target=r.c.TARGET, status='completed',
            generation=generation, targeted_shutdown_certified=True, commands=[start_row, row],
            snapshot_sha256=metadata['snapshot_sha256'], manifest_sha256=metadata['manifest_sha256'],
            worker_sha256=metadata['worker_sha256'], controller_sha256=r.c.HELPER_PIN,
            capture_received=True, worker_receipt_present=True, worker_exit_code=0)
        (host/'capture.tar.gz').write_bytes(b'synthetic capture identity only; never exported')
        receipt['capture_sha256'] = r.sha(host/'capture.tar.gz')
        put(host/'receipt.json', receipt)
        t = r.c.TARGET
        after = dict(name=t['instance'], zone='projects/'+t['project']+'/zones/'+t['zone'],
            selfLink='https://www.googleapis.com/compute/v1/projects/'+t['project']+'/zones/'+t['zone']+'/instances/'+t['instance'],
            status='TERMINATED', lastStartTimestamp=generation, lastStopTimestamp='2026-09-27T10:10:00.123456Z',
            metadata={'sshKeys': 'MUST_NOT_EXPORT'}, networkInterfaces=[{'networkIP': '10.0.0.1'}])
        put(original/'after_stop.json', after)
        with patch.object(r.package, 'collect', return_value=snapshot_files):
            public, summary = r.collect(original, packaged)
            yes(summary['status'] == 'completed' and summary['scope'] == 'S2_only_no_FULL' and
                summary['FULL_executed'] is False and summary['S2_construction_to_output_ms'] == 12, 'S2-only semantic replay')
            yes('metadata' not in r.projection(after) and 'networkInterfaces' not in r.projection(after), 'safe state projection')
            joined = b''.join(public.values())
            yes(b'203.0.113.1' not in joined and b'account@example.net' not in joined and b'MUST_NOT_EXPORT' not in joined,
                'redacted private host values')
            yes(not any(name.endswith(('.tar.gz', '.u32le', '.pub')) for name in public), 'no archive/input/key exported')
            published = temp/'published'
            yes(r.export(original, packaged, published) == summary and r.replay(published) == summary, 'LIVE export/replay')
            original_summary = (published/'SUMMARY.json').read_bytes()
            put(published/'SUMMARY.json', dict(summary, FULL_executed=True))
            no(lambda: r.replay(published), 'changed publication')
            (published/'SUMMARY.json').write_bytes(original_summary)
            (published/'unexpected.txt').write_text('not allowlisted')
            no(lambda: r.replay(published), 'additional file')
            (published/'unexpected.txt').unlink()
            (vm/'input.u32le').write_bytes(b'forbidden')
            no(lambda: r.collect(original, packaged), 'unexpected VM payload')
            (vm/'input.u32le').unlink()
            worker_raw = (vm/'receipt.json').read_bytes()
            failed_worker = r.read(vm/'receipt.json')
            failed_worker.update(status='failed', error='SYNTHETIC compilation failure')
            put(vm/'receipt.json', failed_worker)
            put(host/'receipt.json', dict(receipt, status='worker_failed', worker_exit_code=2))
            with patch.object(r.session, 'validate_received', side_effect=ValueError('must not promote failed input')) as validator:
                failed_public, failed_summary = r.collect(original, packaged)
                yes(failed_summary['status'] == 'failed' and not failed_summary['semantic_replay'] and
                    not validator.called and 'measure' not in failed_summary, 'failed session never promoted')
                yes(failed_public['vm/receipt.json'] == (vm/'receipt.json').read_bytes(), 'raw failure preserved')
                failed_dir = temp/'failed_publication'
                yes(r.export(original, packaged, failed_dir) == failed_summary and r.replay(failed_dir) == failed_summary,
                    'failed publication round trip')
            (vm/'receipt.json').write_bytes(worker_raw)
            put(host/'receipt.json', receipt)
            yes(r.replay(published) == summary, 'private original evidence restored')
            saved_guard = (host/'stop_and_verify.sh').read_bytes()
            (host/'stop_and_verify.sh').write_bytes(b'fake guard')
            no(lambda: r.collect(original, packaged), 'changed copied guard')
            (host/'stop_and_verify.sh').write_bytes(saved_guard)
            wrong_guard = deepcopy(receipt)
            wrong_guard['commands'][-1]['argv'][0] = '/unrelated/stop_and_verify.sh'
            put(host/'receipt.json', wrong_guard)
            no(lambda: r.collect(original, packaged), 'different guard path')
            put(host/'receipt.json', receipt)
        yes(r.redact(b'2001:db8::1 10.0.0.1 x@example.net ssh-ed25519 AAAABBB label\n').count(b'redacted') == 4,
            'IPv6/IPv4/account/public key redaction')
        no(lambda: r.redact(b'-----BEGIN OPENSSH PRIVATE KEY-----'), 'private key refusal')
        no(lambda: r.stop_certificate(dict(after, lastStartTimestamp='other'), generation), 'stop generation')
        no(lambda: r.stop_certificate(dict(after, status='RUNNING'), generation), 'running state')
        no(lambda: r.stop_certificate(dict(after, lastStopTimestamp='2026-09-26T00:00:00Z'), generation), 'stop chronology')
    print(json.dumps(dict(schema=r.SCHEMA, status='passed', **checks, real_subprocesses=0,
        GCP_used=False, measured_data=False, reused_fixture_positive=fixture_checks.positive,
        reused_fixture_rejected=fixture_checks.rejected), sort_keys=True))


if __name__ == '__main__':
    main()
