#!/usr/bin/env python3
"""Read-only v2 receipt judge; not a geometry/completeness oracle.

Explicit port of frozen v1 judge:
24e789459ee7adb8b48819dddc8bef8832b2b152ad9418c1a1d281038315e2c7.
Usage: full_gabriel_lazy_probe_audit.py [--selftest] PATH.receipt.json
Code 0 means a coherent receipt, possibly a refusal, not a successful probe.
Missing terminals/censored captures cannot pass. No engine is invoked.
Forest bytes are not retained: this checks reported per-order digest formats
and recomputes their global binding, not their geometric truth.
"""
import copy
import hashlib
import json
import math
import re
import shlex
import sys
from collections.abc import Iterable, Sequence
from pathlib import Path
from typing import Any

JsonObject = dict[str, Any]
AUTH = (
    'full_horizontal_relative_to_supplied_complete_exact_regular_'
    'gabriel_catalogues'
)
SCHEMA = 'mhgp7-full-gabriel-probe-v2'
INPUT_KIND = 'sha256_FULLv1_labelled_u16_input'
DIGEST_KIND = 'sha256_FULLv1_semantic_labelled_horizontal_forest'
COMPARISON = (
    'semantic_labelled_horizontal_forests_not_catalogue_completeness'
)
POLICIES = {
    'eager': 'eager_all_incident_facets_v1',
    'lazy': 'lazy_first_c_strict_resolutions_v1',
}
WORK = {
    'input_records': 8000000,
    'aliases': 8000000,
    'face_visits': 128000000,
    'portal_requests': 8000000,
    'chain_steps': 2000000,
    'meb_calls': 4000000,
    'query_nodes': 1000000000,
    'meb_supports': 1000000000,
    'successor_steps': 128000000,
    'certificate_nodes': 4000000,
    'certificate_parent_refs': 8000000,
}
FIXED = {
    'max_raw_candidates': 16000000,
    'effective_raw_cap': 16000000,
    'named_payload_budget_bytes': 8 << 30,
    'historical_fold_inflight': 2,
    'historical_event_payload_factor': 4,
    'max_points_per_order': 32000,
    'max_certificate_batches_per_order': 4000000,
    'max_read_point_refs_per_order': 40000000,
}
FIXED.update({'max_' + name + '_per_order': cap for name, cap in WORK.items()})
STAGES = (
    'input index generation rle prefilter census regularity count expand '
    'full read digest release'
).split()
WORKERS = {
    'workers_wspd', 'workers_rects', 'workers_sort',
    'workers_prefilter', 'workers_census', 'workers_expand',
}
WORK_FIELDS = (
    'input_records aliases face_visits alias_hits portal_requests chain_steps '
    'terminal_direct max_chain_length normalized_anchors successor_steps '
    'no_op_connections meb_calls geometry_meb_calls meb_supports query_nodes '
    'query_leaves query_range_skips minimum_lookups minimum_hits '
    'cache_lookups cache_hits cache_inserts cache_skips '
    'singleton_intruder_resolutions direct_lookups'
).split()


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def unique(pairs: Iterable[Sequence[Any]]) -> JsonObject:
    out: JsonObject = {}
    for key, value in pairs:
        require(key not in out, 'duplicate_key')
        out[key] = value
    return out


def loads(text: str) -> Any:
    return json.loads(
        text,
        object_pairs_hook=unique,
        parse_constant=lambda _: require(False, 'nonfinite'),
    )


def read(path: Path) -> str:
    with path.open('rb') as stream:
        data = stream.read(1048577)
    require(len(data) <= 1048576, 'file_size_budget')
    return data.decode('utf-8')


def numeric(value: Any) -> None:
    if isinstance(value, dict):
        for key, item in value.items():
            if key == 'public_status':
                require(item == 'not_claimed', 'scope')
            if key in (
                'vertical', 'integrated_inter_k_tower',
                'whole_tower_authority', 'certificate_retained',
            ):
                require(item is False, 'scope')
            numeric(item)
    elif isinstance(value, list):
        for item in value:
            numeric(item)
    elif type(value) in (int, float):
        require(
            math.isfinite(value) and value >= 0,
            'nonfinite_or_negative',
        )


def timing(row: JsonObject, keys: Iterable[str]) -> None:
    require(
        all(
            type(row[key]) in (int, float)
            and math.isfinite(row[key])
            and row[key] >= 0
            for key in keys
        ),
        'timing',
    )


def is_digest(value: Any) -> bool:
    return (
        isinstance(value, str)
        and re.fullmatch(r'[0-9a-f]{64}', value) is not None
    )


def aggregate_digest(input_hash: str, hashes: Sequence[str]) -> str:
    digest = hashlib.sha256()

    def number(value: int) -> None:
        digest.update(value.to_bytes(8, 'little'))

    def tag(value: str) -> None:
        encoded = value.encode('ascii')
        number(len(encoded))
        digest.update(encoded)

    tag('mhgp7-full-semantic-v1:horizontal-orders')
    tag(input_hash)
    number(len(hashes))
    for index, value in enumerate(hashes):
        number(index + 1)
        tag(value)
    return digest.hexdigest()


def work_policy(row: JsonObject, policy: str, cache: int) -> None:
    require(
        all(type(row[name]) is int for name in WORK_FIELDS),
        'work_integer',
    )
    require(
        row['minimum_hits'] <= row['minimum_lookups'] <= row['face_visits']
        and row['cache_hits'] <= row['cache_lookups'] <= row['face_visits']
        and row['cache_inserts'] <= cache
        and row['cache_skips'] <= row['portal_requests']
        and row['singleton_intruder_resolutions'] <= row['portal_requests']
        and row['direct_lookups'] <= row['meb_calls'],
        'cache_counters',
    )
    if policy == 'lazy':
        require(row['aliases'] == row['alias_hits'] == 0, 'lazy_aliases')
    else:
        require(
            all(
                row[name] == 0
                for name in (
                    'minimum_lookups', 'minimum_hits', 'cache_lookups',
                    'cache_hits', 'cache_inserts', 'cache_skips',
                    'singleton_intruder_resolutions',
                )
            ),
            'eager_cache',
        )


def success_identities(row: JsonObject, policy: str) -> None:
    require(
        row['terminal_direct'] == row['portal_requests']
        and row['meb_calls'] == row['portal_requests'] + row['chain_steps']
        and row['direct_lookups'] == (
            row['singleton_intruder_resolutions'] + row['chain_steps']
        ),
        'success_work_identity',
    )
    if policy == 'lazy':
        require(
            row['minimum_lookups'] == row['face_visits']
            and row['minimum_hits'] + row['cache_hits']
            + row['portal_requests'] == row['face_visits']
            and row['cache_lookups'] == (
                row['cache_hits'] + row['portal_requests']
            )
            and row['cache_inserts'] + row['cache_skips']
            == row['portal_requests'],
            'lazy_success_identity',
        )
    else:
        all_faces = (row['k'] + 1) * row['connection_catalogue_records']
        strict = row['face_visits'] - all_faces
        require(
            strict >= 0
            and row['alias_hits'] + row['portal_requests'] == strict
            and row['aliases'] == (
                row['certificate_minima'] + 2 * all_faces
                - row['face_visits'] + row['portal_requests']
            ),
            'eager_success_identity',
        )


def judge(
    raw: str,
    receipt: JsonObject,
    intent: JsonObject,
    protocol: JsonObject,
) -> JsonObject:
    lines = raw.splitlines()
    split = next(
        (i for i, line in enumerate(lines) if not line.startswith('{')),
        len(lines),
    )
    rows = [loads(line) for line in lines[:split]]
    require(
        len(rows) >= 2
        and rows[0]['type'] == 'configuration'
        and rows[-1]['type'] == 'terminal',
        'terminal',
    )
    expected_types = (
        ['configuration'] + ['order'] * (len(rows) - 2) + ['terminal']
    )
    require([row['type'] for row in rows] == expected_types, 'row_types')
    config, terminal, orders = rows[0], rows[-1], rows[1:-1]
    numeric(rows)
    numeric(receipt)
    require(
        receipt['terminal'] == terminal and receipt['orders'] == orders,
        'receipt_capture',
    )
    require(
        all(
            receipt[key] == intent[key]
            for key in ('id', 'command', 'started')
        ) and 'ended' in receipt,
        'intent_binding',
    )
    n, s, k = config['n'], config['s'], config['kmax_requested']
    policy = next(
        (key for key, value in POLICIES.items()
         if config['alias_policy'] == value),
        '',
    )
    cache = config['cache_entries']
    require(
        policy in POLICIES
        and type(cache) is int and 0 <= cache <= 1000000
        and (policy == 'lazy' or cache == 0),
        'alias_policy',
    )
    work_caps = dict(WORK)
    work_caps['aliases'] = 0 if policy == 'lazy' else 8000000
    fixed_caps = dict(FIXED)
    fixed_caps['max_aliases_per_order'] = work_caps['aliases']
    effective = min(n, k)
    require(
        n in (8, 8000, 16000, 32000)
        and s in (8, 10, 12)
        and k in (5, 10),
        'profile',
    )
    require(
        type(protocol['kmax']) is int and k == protocol['kmax'],
        'protocol_kmax',
    )
    require(
        receipt['id'] == f'n{n}_s{s}_k{k}_{policy}_c{cache}'
        and [n, s, policy, cache] in protocol['planned_sequence'],
        'planned_attempt',
    )
    argv = [
        'timeout', '--signal=TERM', '--kill-after=10s', '600s',
        'taskset', '-c', '6', '/usr/bin/time', '-v',
        protocol['binary'], f'--n={n}', f'--s={s}', f'--kmax={k}',
        f'--alias-policy={policy}',
    ]
    if policy == 'lazy':
        argv.append(f'--cache-entries={cache}')
    require(shlex.split(receipt['command']) == argv, 'command_binding')
    require(
        protocol['public_status'] == 'not_claimed'
        and protocol['authority'] == AUTH
        and protocol['scope'] == (
            'horizontal_relative_orders_not_integrated_inter_k_tower'
        )
        and not protocol['slo_claim']
        and not protocol['paired_F_speedup_claim']
        and not protocol['gcp_used'],
        'protocol_scope',
    )
    for row in rows:
        require(row['schema'] == SCHEMA, 'schema')
    for row in (config, terminal):
        require(
            row['public_status'] == 'not_claimed'
            and row['authority'] == AUTH
            and row['input_digest_kind'] == INPUT_KIND
            and row['certificate_digest_kind'] == DIGEST_KIND
            and row['s_comparison_scope'] == COMPARISON,
            'scope',
        )
        signature = (
            row['n'], row['s'],
            row['kmax_requested'], row['kmax_effective'],
        )
        require(signature == (n, s, k, effective), 'config_binding')
        require(
            row['alias_policy'] == POLICIES[policy]
            and row['cache_entries'] == cache,
            'policy_binding',
        )
    require(
        config['threads'] == 1
        and config['family'] == 'uniform'
        and config['seed'] == 3
        and config['coord'] == 65536
        and config['gpu'] is False
        and config['archive'] is False
        and config['vertical'] is False,
        'fixed_profile',
    )
    require(
        all(
            type(config[key]) is int and config[key] == cap
            for key, cap in fixed_caps.items()
        ),
        'fixed_caps',
    )
    require(
        config['cache_policy'] == (
            'first_C_resolved_nonminimum_strict_facets'
            if policy == 'lazy' else 'not_applicable'
        )
        and config['digest_scratch_bytes_per_node'] == 25
        and config['digest_scratch_bytes_per_parent'] == 8
        and config['max_digest_scratch_logical_bytes'] == 164000000
        and config['input_digest_scratch_bytes_per_point'] in (4, 8)
        and config['digest_scratch_scope'] == (
            'additional_logical_sizes_not_allocator_capacity_or_RSS_bound'
        )
        and config['digest_timing_scope'] == (
            'included_in_elapsed_and_stage_digest_ms_not_subtracted'
        ),
        'digest_configuration',
    )
    require(
        0 < config['vm_soft_limit_bytes'] <= 26 << 30
        and config['catalogue_cardinality_max'] == min(n, k + 1),
        'vm_window',
    )
    require(
        config['sizeof_ball_candidate'] > 0
        and config['candidate_fusion_cap_2e'] == (
            (8 << 30) // (2 * config['sizeof_ball_candidate'])
        ),
        'fusion_cap',
    )
    require(
        [row['k'] for row in orders] == list(range(1, len(orders) + 1))
        and len(orders) <= effective,
        'order_sequence',
    )
    completed, previous = 0, 0
    for index, row in enumerate(orders):
        timing(
            row,
            (
                'expand_ms', 'build_ms', 'read_ms', 'digest_ms', 'release_ms',
                'rss_mib_sample', 'hwm_mib_sample',
            ),
        )
        require(
            row['provisional'] is True
            and row['whole_tower_authority'] is False,
            'provisional',
        )
        require(row['minimum_catalogue_records'] == previous,
                'catalogue_transfer')
        require(
            row['alias_policy'] == POLICIES[policy]
            and row['cache_entries'] == cache,
            'policy_binding',
        )
        previous = row['connection_catalogue_records']
        require(
            all(
                name in row
                and type(row[name]) is int
                and row[name] <= cap
                for name, cap in work_caps.items()
            ),
            'work_cap',
        )
        require(row['meb_calls'] == row['geometry_meb_calls'],
                'meb_accounting')
        work_policy(row, policy, cache)
        if row['outcome'] == 'complete_relative':
            completed += 1
            require(is_digest(row['certificate_digest']), 'order_digest')
            require(
                row['certificate_minima'] == (
                    n if row['k'] == 1 else row['minimum_catalogue_records']
                ),
                'minimum_binding',
            )
            require(
                row['certificate_nodes'] <= 2 * row['certificate_minima'] - 1,
                'multifusion_arity',
            )
            success_identities(row, policy)
            require(
                row['reason'] == AUTH
                and row['terminal_roots'] == 1
                and row['terminal_coverage_points'] == n
                and row['certificate_nodes'] >= row['certificate_minima'] > 0
                and row['certificate_parent_refs'] == (
                    row['certificate_nodes'] - 1
                ),
                'positive_sentinels',
            )
            require(
                row['input_records'] == (
                    row['minimum_catalogue_records'] + previous
                ),
                'input_count',
            )
        else:
            empty_payload = all(
                row[name] == 0
                for name in (
                    'certificate_nodes', 'certificate_minima',
                    'certificate_parent_refs', 'terminal_roots',
                    'terminal_coverage_points',
                )
            )
            require(
                index == len(orders) - 1
                and row['outcome'] == terminal['outcome']
                and row['reason'] == terminal['reason']
                and empty_payload and row['certificate_digest'] == '',
                'refused_order',
            )
    code = {
        'complete_relative': 0,
        'invalid_input': 2,
        'unsupported_degeneracy': 2,
        'resource_exhausted': 2,
        'invariant_violated': 3,
    }[terminal['outcome']]
    require(
        type(receipt['exit_code']) is int
        and receipt['exit_code'] == terminal['exit_code'] == code,
        'exit_code',
    )
    require(
        terminal['terminal_status'] == (
            'completed' if code == 0 else 'failed'
        )
        and terminal['complete_requested_horizontal_orders'] is (code == 0),
        'terminal_status',
    )
    require(
        terminal['completed_orders_diagnostic'] == completed
        and completed <= terminal['last_order']
        <= min(completed + 1, effective),
        'completed_prefix',
    )
    require(code != 0 or completed == effective, 'global_completion')
    require(
        (is_digest(terminal['input_digest'])
         or (code != 0 and not orders and terminal['input_digest'] == ''))
        and terminal['digest_proves_catalogue_completeness'] is False,
        'digest_scope',
    )
    if code == 0:
        require(
            terminal['certificate_digest'] == aggregate_digest(
                terminal['input_digest'],
                [row['certificate_digest'] for row in orders],
            ),
            'aggregate_digest',
        )
    else:
        require(terminal['certificate_digest'] == '', 'refusal_digest')
    require(
        terminal['integrated_inter_k_tower'] is False
        and terminal['certificate_retained'] is False
        and terminal['terminal_root_coverage_proves_equality'] is False,
        'scope',
    )
    worker_keys = {
        key for key in terminal if key.startswith('workers_')
    }
    require(worker_keys == WORKERS, 'worker_inventory')
    require(
        all(
            type(terminal[key]) is int and terminal[key] <= 1
            for key in WORKERS
        ),
        'mono',
    )
    require(
        set(terminal['stage_ms']) == set(STAGES)
        and terminal['last_stage'] in STAGES,
        'stage_inventory',
    )
    timing(terminal['stage_ms'], STAGES)
    timing(
        terminal,
        (
            'elapsed_before_terminal_ms', 'provisional_output_ms', 'digest_ms',
            'compute_read_release_ms_subtracted_diagnostic',
            'generation_wspd_ms', 'generation_rects_ms',
            'rss_mib_sample', 'hwm_mib_sample',
        ),
    )
    require(
        terminal['reference_timing'] == (
            'elapsed_before_terminal_ms_includes_provisional_output'
        )
        and terminal['subtracted_timing_scope'] == (
            'diagnostic_only_not_an_independent_timer'
        ),
        'timing_scope',
    )
    require(
        # Each stage timer is disjoint and lies inside the elapsed interval.
        # Values are printed to six decimals in milliseconds. Accumulated
        # roundoff is bounded by one micro-ms per printed operand; the three
        # independently rounded subtraction operands admit three micro-ms.
        sum(terminal['stage_ms'].values())
        <= terminal['elapsed_before_terminal_ms'] + (len(STAGES) + 1) * 1e-6
        and terminal['provisional_output_ms']
        <= terminal['elapsed_before_terminal_ms'] + 1e-6
        and abs(
            terminal['compute_read_release_ms_subtracted_diagnostic']
            - max(
                0.0,
                terminal['elapsed_before_terminal_ms']
                - terminal['provisional_output_ms'],
            )
        ) <= 3e-6,
        'timing_consistency',
    )
    require(
        terminal['digest_ms'] == terminal['stage_ms']['digest']
        and terminal['digest_ms'] + 0.000001 * (len(orders) + 1)
        >= sum(row['digest_ms'] for row in orders)
        and terminal['elapsed_before_terminal_ms'] + 0.000001
        >= terminal['digest_ms'],
        'digest_timing',
    )
    last_work = terminal['last_order_work']
    require(
        last_work['diagnostic_only'] is True
        and set(last_work) == set(WORK_FIELDS) | {'diagnostic_only'},
        'work_inventory',
    )
    require(
        all(
            type(last_work[name]) is int and last_work[name] <= cap
            for name, cap in work_caps.items()
            if not name.startswith('certificate_')
        ),
        'work_cap',
    )
    work_policy(last_work, policy, cache)
    if orders:
        if orders[-1]['k'] == terminal['last_order']:
            require(
                {key: last_work[key] for key in WORK_FIELDS}
                == {key: orders[-1][key] for key in WORK_FIELDS},
                'last_work_binding',
            )
        else:
            # A read failure can precede the next provisional order line.
            # Do not manufacture that missing line or publish its result.
            require(
                code != 0
                and completed == len(orders)
                and terminal['last_order'] == orders[-1]['k'] + 1,
                'unemitted_order',
            )
    require(
        terminal['raw_candidates'] <= config['max_raw_candidates']
        and terminal['unique_candidates'] <= terminal['raw_candidates']
        and terminal['census_balls'] <= terminal['unique_candidates'],
        'frontier_counts',
    )
    if code == 0:
        require(
            terminal['frontier_ledger_closed']
            and terminal['rank_window_regular']
            and terminal['generation_cap_refus']
            == terminal['invariant_jneg']
            == terminal['rank_relevant_extra_shell'] == 0,
            'frontier_complete',
        )
        require(
            sum(row['connection_catalogue_records'] for row in orders)
            == terminal['census_balls'],
            'catalogue_partition',
        )
    if terminal['frontier_ledger_closed']:
        expected_mass = n * (n - 1) // 2
        require(
            terminal['pair_mass_expected_per_lane'] == expected_mass
            and all(
                terminal[f'ledger_q{q}_emitted']
                + terminal[f'ledger_q{q}_killed'] == expected_mass
                for q in (2, 3, 4)
            ),
            'ledger',
        )
    tail = lines[split:]
    if code != 0:
        require(
            tail.pop(0) == f'Command exited with non-zero status {code}',
            'gnu_nonzero',
        )
    fields = unique(line.strip().split(': ', 1) for line in tail)
    require(
        fields['Command being timed'] == '"' + shlex.join(argv[9:]) + '"'
        and fields['Exit status'] == str(code)
        and tail[-1].strip() == f'Exit status: {code}',
        'gnu_exit',
    )
    for name in ('User time (seconds)', 'System time (seconds)'):
        value = float(fields[name])
        require(math.isfinite(value) and value >= 0, 'gnu_timing')
    elapsed = fields['Elapsed (wall clock) time (h:mm:ss or m:ss)']
    require(
        re.fullmatch(r'(?:[0-9]+:)?[0-9]+:[0-5][0-9](?:\.[0-9]+)?', elapsed)
        is not None,
        'gnu_timing',
    )
    require(fields['Maximum resident set size (kbytes)'].isdigit(), 'gnu_rss')
    return {
        'audit_status': 'valid',
        'attempt_success': code == 0,
        'outcome': terminal['outcome'],
        'orders_complete': completed,
        'alias_policy': policy,
        'cache_entries': cache,
        'input_digest': terminal['input_digest'],
        'certificate_digest': terminal['certificate_digest'],
        'scope': 'receipt_and_digest_binding_not_geometry_or_completeness',
    }


def selftest(
    raw: str,
    receipt: JsonObject,
    intent: JsonObject,
    protocol: JsonObject,
) -> JsonObject:
    require(
        judge(raw, receipt, intent, protocol)['attempt_success'],
        'positive_fixture',
    )
    rows = [
        loads(line) for line in raw.splitlines() if line.startswith('{')
    ]
    tail = '\n'.join(
        line for line in raw.splitlines() if not line.startswith('{')
    ) + '\n'
    refused = copy.deepcopy(rows)
    refused_receipt = copy.deepcopy(receipt)
    refused[-2].update(
        outcome='resource_exhausted',
        reason='silent_meb_support_budget',
        meb_supports=WORK['meb_supports'],
        certificate_digest='',
        certificate_nodes=0,
        certificate_minima=0,
        certificate_parent_refs=0,
        terminal_roots=0,
        terminal_coverage_points=0,
    )
    refused[-1].update(
        outcome='resource_exhausted',
        reason='silent_meb_support_budget',
        terminal_status='failed',
        complete_requested_horizontal_orders=False,
        exit_code=2,
        completed_orders_diagnostic=len(rows) - 3,
        last_stage='full',
        certificate_digest='',
    )
    refused[-1]['last_order_work']['meb_supports'] = WORK['meb_supports']
    refused_receipt.update(
        terminal=refused[-1], orders=refused[1:-1], exit_code=2,
    )
    refused_raw = (
        '\n'.join(json.dumps(row) for row in refused)
        + '\nCommand exited with non-zero status 2\n'
        + tail.replace('Exit status: 0', 'Exit status: 2')
    )
    require(
        judge(refused_raw, refused_receipt, intent, protocol)[
            'attempt_success'
        ] is False,
        'synthetic_refusal',
    )
    unprinted = copy.deepcopy(rows[:-2] + [rows[-1]])
    unprinted[-1].update(
        outcome='resource_exhausted',
        reason='full_read_allocation_failed',
        terminal_status='failed',
        complete_requested_horizontal_orders=False,
        exit_code=2,
        completed_orders_diagnostic=len(unprinted) - 2,
        last_stage='read',
        certificate_digest='',
    )
    unprinted_receipt = copy.deepcopy(receipt)
    unprinted_receipt.update(
        terminal=unprinted[-1], orders=unprinted[1:-1], exit_code=2,
    )
    unprinted_raw = (
        '\n'.join(json.dumps(row) for row in unprinted)
        + '\nCommand exited with non-zero status 2\n'
        + tail.replace('Exit status: 0', 'Exit status: 2')
    )
    require(
        judge(unprinted_raw, unprinted_receipt, intent, protocol)[
            'attempt_success'
        ] is False,
        'synthetic_unemitted_read_refusal',
    )
    killed = []
    mutations = {
        'missing_terminal': 'terminal',
        'promoted_status': 'scope',
        'cap_exceeded': 'work_cap',
        'skipped_k': 'order_sequence',
        'timing_nan': 'nonfinite',
        'altered_receipt': 'receipt_capture',
        'protocol_kmax': 'protocol_kmax',
        'missing_worker': 'worker_inventory',
        'last_work_disagreement': 'last_work_binding',
        'policy_mismatch': 'policy_binding',
        'cache_capacity': 'cache_counters',
        'order_digest': 'order_digest',
        'global_digest': 'aggregate_digest',
        'success_identity': 'success_work_identity',
        'minimum_count': 'minimum_binding',
        'non_multifusion_node_count': 'multifusion_arity',
        'elapsed_zero': 'timing_consistency',
        'subtracted_time': 'timing_consistency',
    }
    for name, expected_reason in mutations.items():
        altered = copy.deepcopy(rows)
        rec = copy.deepcopy(receipt)
        plan = copy.deepcopy(protocol)
        if name == 'missing_terminal':
            altered.pop()
        elif name == 'promoted_status':
            altered[-1]['public_status'] = 'exact'
        elif name == 'cap_exceeded':
            altered[1]['query_nodes'] = WORK['query_nodes'] + 1
        elif name == 'skipped_k':
            altered[1]['k'] = 2
        elif name == 'timing_nan':
            altered[-1]['elapsed_before_terminal_ms'] = float('nan')
        elif name == 'protocol_kmax':
            plan['kmax'] = 5 if protocol['kmax'] == 10 else 10
        elif name == 'missing_worker':
            del altered[-1]['workers_census']
        elif name == 'last_work_disagreement':
            work = altered[-1]['last_order_work']
            work['query_range_skips'] += 1
        elif name == 'policy_mismatch':
            altered[1]['alias_policy'] = 'unknown_policy'
        elif name == 'cache_capacity':
            altered[1]['cache_inserts'] = altered[0]['cache_entries'] + 1
        elif name == 'order_digest':
            altered[1]['certificate_digest'] = 'x' * 64
        elif name == 'global_digest':
            value = altered[-1]['certificate_digest']
            altered[-1]['certificate_digest'] = (
                ('0' if value[0] != '0' else '1') + value[1:]
            )
        elif name == 'success_identity':
            altered[1]['terminal_direct'] += 1
        elif name == 'minimum_count':
            altered[1]['certificate_minima'] = 1
        elif name == 'non_multifusion_node_count':
            altered[1]['certificate_nodes'] = (
                2 * altered[1]['certificate_minima']
            )
            altered[1]['certificate_parent_refs'] = (
                altered[1]['certificate_nodes'] - 1
            )
        elif name == 'elapsed_zero':
            altered[-1]['elapsed_before_terminal_ms'] = 0
        elif name == 'subtracted_time':
            altered[-1][
                'compute_read_release_ms_subtracted_diagnostic'
            ] += 1
        rec['terminal'] = altered[-1]
        rec['orders'] = altered[1:-1]
        if name == 'altered_receipt':
            rec['terminal']['last_order'] = 0
        changed = '\n'.join(json.dumps(row) for row in altered) + '\n' + tail
        try:
            judge(
                raw if name == 'altered_receipt' else changed,
                rec,
                intent,
                plan,
            )
        except ValueError as error:
            require(
                str(error) == expected_reason,
                'mutant_wrong_reason:' + name + ':' + str(error),
            )
            killed.append(name)
    promoted_refusal = copy.deepcopy(refused)
    promoted_refusal[-1]['certificate_digest'] = rows[-1]['certificate_digest']
    promoted_receipt = copy.deepcopy(refused_receipt)
    promoted_receipt['terminal'] = promoted_refusal[-1]
    promoted_raw = (
        '\n'.join(json.dumps(row) for row in promoted_refusal)
        + '\nCommand exited with non-zero status 2\n'
        + tail.replace('Exit status: 0', 'Exit status: 2')
    )
    try:
        judge(promoted_raw, promoted_receipt, intent, protocol)
    except ValueError as error:
        require(str(error) == 'refusal_digest', 'refusal_mutant_wrong_reason')
        killed.append('refusal_global_digest')
    require(len(killed) == 19, 'mutant_nonvacuum')
    return {
        'audit_status': 'selftests_passed',
        'real_positive': 1,
        'synthetic_refusal': 1,
        'synthetic_unemitted_read_refusal': 1,
        'mutants_killed': killed,
    }


def main() -> int:
    require(
        len(sys.argv) in (2, 3)
        and (len(sys.argv) == 2 or sys.argv[1] == '--selftest'),
        'arguments',
    )
    path = Path(sys.argv[-1])
    require(path.name.endswith('.receipt.json'), 'receipt_path')
    stem = path.name.removesuffix('.receipt.json')
    require(
        re.fullmatch(r'n\d+_s\d+_k\d+_(?:eager|lazy)_c\d+', stem) is not None,
        'attempt_id',
    )
    raw = read(path.with_name(stem + '.raw.txt'))
    receipt = loads(read(path))
    intent = loads(read(path.with_name(stem + '.intent.json')))
    protocol = loads(read(path.parent / 'protocol.json'))
    sources = loads(read(path.parent / 'sources_before.json'))
    require(
        sources['binary'] == protocol['binary']
        and sources['binary_sha256'] == protocol['binary_sha256']
        and sources['files']['morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp']
        == protocol['probe_sha256']
        and sources['files'][
            'morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp'
        ] == protocol['digest_header_sha256']
        and sources['files']['morsehgp3D_v7/src/forest/full_gabriel.hpp']
        == protocol['producer_sha256'],
        'declared_source_binding',
    )
    if len(sys.argv) == 3:
        result = selftest(raw, receipt, intent, protocol)
    else:
        result = judge(raw, receipt, intent, protocol)
    result['raw_sha256'] = hashlib.sha256(raw.encode('utf-8')).hexdigest()
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (
        ValueError, KeyError, TypeError, IndexError, OSError,
        OverflowError, RecursionError,
    ) as error:
        print(json.dumps(
            {'audit_status': 'invalid', 'reason': str(error)},
            sort_keys=True,
        ))
        sys.exit(1)
