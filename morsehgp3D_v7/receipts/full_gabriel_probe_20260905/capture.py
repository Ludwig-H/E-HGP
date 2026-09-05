#!/usr/bin/env python3
"""One-shot bounded micro admission; no n>=8000 command is constructed."""
import hashlib
import json
import math
import os
from pathlib import Path
import resource
import shlex
import signal
import subprocess
import time

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_full_gabriel_probe_20260905'
RUN = BASE / 'micro'
SOURCE = ROOT / 'morsehgp3D_v7/bench/full_gabriel_probe.cpp'
SOURCE_PIN = 'f3de0d3ca850611f328cb41b251ec66c914afe473eed8e55f89eb889898f1849'
FULL_PIN = 'e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170'
AUTHORITY = 'full_horizontal_relative_to_supplied_complete_exact_regular_gabriel_catalogues'
SCHEMA = 'mhgp7-full-gabriel-probe-v1'
CAPS = {
    'max_raw_candidates': 16000000, 'effective_raw_cap': 16000000,
    'named_payload_budget_bytes': 8 << 30, 'historical_fold_inflight': 2,
    'historical_event_payload_factor': 4, 'max_points_per_order': 32000,
    'max_input_records_per_order': 8000000, 'max_aliases_per_order': 8000000,
    'max_face_visits_per_order': 128000000, 'max_portal_requests_per_order': 8000000,
    'max_chain_steps_per_order': 2000000, 'max_meb_calls_per_order': 4000000,
    'max_query_nodes_per_order': 1000000000, 'max_meb_supports_per_order': 1000000000,
    'max_successor_steps_per_order': 128000000, 'max_certificate_batches_per_order': 4000000,
    'max_certificate_nodes_per_order': 4000000, 'max_certificate_parent_refs_per_order': 8000000,
    'max_read_point_refs_per_order': 40000000,
}


def demand(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def save(path, obj):
    with path.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write(json.dumps(obj, indent=2, sort_keys=True) + '\n')


def snapshot():
    paths = list((ROOT / 'morsehgp3D_v7/src').rglob('*')) + [SOURCE, Path(__file__)]
    return {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sorted(paths) if p.is_file()}


def interrupted(signum, _frame):
    raise InterruptedError(f'signal {signum}')


def drain(process):
    for sig in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(process.pid, sig)
        except ProcessLookupError:
            pass
        if sig == signal.SIGTERM:
            try:
                process.wait(timeout=0.5)
            except subprocess.TimeoutExpired:
                pass
    process.wait()


def child_limit():
    soft, hard = resource.getrlimit(resource.RLIMIT_AS)
    ceiling = 26 << 30
    if soft != resource.RLIM_INFINITY:
        ceiling = min(soft, ceiling)
    if hard != resource.RLIM_INFINITY:
        ceiling = min(hard, ceiling)
    resource.setrlimit(resource.RLIMIT_AS, (ceiling, hard))


def execute(label, argv, expected, records):
    stdout = RUN / (label + '.stdout')
    stderr = RUN / (label + '.stderr')
    record = {'label': label, 'argv': argv, 'cwd': str(ROOT), 'expected_rc': expected,
              'timeout_seconds': 60, 'status': 'running'}
    save(RUN / (label + '.intent.json'), record)
    process = None
    error = None
    begin = time.monotonic()
    try:
        with stdout.open('xb') as out, stderr.open('xb') as err:
            process = subprocess.Popen(argv, cwd=ROOT, stdout=out, stderr=err,
                                       start_new_session=True, preexec_fn=child_limit)
            process.wait(timeout=60)
    except BaseException as exc:
        error = f'{type(exc).__name__}: {exc}'
    finally:
        if process is not None:
            drain(process)
        record.update({'rc': None if process is None else process.returncode,
                       'elapsed_seconds': time.monotonic() - begin,
                       'error': error, 'status': 'completed' if error is None else 'failed'})
        for kind, path in (('stdout', stdout), ('stderr', stderr)):
            data = path.read_bytes() if path.exists() else b''
            record[kind + '_bytes'] = len(data)
            record[kind + '_sha256'] = sha(data)
        save(RUN / (label + '.receipt.json'), record)
        records.append(record)
    demand(error is None and record['rc'] == expected, f'{label}: command failed: {record}')
    return stdout.read_bytes(), stderr.read_bytes()


def unique_keys(pairs):
    out = {}
    for key, value in pairs:
        demand(key not in out, f'duplicate JSON key {key}')
        out[key] = value
    return out


def parse(raw):
    rows = [json.loads(line, object_pairs_hook=unique_keys) for line in raw.decode('utf-8').splitlines()]
    demand(rows and all(row.get('schema') == SCHEMA for row in rows), 'schema/empty')
    demand(sum(row.get('type') == 'terminal' for row in rows) == 1 and rows[-1]['type'] == 'terminal',
           'unique final terminal')
    return rows


def judge_positive(raw, stderr, separation, kmax):
    demand(stderr == b'', 'positive stderr')
    rows = parse(raw)
    effective = min(kmax, 8)
    demand(len(rows) == effective + 2 and rows[0]['type'] == 'configuration', 'row inventory')
    config, terminal = rows[0], rows[-1]
    orders = rows[1:-1]
    demand([row['k'] for row in orders] == list(range(1, effective + 1)), 'order sequence')
    for row in (config, terminal):
        demand(row['n'] == 8 and row['s'] == separation and row['kmax_requested'] == kmax and
               row['kmax_effective'] == effective, 'configuration binding')
        demand(row['authority'] == AUTHORITY and row['public_status'] == 'not_claimed', 'authority')
        demand(row['input_digest_kind'] == row['certificate_digest_kind'] == 'none', 'no digest claim')
        demand(row['s_comparison_scope'] == 'costs_and_volumes_only_not_forest_equality', 'comparison scope')
    demand(config['threads'] == 1 and config['seed'] == 3 and config['coord'] == 65536 and
           config['family'] == 'uniform' and not config['gpu'] and not config['archive'] and not config['vertical'],
           'fixed profile')
    demand(config['catalogue_cardinality_max'] == min(kmax + 1, 8), 'catalogue window')
    demand(all(config[key] == value for key, value in CAPS.items()), 'cap signature')
    demand(0 < config['vm_soft_limit_bytes'] <= 26 << 30, 'VM guard')
    demand(config['candidate_fusion_cap_2e'] == (8 << 30) // (2 * config['sizeof_ball_candidate']), '2E guard')
    previous_direct = 0
    cap_work = {'input_records': 'max_input_records_per_order', 'aliases': 'max_aliases_per_order',
                'face_visits': 'max_face_visits_per_order', 'portal_requests': 'max_portal_requests_per_order',
                'chain_steps': 'max_chain_steps_per_order', 'meb_calls': 'max_meb_calls_per_order',
                'query_nodes': 'max_query_nodes_per_order', 'meb_supports': 'max_meb_supports_per_order',
                'successor_steps': 'max_successor_steps_per_order', 'certificate_nodes': 'max_certificate_nodes_per_order',
                'certificate_parent_refs': 'max_certificate_parent_refs_per_order'}
    for row in orders:
        demand(row['type'] == 'order' and row['provisional'] is True and row['whole_tower_authority'] is False,
               'provisional order')
        demand(row['outcome'] == 'complete_relative' and row['reason'] == AUTHORITY, 'positive order')
        demand(row['minimum_catalogue_records'] == previous_direct, 'catalogue move')
        previous_direct = row['connection_catalogue_records']
        demand(row['input_records'] == row['minimum_catalogue_records'] + previous_direct, 'input count')
        demand(row['terminal_roots'] == 1 and row['terminal_coverage_points'] == 8, 'terminal sentinels')
        demand(0 < row['certificate_minima'] <= row['certificate_nodes'], 'nonempty certificate')
        demand(row['certificate_minima'] == (8 if row['k'] == 1 else row['minimum_catalogue_records']), 'minima count')
        demand(row['certificate_parent_refs'] == row['certificate_nodes'] - 1, 'single tree parent count')
        demand(row['meb_calls'] == row['geometry_meb_calls'], 'MEB count binding')
        demand(all(0 <= row[field] <= config[cap] for field, cap in cap_work.items()), 'work caps')
        for field in ('expand_ms', 'build_ms', 'read_ms', 'release_ms', 'rss_mib_sample', 'hwm_mib_sample'):
            demand(math.isfinite(row[field]) and row[field] >= 0, 'finite costs')
    if kmax == 10:
        demand(orders[-1]['k'] == 8 and previous_direct == 0 and orders[-1]['certificate_minima'] == 1 and
               orders[-1]['certificate_nodes'] == 1 and orders[-1]['certificate_parent_refs'] == 0, 'K=n terminal')
    demand(terminal['terminal_status'] == 'completed' and terminal['outcome'] == 'complete_relative' and
           terminal['exit_code'] == 0 and terminal['complete_requested_horizontal_orders'] is True, 'global closure')
    demand(terminal['completed_orders_diagnostic'] == effective and terminal['last_order'] == effective, 'all orders')
    demand(not terminal['integrated_inter_k_tower'] and not terminal['certificate_retained'] and
           not terminal['terminal_root_coverage_proves_equality'], 'bounded scope')
    demand(terminal['frontier_ledger_closed'] and terminal['rank_window_regular'] and
           terminal['rank_relevant_extra_shell'] == terminal['generation_cap_refus'] == terminal['invariant_jneg'] == 0,
           'frontier completion')
    demand(terminal['pair_mass_expected_per_lane'] == 28, 'pair domain')
    for q in (2, 3, 4):
        demand(terminal[f'ledger_q{q}_emitted'] + terminal[f'ledger_q{q}_killed'] == 28, 'pair closure')
    demand(all(0 <= value <= 1 for key, value in terminal.items() if key.startswith('workers_')), 'mono')
    demand(terminal['raw_candidates'] >= terminal['unique_candidates'] >= terminal['census_balls'] > 0, 'frontier counts')
    demand(sum(row['connection_catalogue_records'] for row in orders) == terminal['census_balls'], 'all catalogue ranks')
    demand(terminal['reference_timing'] == 'elapsed_before_terminal_ms_includes_provisional_output' and
           terminal['subtracted_timing_scope'] == 'diagnostic_only_not_an_independent_timer', 'timing scope')
    return {'s': separation, 'kmax_requested': kmax, 'orders': effective,
            'raw_candidates': terminal['raw_candidates'], 'census_balls': terminal['census_balls'],
            'nodes_by_k': [row['certificate_nodes'] for row in orders],
            'elapsed_before_terminal_ms': terminal['elapsed_before_terminal_ms']}


def main():
    RUN.mkdir(exist_ok=False)
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    before = snapshot()
    save(RUN / 'sources_before.json', before)
    records, summaries = [], []
    receipt = {'status': 'failed', 'reason': 'not_started', 'commands': records, 'micro_summaries': summaries}
    try:
        demand(sha(SOURCE.read_bytes()) == SOURCE_PIN, 'probe pin')
        demand(before['morsehgp3D_v7/src/forest/full_gabriel.hpp'] == FULL_PIN, 'FULL pin')
        binary = RUN / 'full_gabriel_probe'
        depfile = RUN / 'full_gabriel_probe.d'
        compile_argv = ['/usr/bin/taskset', '-c', '0', '/usr/bin/g++', '-std=c++20', '-O3', '-DNDEBUG',
                        '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread', '-MMD', '-MF', str(depfile),
                        str(SOURCE), '-o', str(binary)]
        execute('compile', compile_argv, 0, records)
        deps_text = depfile.read_text().replace('\\\n', ' ')
        deps = shlex.split(deps_text.split(':', 1)[1])
        demand(deps and str(SOURCE) in deps, 'source dependency absent')
        dependency_pins = {}
        for entry in deps:
            path = (ROOT / entry).resolve()
            name = str(path.relative_to(ROOT))
            demand(name in before and sha(path.read_bytes()) == before[name], f'dependency binding {name}')
            dependency_pins[name] = before[name]
        save(RUN / 'dependencies.json', dependency_pins)
        receipt['binary_sha256'] = sha(binary.read_bytes())
        for separation in (8, 10, 12):
            for kmax in (5, 10):
                argv = ['/usr/bin/taskset', '-c', '6', str(binary), '--n=8', f'--s={separation}', f'--kmax={kmax}']
                raw, err = execute(f'n8_s{separation}_k{kmax}', argv, 0, records)
                summaries.append(judge_positive(raw, err, separation, kmax))
        negatives = {
            'missing': ['--n=8', '--s=8'],
            'duplicate': ['--n=8', '--n=8', '--s=8', '--kmax=10'],
            'n9': ['--n=9', '--s=8', '--kmax=10'],
            's9': ['--n=8', '--s=9', '--kmax=10'],
            'k0': ['--n=8', '--s=8', '--kmax=0'],
            'unknown': ['--n=8', '--s=8', '--kmax=10', '--unknown'],
        }
        for label, options in negatives.items():
            raw, err = execute('reject_' + label, ['/usr/bin/taskset', '-c', '6', str(binary)] + options, 2, records)
            rows = parse(raw)
            demand(err == b'' and len(rows) == 1 and rows[0]['outcome'] == 'invalid_input' and
                   rows[0]['reason'] == 'probe_arguments' and rows[0]['exit_code'] == 2 and
                   rows[0]['terminal_status'] == 'failed' and rows[0]['completed_orders_diagnostic'] == 0 and
                   not rows[0]['complete_requested_horizontal_orders'], 'negative parser judge')
        demand(sha(binary.read_bytes()) == receipt['binary_sha256'], 'binary drift')
        demand(len(records) == 13 and len(summaries) == 6, 'non-vacuity')
        receipt.update(status='completed', reason='six_n8_and_six_parser_rejects')
    except BaseException as exc:
        receipt.update(status='failed', reason=f'{type(exc).__name__}: {exc}')
    finally:
        after = snapshot()
        save(RUN / 'sources_after.json', after)
        receipt['sources_stable'] = before == after
        if before != after:
            receipt.update(status='failed', reason='source_drift')
        save(RUN / 'receipt.json', receipt)
    print(json.dumps(receipt, sort_keys=True))
    return 0 if receipt['status'] == 'completed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
