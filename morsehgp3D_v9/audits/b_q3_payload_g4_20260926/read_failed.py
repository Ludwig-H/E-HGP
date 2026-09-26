#!/usr/bin/env python3
"""Offline audit of failed capture and two closed recovery attempts; no benchmark claim."""
import argparse
import json
from pathlib import Path
import tarfile

import readback as reader

need, read, sha = reader.need, reader.read, reader.sha


def command_records(directory, receipt, published):
    previous = 0
    for row in receipt['commands']:
        name = row['name']
        need(read(directory / (name + '.command.json')) == row, 'command record differs')
        need(row['group_closed'] is True and previous <= row['started_epoch'] <= row['ended_epoch'],
             'command chronology/closure')
        previous = row['ended_epoch']
        if not published:
            need(all(sha(directory / (name + '.' + suffix)) == row[suffix + '_sha256']
                     for suffix in ('stdout', 'stderr')), 'raw command hash')
    if published:
        reader.published_redactions(directory)


def build(host, prestart, allocated, snapshot, package_path=None, published_root=None, verify_inventory=True):
    published = published_root is not None
    if published and verify_inventory:
        reader.check_inventory(published_root)
    original, first, second = (read(directory / 'receipt.json') for directory in (host, prestart, allocated))
    reader.closed_host(original, True)
    for directory, receipt in ((host, original), (prestart, first), (allocated, second)):
        command_records(directory, receipt, published)
    manifest_path = (published_root or host) / 'source_manifest.json'
    manifest = read(manifest_path)
    need(sha(snapshot) == original['snapshot_sha256'] and sha(manifest_path) == original['manifest_sha256'], 'original payload pins')
    reader.session.validate_protocol_runtime(manifest)
    cases, provenance = reader.session.validate_snapshot(snapshot, manifest)
    need(original['provenance'] == provenance and original['worker_sha256'] == manifest['gcp-migration/tower_worker_v9.py'] and
         original['controller_sha256'] == manifest['gcp-migration/tower_session_v9.py'], 'original source identity')
    if package_path is not None:
        package = read(package_path)
        need(package['commit'] == provenance['commit'] and package['snapshot_sha256'] == original['snapshot_sha256'] and
             package['manifest_sha256'] == original['manifest_sha256'] and package['worker_sha256'] == original['worker_sha256'] and
             package['cases'] == cases, 'package pins')
    need(second.get('schema') == 'mhgp9_capture_recovery_v1' and second.get('status') == 'failed' and
         second.get('benchmark_executed') is False and second.get('targeted_shutdown_certified') is True and
         second.get('target') == reader.worker.TARGET and second.get('original_generation') == original['generation'] and
         second.get('original_host_receipt_sha256') == sha(host / 'receipt.json') and
         second.get('original_worker_receipt_hash_pinned_before_stop') is False, 'allocated recovery identity/status')
    expected = ['original_after_stop', 'oslogin_add', 'guarded_start', 'before_recovery', 'guest_schedule',
                'pack_capture', 'guarded_stop', 'after_stop']
    need([row['name'] for row in second['commands']] == expected and
         [row['exit_code'] for row in second['commands']] == [0, 0, 0, 0, 0, 1, 0, 0], 'allocated recovery stopped after failed pack')
    prestart_result = reader.failed_recovery_evidence(prestart, host, second, published)
    before, after = read(allocated / 'original_after_stop.json'), read(allocated / 'after_stop.json')
    original_cost = reader.duration(before, original['generation'])
    recovery_cost = reader.duration(after, second['generation'])
    observed = reader.session.epoch(second['original_after_stop_observed_at'])
    need(reader.session.epoch(before['lastStopTimestamp']) <= observed < reader.session.epoch(second['generation']) and
         second['commands'][0]['ended_epoch'] <= observed <= second['commands'][1]['started_epoch'],
         'original stop read before recovery restart')
    for name in ('original_after_stop', 'after_stop'):
        pin = second[name + '_sha256']
        if published:
            need(read(allocated / 'PUBLICATION_REDACTIONS.json')[name + '_source_sha256'] == pin, 'state projection pin')
        else:
            need(sha(allocated / (name + '.json')) == pin, 'raw state pin')
    reader.stop_evidence(host, original, published)
    reader.stop_evidence(allocated, second, published)
    mark = second['verified_guard']['mark']
    reader.session.target(mark)
    need(mark['generation'] == second['generation'] and mark['guest_shutdown_minutes'] == '10' and
         mark['max_run_seconds'] == '3600', 'allocated recovery guard identity')
    seconds = original_cost['vm_elapsed_seconds'] + recovery_cost['vm_elapsed_seconds']
    with tarfile.open(snapshot, 'r:*') as archive:
        plan = archive.extractfile(reader.worker.PLAN).read()
    if published:
        need((published_root / 'plan.json').read_bytes() == plan, 'plan differs')
    result = dict(schema='mhgp9_q3_payload_failed_capture_analysis_v1', status='capture_failed', public_status='not_claimed',
                  FULL_qualified=False, GPU_qualified=False, contract_certified=False, global_subquadratic_claim=False,
                  planned_case_count=len(cases), locally_validated_case_count=0, measured_timings=None,
                  source_commit=provenance['commit'], snapshot_sha256=sha(snapshot), manifest_sha256=sha(manifest_path),
                  original=dict(receipt_sha256=sha(host / 'receipt.json'), status=original['status'],
                                GPU_executed=original['GPU_executed'], FULL_executed=original['FULL_executed'],
                                worker_exit_code=original['worker_exit_code'], capture_error=original.get('capture_error'),
                                targeted_shutdown_certified=True),
                  prestart_recovery=prestart_result,
                  allocated_recovery=dict(receipt_sha256=sha(allocated / 'receipt.json'), status=second['status'],
                      error=second.get('error'), benchmark_executed=False, targeted_shutdown_certified=True,
                      diagnosis='Pack command failed with no output; exact failed precondition not established'),
                  cost=dict(sessions=[dict(role='original_benchmark', **original_cost),
                                      dict(role='failed_copy_only_recovery', **recovery_cost)],
                            vm_elapsed_seconds=seconds, vm_elapsed_hours=seconds / 3600, billed_cost_usd=None,
                            reason='Two allocated generations summed; pre-start refusal adds no generation; no billing export'))
    if published and verify_inventory:
        need(read(published_root / 'SUMMARY.json') == result, 'failure summary differs')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--snapshot', required=True, type=Path)
    args = parser.parse_args()
    root = args.receipt
    result = build(root / 'host', root / 'failed_recovery', root / 'recovery_allocated', args.snapshot,
                   root / 'PACKAGE.json', published_root=root)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
