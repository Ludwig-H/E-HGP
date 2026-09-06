#!/usr/bin/env python3
"""Offline verification of this closed, negative G4 capture. No subprocess/GCP."""
import hashlib
import json
from pathlib import Path
import re
import sys

TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b', instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
GENERATION = '2026-09-06T06:19:11.593-07:00'
PINS = {'host/receipt.json': '6f02b9ff6e0e99a098a07c5a0555a6bf92c49179b3e877b5bdc7ce9b21aed3bd',
        'guest/receipt.json': '10ab69dd048e257de39a8c6824f2eda87da0d48638b2d3ebf0bd5c5b2b0562bb',
        'closure_readonly.json': 'a969b2e996543959d23f3422a296ee866ca922224246f9f82ba490c2e1896bce',
        'launch_context.json': '9660894f1f3602e7f3f60a410ad8933b3664c82b6352c0f65d754b1377f22200',
        'source_manifest.json': '79ffb8d63904f900ed13e40cc951ca3a75f37cb4b98000e9ccb70cd21867eac6'}


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def read(path):
    return json.loads(path.read_text(), object_pairs_hook=unique)


def fields(path):
    return unique(line.split('=', 1) for line in path.read_text().splitlines())


def main(packet):
    publication = read(packet / 'publication.json')
    for name, pin in {**publication['files'], **PINS}.items():
        need(sha(packet / name) == pin, 'capture hash: ' + name)
    if (packet / 'SHA256SUMS').exists():
        for line in (packet / 'SHA256SUMS').read_text().splitlines():
            pin, name = line.split('  ', 1)
            need(not name.startswith(('/', './')) and '..' not in Path(name).parts and sha(packet / name) == pin, 'manifest')
    manifest = read(packet / 'source_manifest.json')
    need(len(manifest) == 42 and set(publication['source_references']) == set(manifest), '42 source references')
    for name, ref in publication['source_references'].items():
        need(ref['sha256'] == manifest[name] and sha(packet / ref['relative_path']) == manifest[name], 'sealed source: ' + name)
    guest, host = read(packet / 'guest/receipt.json'), read(packet / 'host/receipt.json')
    need(guest['target'] == host['target'] == TARGET and guest['generation'] == host['generation'] == GENERATION, 'target/generation')
    need(guest['status'] == 'failed' and host['status'] == 'worker_failed' and host['worker_exit_code'] == 1 and
         guest['contract_certified'] is False and host['targeted_shutdown_certified'] is True, 'scope and closure')
    need(guest['available_cpus'] == list(range(48)) and guest['sources_stable'] is True and
         guest['binary_sha256'] == guest['binary_sha256_after'], 'CPU/source/binary stability')
    need(guest['worker_sha256'] == host['worker_sha256'] == publication['scripts']['worker']['sha256'] and
         host['controller_sha256'] == publication['scripts']['controller']['sha256'], 'recorded script pins')
    for name in ('sources_before.json', 'sources_after.json', 'compiled_dependencies.json'):
        need(read(packet / 'guest' / name) == manifest, 'compiled/source binding')
    need(sha(packet / 'guest/full_probe.d') == guest['depfile_sha256'], 'depfile')
    command_names = ('compiler_version', 'time_version', 'compile', 'smoke_n8_k10', 'n50000_k10', 'n50000_k5')
    need(len(guest['commands']) == 6, 'six guest commands')
    for name, row, expected in zip(command_names, guest['commands'], (0, 0, 0, 0, 2, 2)):
        need(read(packet / 'guest' / (name + '.command.json')) == row and row['exit_code'] == expected and
             row['group_closed'] is True and row['per_command_watchdog'] is None and type(row['pid']) is int, 'guest command')
        for stream in ('stdout', 'stderr'):
            need(sha(packet / 'guest' / (name + '.' + stream)) == row[stream + '_sha256'], 'guest stream')
    host_commands = {row['name']: row for row in host['commands']}
    need(len(host_commands) == len(host['commands']) == 13, 'host command sequence')
    for name, row in host_commands.items():
        need(row['exit_code'] == (1 if name == 'worker' else 0) and row['group_closed'] is True, 'host closure')
        for stream in ('stdout', 'stderr'):
            path = packet / 'host' / (name + '.' + stream)
            if path.exists():
                need(sha(path) == row[stream + '_sha256'], 'selected host stream')
    stop = host_commands['guarded_stop']
    need(stop['argv'][-2:] == ['--expected-last-start-timestamp', GENERATION], 'versioned stop')
    stop_text = (packet / 'host/guarded_stop.stdout').read_text()
    need('[OK] Cible ' + TARGET['instance'] + ' arrêtée et vérifiée (état GCE TERMINATED).' in stop_text and
         'Aucune autre VM project=e-hgp active détectée.' in stop_text, 'stop certificate')
    closure = read(packet / 'closure_readonly.json')
    observed = json.loads(closure['stdout'], object_pairs_hook=unique)
    need(closure['exit_code'] == 0 and observed['status'] == 'TERMINATED' and observed['name'] == TARGET['instance'] and
         observed['lastStartTimestamp'] == GENERATION and observed['labels']['project'] == 'e-hgp' and
         observed['scheduling']['provisioningModel'] == 'SPOT' and observed['scheduling']['instanceTerminationAction'] == 'STOP', 'filtered final read')
    handoff = read(packet / 'host/handoff.json')
    lifecycle = fields(packet / 'host/lifecycle.txt')
    mark = fields(packet / 'host/guardmarks/double_guard_verified')
    need(handoff['last_start_timestamp'] == lifecycle['generation'] == mark['generation'] == GENERATION and
         mark['mark'] == 'double_guard_verified' and mark['guest_shutdown_minutes'] == '30', 'guard evidence')
    need(sha(packet / 'host/guardmarks/double_guard_verified') == guest['guard_mark_sha256'], 'mark pin')
    trials = []
    for name, n, kmax, extra in (('smoke_n8_k10', 8, 10, None), ('n50000_k10', 50000, 10, 4), ('n50000_k5', 50000, 5, 3)):
        rows = [json.loads(line, object_pairs_hook=unique) for line in (packet / 'guest' / (name + '.stdout')).read_text().splitlines()]
        expected_types = ['configuration'] + (['order'] * 8 if extra is None else []) + ['terminal']
        need([row['type'] for row in rows] == expected_types, 'exact FULL row sequence')
        config, terminal = rows[0], rows[-1]
        need(all(row['schema'] == 'mhgp7-full-gabriel-probe-v6' and row['pipeline_threads'] == 48 and
                 row['full_order_builder_threads'] == 1 and row['census_payload_accounting'] ==
                 'preflight_survivor_then_direct_census_v2' for row in rows), 'probe profile')
        need(config['n'] == terminal['n'] == n and config['s'] == terminal['s'] == 8 and
             config['kmax_requested'] == terminal['kmax_requested'] == kmax, 'requested scope')
        if extra is None:
            need(terminal['exit_code'] == 0 and terminal['outcome'] == 'complete_relative' and
                 terminal['complete_requested_horizontal_orders'] is True and
                 [row['k'] for row in rows[1:-1]] == list(range(1, 9)) and
                 all(row['outcome'] == 'complete_relative' for row in rows[1:-1]), 'smoke complete')
        else:
            need(type(terminal['rank_relevant_extra_shell']) is int and terminal['rank_relevant_extra_shell'] == extra and
                 terminal['exit_code'] == 2 and terminal['terminal_status'] == 'failed' and
                 terminal['outcome'] == 'unsupported_degeneracy' and terminal['reason'] == 'probe_rank_relevant_extra_shell' and
                 terminal['last_stage'] == 'regularity' and terminal['completed_orders_diagnostic'] == terminal['last_order'] == 0 and
                 terminal['rank_window_regular'] is False and terminal['complete_requested_horizontal_orders'] is False and
                 terminal['stage_ms']['full'] == 0 and terminal['certificate_digest'] == '', 'refusal before FULL')
            need(re.search(r'^\s*Exit status: 2$', (packet / 'guest' / (name + '.stderr')).read_text(), re.M), 'GNU time exit')
            trials.append(dict(kmax=kmax, exit_code=2, FULL_orders=0, extra_shell=extra, reason=terminal['reason']))
    need(guest['fallback_executed'] is True and guest['k10']['reported_complete'] is False and
         guest['k5']['reported_complete'] is False, 'two actual refused processes')
    print(json.dumps(dict(status='verified_closed_capture', smoke_orders=8, trials=trials,
          targeted_shutdown='TERMINATED', generation=GENERATION, global_FULL_successful=False,
          performance_contract_certified=False, public_status='not_claimed'), sort_keys=True))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit(2)
    main(Path(sys.argv[1]).resolve())
