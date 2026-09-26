#!/usr/bin/env python3
"""Read-only replay of a closed FULL q3-payload G4 capture. No cloud calls."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
from statistics import median
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import tower_session_v9 as session
import tower_worker_v9 as worker

need, sha = worker.need, worker.sha
read = lambda path: worker.strict_json(path.read_bytes())


def statistics(values):
    return dict(count=len(values), minimum=min(values), median=median(values), maximum=max(values)) if values else None


def inventory(folder):
    result = {}
    for path in sorted(folder.rglob('*')):
        need(not path.is_symlink(), 'symlink in publication')
        if path.is_file() and path != folder / 'SHA256SUMS':
            result[str(path.relative_to(folder))] = sha(path)
    return result


def check_inventory(folder):
    path = folder / 'SHA256SUMS'
    need(path.is_file(), 'published inventory absent')
    rows = {}
    for line in path.read_text().splitlines():
        pin, name = line.split('  ', 1)
        need(name not in rows, 'duplicate inventory path')
        rows[name] = pin
    need(rows == inventory(folder), 'published inventory differs')


def duration(after, generation):
    session.validate_target(after, 'TERMINATED', generation)
    need(type(after.get('lastStopTimestamp')) is str, 'missing stop timestamp')
    start = datetime.fromisoformat(generation.replace('Z', '+00:00'))
    stop = datetime.fromisoformat(after['lastStopTimestamp'].replace('Z', '+00:00'))
    need(start.tzinfo is not None and stop.tzinfo is not None and stop >= start, 'stop chronology')
    seconds = (stop - start).total_seconds()
    return dict(generation=generation, stopped=after['lastStopTimestamp'], vm_elapsed_seconds=seconds,
                vm_elapsed_hours=seconds / 3600, provisioning='SPOT', machine='g4-standard-48',
                billed_cost_usd=None, reason='No billing export or verified hourly rate; elapsed allocation is not an invoice')


def closed_host(receipt, recovering=False):
    expected = 'capture_failed' if recovering else 'completed'
    need(receipt.get('status') == expected and receipt.get('targeted_shutdown_certified') is True,
         'closed ' + expected + ' host receipt required')
    if recovering:
        need(receipt.get('GPU_executed') is False and receipt.get('FULL_executed') is False and
             receipt.get('worker_exit_code') == 0, 'original failed capture scope must remain unchanged')
    else:
        need(receipt.get('GPU_executed') is True and receipt.get('FULL_executed') is True, 'FULL GPU host scope')


def stop_evidence(directory, receipt, published):
    stops = [row for row in receipt['commands'] if row.get('name') == 'guarded_stop']
    need(len(stops) == 1 and stops[0].get('exit_code') == 0 and stops[0].get('group_closed') is True and
         stops[0]['argv'][-2:] == ['--expected-last-start-timestamp', receipt['generation']], 'targeted stop invocation')
    if not published:
        need(all(sha(directory / ('guarded_stop.' + name)) == stops[0][name + '_sha256'] for name in ('stdout', 'stderr')),
             'raw stop hashes')
    else:
        redactions = read(directory / 'PUBLICATION_REDACTIONS.json')['redactions']
        for suffix in ('stdout', 'stderr'):
            matching = [row for row in redactions if row.get('source_name') == 'guarded_stop.' + suffix]
            need(len(matching) == 1 and matching[0].get('source_sha256') == stops[0][suffix + '_sha256'] and
                 matching[0].get('published_name') == 'guarded_stop.redacted.' + suffix and
                 matching[0].get('published_sha256') == sha(directory / ('guarded_stop.redacted.' + suffix)),
                 'published targeted stop hash/redaction binding')


def published_redactions(directory):
    receipt = read(directory / 'receipt.json')
    commands = {row['name']: row for row in receipt['commands']}
    seen = set()
    for row in read(directory / 'PUBLICATION_REDACTIONS.json')['redactions']:
        name = row['source_name']
        need(name not in seen and name.count('.') == 1, 'duplicate/invalid redaction name')
        seen.add(name)
        stem, suffix = name.split('.')
        need(stem in commands and suffix in ('stdout', 'stderr') and
             row['source_sha256'] == commands[stem][suffix + '_sha256'] and
             row['published_name'] == stem + '.redacted.' + suffix and
             row['published_sha256'] == sha(directory / row['published_name']), 'redacted log command binding')


def recovery_evidence(directory, original_host, vm, published):
    """Recovery copies an older worker run; it cannot promote its host status."""
    recovery = read(directory / 'receipt.json')
    original = read(original_host / 'receipt.json')
    need(recovery.get('schema') == 'mhgp9_capture_recovery_v1' and recovery.get('status') == 'recovered_capture' and
         recovery.get('benchmark_executed') is False and recovery.get('targeted_shutdown_certified') is True and
         recovery.get('target') == worker.TARGET and recovery.get('replayed_original_status') == 'completed',
         'closed copy-only recovery required')
    need(recovery.get('original_generation') == original['generation'] and
         recovery.get('original_host_receipt_sha256') == sha(original_host / 'receipt.json') and
         recovery.get('original_worker_receipt_sha256') == sha(vm / 'receipt.json'), 'recovery original identity')
    need(recovery.get('generation') != original['generation'], 'recovery must have a separate generation')
    before, after = read(directory / 'original_after_stop.json'), read(directory / 'after_stop.json')
    original_cost, recovery_cost = duration(before, original['generation']), duration(after, recovery['generation'])
    observed = session.epoch(recovery['original_after_stop_observed_at'])
    need(session.epoch(before['lastStopTimestamp']) <= observed < session.epoch(recovery['generation']),
         'original termination must be observed before recovery start')
    redactions = read(directory / 'PUBLICATION_REDACTIONS.json') if published else None
    for name in ('original_after_stop', 'after_stop'):
        pin = recovery.get(name + '_sha256')
        need(type(pin) is str and len(pin) == 64, 'recovery state pin absent')
        if published:
            need(redactions.get(name + '_source_sha256') == pin, 'recovery state projection binding')
        else:
            need(sha(directory / (name + '.json')) == pin, 'recovery raw state hash')
    guard = recovery['verified_guard']; mark, schedule = guard['mark'], guard['schedule']
    session.target(mark)
    need(mark.get('schema') == 'e-hgp.guard-mark.v1' and mark.get('mark') == 'double_guard_verified' and
         mark.get('generation') == recovery['generation'] and mark.get('guest_shutdown_minutes') == '10' and
         mark.get('max_run_seconds') == '3600', 'recovery double guard identity/durations')
    marked = session.epoch(mark['date_utc'])
    need(session.epoch(recovery['generation']) <= marked <= session.epoch(after['lastStopTimestamp']),
         'recovery guard chronology')
    need(schedule.get('MODE') == 'poweroff' and str(schedule.get('USEC', '')).isdigit(), 'recovery guest shutdown')
    deadline = int(schedule['USEC']) / 1000000
    need(marked < deadline <= session.epoch(recovery['generation']) + 3300, 'recovery guest deadline')
    stop_evidence(directory, recovery, published)
    seconds = original_cost['vm_elapsed_seconds'] + recovery_cost['vm_elapsed_seconds']
    cost = dict(sessions=[dict(role='original_benchmark', **original_cost), dict(role='copy_only_recovery', **recovery_cost)],
                vm_elapsed_seconds=seconds, vm_elapsed_hours=seconds / 3600, billed_cost_usd=None,
                reason='Two distinct guarded allocations summed; no billing export or verified hourly rate')
    return recovery, cost


def recovery_commands(directory, recovery, published):
    expected = ['original_after_stop', 'oslogin_add', 'guarded_start', 'before_recovery', 'guest_schedule',
                'pack_capture', 'before_download', 'download', 'after_download', 'guarded_stop', 'after_stop']
    rows = recovery['commands']
    need([row.get('name') for row in rows] == expected, 'recovery command scope: copy only, no benchmark')
    previous = 0
    for row in rows:
        name = row['name']
        need(row.get('exit_code') == 0 and row.get('group_closed') is True and
             previous <= row['started_epoch'] <= row['ended_epoch'], 'recovery command closure/chronology')
        previous = row['ended_epoch']
        need(read(directory / (name + '.command.json')) == row, 'recovery command record differs')
        if not published:
            need(all(sha(directory / (name + '.' + suffix)) == row[suffix + '_sha256'] for suffix in ('stdout', 'stderr')),
                 'recovery command raw hashes')
    observed = session.epoch(recovery['original_after_stop_observed_at'])
    need(rows[0]['ended_epoch'] <= observed <= rows[1]['started_epoch'] and
         rows[2]['started_epoch'] <= session.epoch(recovery['generation']) <= rows[2]['ended_epoch'],
         'recovery live observation/start chronology')
    need('--guest-shutdown-minutes' in rows[2]['argv'] and
         rows[2]['argv'][rows[2]['argv'].index('--guest-shutdown-minutes') + 1] == '10', 'recovery guest guard invocation')
    if not published:
        need(sha(directory / 'capture.tar.gz') == recovery['capture_sha256'], 'recovery archive hash')
    # The original worker receipt was not independently pinned before stop.
    # It is a post-retrieval pin, followed by the complete original protocol replay.
    need(recovery.get('original_worker_receipt_hash_pinned_before_stop') is False, 'do not invent an original pre-stop receipt pin')


def failed_recovery_evidence(directory, original_host, recovery, published):
    failed = read(directory / 'receipt.json')
    original = read(original_host / 'receipt.json')
    need(failed.get('schema') == 'mhgp9_capture_recovery_v1' and failed.get('status') == 'failed' and
         failed.get('benchmark_executed') is False and failed.get('no_start_lifecycle_created') is True and
         failed.get('targeted_shutdown_certified') is False and 'generation' not in failed and
         failed.get('target') == worker.TARGET and failed.get('original_generation') == original['generation'] and
         failed.get('original_host_receipt_sha256') == sha(original_host / 'receipt.json'), 'original pre-start failure identity')
    need(not (directory / 'handoff.json').exists() and not (directory / 'lifecycle.txt').exists(),
         'pre-start failure must not conceal a new lifecycle')
    rows = failed['commands']
    need([row.get('name') for row in rows] == ['original_after_stop', 'oslogin_add', 'guarded_start'] and
         [row.get('exit_code') for row in rows] == [0, 0, 1] and all(row.get('group_closed') is True for row in rows),
         'pre-start failure command scope')
    for row in rows:
        need(read(directory / (row['name'] + '.command.json')) == row, 'failed recovery command record differs')
        if not published:
            need(all(sha(directory / (row['name'] + '.' + suffix)) == row[suffix + '_sha256']
                     for suffix in ('stdout', 'stderr')), 'failed recovery raw hashes')
    need(rows[-1]['ended_epoch'] < session.epoch(recovery['original_after_stop_observed_at']),
         'failed recovery must precede successful live original-stop observation')
    before = read(directory / 'original_after_stop.json')
    session.validate_target(before, 'TERMINATED', original['generation'])
    if published:
        need(read(directory / 'PUBLICATION_REDACTIONS.json')['original_after_stop_source_sha256'] ==
             failed['original_after_stop_sha256'], 'failed recovery state projection binding')
    else:
        need(sha(directory / 'original_after_stop.json') == failed['original_after_stop_sha256'], 'failed recovery state pin')
    return dict(receipt_sha256=sha(directory / 'receipt.json'), status=failed['status'], error=failed.get('error'),
                benchmark_executed=False, no_new_generation=True, vm_elapsed_seconds=0,
                reason='Refused before lifecycle creation; subsequent live GCE read still has the original stopped generation')


def analyze(cases, outcomes, probes, commands):
    """Derive pair comparisons from raw bodies, never worker summaries."""
    rows, groups = [], {}
    for index, (case, outcome) in enumerate(zip(cases, outcomes)):
        if outcome['outcome'] != 'complete_relative':
            rows.append(dict(index=index, case=case, outcome=outcome['outcome']))
            continue
        value = probes[index]
        need(worker.validate_probe(value, case, commands[index]['exit_code']) == 'complete_relative', 'raw probe validation')
        frame_times = {name: statistics(value['frames'][name]) for name in worker.FRAME_LISTS}
        warm = {name: statistics(value['frames'][name][1:]) for name in worker.FRAME_LISTS}
        row = dict(index=index, case=case, outcome='complete_relative', input=value['input'],
                   tower_digest=value['tower_digest'], catalogue_digest=value['catalogue_digest'],
                   presentation_digest=value['presentation_digest'], catalogue=value['catalogue'],
                   generator=value['generator'], ledger=value['ledger'], times_ms=value['times_ms'],
                   q34_batch=value['q34_batch'], orders=value['orders'], tower_phases_ms=value['tower_phases_ms'],
                   process_elapsed_seconds=commands[index]['elapsed_seconds'],
                   frame_timings_ms=frame_times, warm_frame_timings_ms=warm,
                   under_100ms_all_frames=all(t < 100 for t in value['frames']['chain_total_ms']),
                   under_1s_all_frames=all(t < 1000 for t in value['frames']['chain_total_ms']))
        rows.append(row)
        groups.setdefault(worker.payload_case_identity(case), []).append(index)
    comparisons = []
    for indices in groups.values():
        on = [i for i in indices if cases[i]['levers']['q3_interior_payload']]
        off = [i for i in indices if not cases[i]['levers']['q3_interior_payload']]
        if not on:
            continue
        need(off, 'payload ON without completed identical OFF')
        matched = []
        for i in on:
            candidates = [j for j in off if cases[j]['repeat'] == cases[i]['repeat']]
            need(len(candidates) == 1, 'one OFF required per ON repeat')
            j = candidates[0]
            need(worker.payload_pair_equal(probes[i], probes[j]), 'paired object/generator/census differs')
            matched.append(dict(on=i, off=j, repeat=cases[i]['repeat'],
                                chain_delta_ms=probes[i]['times_ms']['chain_total'] - probes[j]['times_ms']['chain_total'],
                                census_delta_ms=probes[i]['times_ms']['census'] - probes[j]['times_ms']['census']))
        # Internal frames are not independent process repetitions. Report
        # process medians, and separately drop frame0 only where it exists.
        by_arm = {}
        for label, members in [('on', on), ('off', off)]:
            by_arm[label] = dict(processes=len(members),
                first_frame_ms={name: statistics([probes[i]['frames'][name][0] for i in members]) for name in worker.FRAME_LISTS},
                process_median_ms={name: statistics([median(probes[i]['frames'][name]) for i in members]) for name in worker.FRAME_LISTS},
                warm_process_median_ms={name: statistics([median(probes[i]['frames'][name][1:]) for i in members
                                                         if len(probes[i]['frames'][name]) > 1]) for name in worker.FRAME_LISTS})
        first = cases[on[0]]
        comparisons.append(dict(scene=first['scene'], K=first['k'], s=first['s'], workers=first['workers'],
                                frames=first['frames'], paired_processes=matched, timings=by_arm,
                                all_objects_and_producer_work_equal=True,
                                census_work=dict(on={k: probes[on[0]]['catalogue'][k] for k in
                                    ('census_nodes', 'census_leaf_tests', 'payload_keys', 'payload_ids', 'payload_fallback_keys')},
                                    off={k: probes[off[0]]['catalogue'][k] for k in ('census_nodes', 'census_leaf_tests')})))
    need(comparisons, 'no completed payload pair')
    return rows, comparisons


def build(folder, snapshot, after_stop=None, verify_inventory=True, recovery_dir=None, failed_recovery_dir=None):
    published = (folder / 'host/receipt.json').is_file()
    host = folder / 'host' if published else folder
    if published:
        need(recovery_dir is None and failed_recovery_dir is None,
             'published recovery is selected from its inventory, not an external override')
        recovery_dir = folder / 'recovery' if (folder / 'recovery').exists() else None
        failed_recovery_dir = folder / 'failed_recovery' if (folder / 'failed_recovery').exists() else None
    recovering = recovery_dir is not None
    vm = folder / 'vm' if published else (recovery_dir or host) / 'received/output'
    if published and verify_inventory:
        check_inventory(folder)
    if published:
        for directory in (host, recovery_dir, failed_recovery_dir):
            if directory:
                published_redactions(directory)
    receipt = read(host / 'receipt.json')
    closed_host(receipt, recovering)
    manifest_path = folder / 'source_manifest.json' if published else host / 'source_manifest.json'
    manifest = read(manifest_path)
    need(sha(snapshot) == receipt['snapshot_sha256'] and sha(manifest_path) == receipt['manifest_sha256'], 'snapshot/manifest pin')
    session.validate_protocol_runtime(manifest)
    cases, provenance = session.validate_snapshot(snapshot, manifest)
    need(receipt.get('provenance') == provenance and receipt.get('target') == worker.TARGET and
         receipt.get('worker_sha256') == manifest['gcp-migration/tower_worker_v9.py'] and
         receipt.get('controller_sha256') == manifest['gcp-migration/tower_session_v9.py'], 'host provenance/protocol pins')
    need(session.validate_received(vm, manifest, receipt['worker_sha256'], cases, receipt['generation'],
                                   provenance, receipt['verified_guard']) == 'completed', 'raw protocol replay')
    worker_receipt = read(vm / 'receipt.json')
    stop_evidence(host, receipt, published)
    recovery = None
    if recovering:
        recovery, cost = recovery_evidence(recovery_dir, host, vm, published)
        recovery_commands(recovery_dir, recovery, published)
        if after_stop:
            need(dict(role='original_benchmark', **duration(read(after_stop), receipt['generation'])) == cost['sessions'][0],
                 'external original final state differs')
    else:
        after_path = after_stop or host / 'after_stop.json'
        cost = duration(read(after_path), receipt['generation'])
    failure = None
    if failed_recovery_dir:
        need(recovering, 'failed recovery evidence requires successful separate recovery')
        failure = failed_recovery_evidence(failed_recovery_dir, host, recovery, published)
    probes, commands = {}, {}
    for i, entry in enumerate(worker_receipt['case_outcomes']):
        if entry['outcome'] == 'complete_relative':
            probes[i] = read(vm / ('probe_%d.stdout' % i))
            commands[i] = read(vm / ('probe_%d.command.json' % i))
    rows, pairs = analyze(cases, worker_receipt['case_outcomes'], probes, commands)
    with tarfile.open(snapshot, 'r:*') as archive:
        plan_raw = archive.extractfile(worker.PLAN).read()
    if published:
        need((folder / 'plan.json').read_bytes() == plan_raw, 'published plan differs')
        package = read(folder / 'PACKAGE.json')
        need(package['snapshot_sha256'] == receipt['snapshot_sha256'] and package['manifest_sha256'] == receipt['manifest_sha256'] and
             package['worker_sha256'] == receipt['worker_sha256'] and package['commit'] == provenance['commit'] and
             package['cases'] == cases, 'published package pins')
    result = dict(schema='mhgp9_q3_payload_g4_analysis_v2', public_status='not_claimed',
                  capture_status='recovered_capture' if recovering else 'completed', original_host_status=receipt['status'],
                  original_host_GPU_executed=receipt['GPU_executed'], original_host_FULL_executed=receipt['FULL_executed'],
                  recovery_receipt_sha256=sha(recovery_dir / 'receipt.json') if recovering else None,
                  recovery_benchmark_executed=False if recovering else None,
                  original_worker_receipt_hash_pinned_before_stop=False if recovering else None,
                  original_capture_error=receipt.get('capture_error'),
                  prior_failed_recovery=failure,
                  contract_certified=False, global_subquadratic_claim=False, FULL_executed=True, GPU_executed=True,
                  timing_scope='Prepared in-memory cloud to explicit FULL tower; file read and verification digests separate',
                  frame_scope='Repeated identical cloud within a process is not a distinct scene or independent repetition',
                  protocol_replayed=True, targeted_shutdown_certified=True, cost=cost,
                  source_commit=provenance['commit'], snapshot_sha256=sha(snapshot), manifest_sha256=sha(manifest_path),
                  plan_sha256=hashlib.sha256(plan_raw).hexdigest(), host_receipt_sha256=sha(host / 'receipt.json'),
                  worker_receipt_sha256=sha(vm / 'receipt.json'), case_count=len(cases), rows=rows, paired_comparisons=pairs)
    if published and verify_inventory:
        need(read(folder / 'SUMMARY.json') == result, 'stored summary differs from independent recomputation')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--after-stop', type=Path)
    parser.add_argument('--recovery-dir', type=Path)
    parser.add_argument('--failed-recovery-dir', type=Path)
    args = parser.parse_args()
    result = build(args.receipt, args.snapshot, args.after_stop, recovery_dir=args.recovery_dir,
                   failed_recovery_dir=args.failed_recovery_dir)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
