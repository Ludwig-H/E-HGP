#!/usr/bin/env python3
"""Read-only v2/v3/v4/v5 receipt judge; not a geometry/completeness oracle.

Explicit port of frozen v1 judge:
24e789459ee7adb8b48819dddc8bef8832b2b152ad9418c1a1d281038315e2c7.
Usage: full_gabriel_lazy_probe_audit.py [--selftest] PATH.receipt.json
Code 0 means a coherent receipt, possibly a refusal, not a successful probe.
Missing terminals/censored captures cannot pass. No engine is invoked.
Forest bytes are not retained: this checks reported per-order digest formats
and recomputes their global binding, not their geometric truth.
Historical v2 output is unchanged. V3 declares successor accounting v2;
its path costs require helper tests/paired evidence, not receipt counters alone.
V4 independently declares the filtered MEB calendar and an explicit order-wide
proposal cap. Its five extra diagnostics never relabel historical v2/v3 bytes.
V5 declares memory/representation bounds without artificial operation quotas.
New diagnostic inequalities are v5-only: historical v2/v3/v4 verdicts retain
their exact predicates and output, including their documented limitations.
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
CURRENT_SCHEMA = 'mhgp7-full-gabriel-probe-v3'
MEB_SCHEMA = 'mhgp7-full-gabriel-probe-v4'
MEMORY_SCHEMA = 'mhgp7-full-gabriel-probe-v5'
LIMITS_PROFILE = 'memory_guarded_no_operation_quotas_v1'
U64_MAX = (1 << 64) - 1
MEMORY_SCHEMAS = (MEB_SCHEMA, MEMORY_SCHEMA)
LEGACY_ACCOUNTING = 'full_successor_reads_writes_v1'
CURRENT_ACCOUNTING = 'full_successor_reads_writes_no_last_pair_v2'
ACCOUNTING_BY_SCHEMA = {
    SCHEMA: LEGACY_ACCOUNTING,
    CURRENT_SCHEMA: CURRENT_ACCOUNTING,
    MEB_SCHEMA: CURRENT_ACCOUNTING,
    MEMORY_SCHEMA: CURRENT_ACCOUNTING,
}
MEB_ACCOUNTING = 'reference_ordinal_plus_native_z_q3_q4_proposal_v2'
MEB_CAP_FIELD = 'max_meb_proposal_supports_per_order'
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
MEB_FIELDS = (
    'meb_proposal_supports meb_proposal_pivots meb_proposal_certified '
    'meb_proposal_fallback meb_reference_supports'
).split()
MEB_WORK_FIELDS = WORK_FIELDS + MEB_FIELDS
MAX_MEB_PROPOSAL_SUPPORTS = 146 * WORK['meb_calls']
OPERATION_FIELDS = ('face_visits portal_requests chain_steps meb_calls query_nodes '
                    'meb_supports successor_steps').split()


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


def meb_work(row: JsonObject, cap: int, complete: bool) -> None:
    """Prospective order-wide work; unfinished calls have only prefix bounds."""
    require(all(type(row.get(name)) is int and row[name] >= 0
                for name in MEB_FIELDS), 'meb_work_integer')
    p, pivots, certified, fallback, actual = (row[name] for name in MEB_FIELDS)
    require(p <= cap and pivots <= p and certified <= p
            and actual <= row['meb_supports']
            and certified + fallback <= row['geometry_meb_calls'] <= row['meb_calls'],
            'meb_work_bounds')
    if cap == 0:
        require(p == pivots == certified == 0 and actual == row['meb_supports'],
                'meb_disabled_reference')
    if complete:
        require(certified + fallback == row['geometry_meb_calls'] == row['meb_calls'],
                'meb_success_identity')


def memory_meb_work(row: JsonObject, cap: int, complete: bool) -> None:
    """Necessary fresh-order invariants, not a realizability/geometry oracle.

    F adds the same amount to c and A. Each certificate adds 1..550 to c
    alone (possibly truncated by a positive L margin). A closed terminal
    cannot interrupt that arithmetic transaction. Python products do not wrap.
    """
    meb_work(row, cap, complete)
    require(all(type(row[name]) is int and 0 <= row[name] <= U64_MAX
                for name in MEB_FIELDS + ['meb_supports', 'geometry_meb_calls', 'meb_calls']), 'meb_u64')
    require(row['meb_proposal_supports'] <= 146 * row['meb_calls'], 'meb_forms_per_call')
    virtual = row['meb_supports'] - row['meb_reference_supports']
    require(row['meb_proposal_certified'] <= virtual <= 550 * row['meb_proposal_certified'],
            'meb_virtual_ordinal_interval')


def memory_profile(config: JsonObject, policy: str) -> tuple[dict, dict]:
    """Reproduce the v5 declared per-arena logical/representation ceilings."""
    require(config.get('limits_profile') == LIMITS_PROFILE and
            config['work_counter_ceiling'] == U64_MAX and
            config['storage_limit_kind'] == 'per_named_logical_arena_and_representation', 'memory_profile')
    sizes = ('sizeof_input_point sizeof_ball_candidate sizeof_forest_event sizeof_facet_key sizeof_full_node '
             'sizeof_full_batch sizeof_full_record sizeof_alias_entry_payload sizeof_full_parent_id sizeof_point_id').split()
    require(all(type(config[key]) is int and 0 < config[key] <= 4096 for key in sizes), 'memory_size_domain')
    size_max, difference_max = config['storage_size_max'], config['storage_difference_max']
    require(type(size_max) is int and size_max in ((1 << 32)-1, U64_MAX) and
            type(difference_max) is int and difference_max == size_max // 2 and
            config['sizeof_full_parent_id'] == 8 and config['sizeof_point_id'] == 4, 'representation_domain')
    budget = 8 << 30
    def entries(name: str) -> int:
        return min(budget // config[name], size_max // config[name], difference_max // config[name])
    cache = entries('sizeof_alias_entry_payload')
    work = dict.fromkeys(OPERATION_FIELDS, U64_MAX)
    work.update(input_records=min(entries('sizeof_forest_event'), entries('sizeof_full_record')),
                aliases=0 if policy == 'lazy' else cache,
                certificate_nodes=min(entries('sizeof_full_node'), entries('sizeof_facet_key'),
                                      entries('sizeof_full_parent_id'), budget, size_max, difference_max),
                certificate_parent_refs=entries('sizeof_full_parent_id'))
    fixed = {'max_'+name+'_per_order': value for name, value in work.items()}
    fixed.update(max_raw_candidates=(1 << 32)-1,
        effective_raw_cap=min((1 << 32)-1, budget // config['sizeof_ball_candidate']),
        named_payload_budget_bytes=budget,
        max_points_per_order=min(entries('sizeof_input_point'), (1 << 30)-1),
        max_certificate_batches_per_order=entries('sizeof_full_batch'),
        max_read_point_refs_per_order=entries('sizeof_point_id'), max_cache_entries=cache)
    return work, fixed


def memory_binding(rows: Sequence[JsonObject], protocol: JsonObject) -> str:
    require(all(row.get('limits_profile') == LIMITS_PROFILE for row in rows) and
            protocol.get('limits_profile') == LIMITS_PROFILE, 'limits_profile_binding')
    cap = rows[0][MEB_CAP_FIELD]
    kind = rows[0].get('meb_proposal_budget_kind')
    require(type(kind) is str and kind in ('disabled', 'finite', 'unlimited') and
            ((kind == 'disabled' and cap == 0) or (kind == 'finite' and 0 < cap <= U64_MAX) or
             (kind == 'unlimited' and cap == U64_MAX)), 'meb_budget_kind')
    require(all(row.get('meb_proposal_budget_kind') == kind for row in rows), 'meb_budget_kind_binding')
    require(not {'historical_fold_inflight', 'historical_event_payload_factor'}.intersection(rows[0]) and
            rows[0].get('legacy_F_fold_guard_applied') is False and
            rows[-1].get('legacy_F_fold_guard_applied') is False, 'unused_F_guard_scope')
    for row in rows:
        for key, value in row.items():
            if key == 'last_order_work':
                require(all(type(v) is int and 0 <= v <= U64_MAX for k, v in value.items()
                            if k != 'diagnostic_only'), 'v5_u64_wire')
            elif key == 'stage_ms' or key.endswith('_ms') or key in ('rss_mib_sample', 'hwm_mib_sample',
                                                                    'compute_read_release_ms_subtracted_diagnostic'):
                continue  # Existing finite/nonnegative timing checks below.
            elif type(value) not in (str, bool):
                require(type(value) is int and 0 <= value <= U64_MAX, 'v5_u64_wire')
    return kind


def accounting_binding(rows: Sequence[JsonObject], protocol: JsonObject) -> str:
    """An explicit version, never inferred from the size of work counters.

    Historical protocols omit both new fields and remain readable unchanged.
    Fresh protocols may also declare the legacy arm without rewriting its raw
    v2 records. The nested last_order_work inherits its terminal's accounting;
    its inventory remains 25 fields in v2/v3 and is explicitly 30 in v4.
    """
    schema = rows[0]['schema']
    require(type(schema) is str and schema in ACCOUNTING_BY_SCHEMA, 'schema')
    require(all(row['schema'] == schema for row in rows), 'schema')
    accounting = ACCOUNTING_BY_SCHEMA[schema]
    if schema == SCHEMA:
        require(all('successor_accounting' not in row for row in rows),
                'legacy_accounting_field')
    else:
        require(all(type(row.get('successor_accounting')) is str
                    and row['successor_accounting'] == accounting for row in rows),
                'successor_accounting')
    if schema != SCHEMA or any(
        name in protocol for name in ('probe_schema', 'successor_accounting')
    ):
        require(protocol.get('probe_schema') == schema
                and protocol.get('successor_accounting') == accounting,
                'protocol_accounting')
    if schema in MEMORY_SCHEMAS:
        require(all(type(row.get('meb_accounting')) is str
                    and row['meb_accounting'] == MEB_ACCOUNTING for row in rows),
                'meb_accounting')
        require(protocol.get('meb_accounting') == MEB_ACCOUNTING,
                'protocol_meb_accounting')
        cap = rows[0].get(MEB_CAP_FIELD)
        require(type(cap) is int and 0 <= cap <= (U64_MAX if schema == MEMORY_SCHEMA else MAX_MEB_PROPOSAL_SUPPORTS),
                'meb_proposal_cap')
        require(all(type(row.get(MEB_CAP_FIELD)) is int and row[MEB_CAP_FIELD] == cap
                    for row in rows), 'meb_cap_binding')
    else:
        forbidden = set(MEB_FIELDS) | {'meb_accounting', MEB_CAP_FIELD}
        require(all(not forbidden.intersection(row) and
                    not set(MEB_FIELDS).intersection(row.get('last_order_work', {}))
                    for row in rows) and 'meb_accounting' not in protocol,
                'legacy_meb_fields')
    if schema == MEMORY_SCHEMA:
        memory_binding(rows, protocol)
    else:
        require(all(not {'limits_profile', 'meb_proposal_budget_kind', 'work_counter_ceiling',
                         'storage_limit_kind'}.intersection(row) for row in rows) and
                'limits_profile' not in protocol, 'legacy_limits_fields')
    return schema


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
    memory = config.get('schema') == MEMORY_SCHEMA
    if memory:
        work_caps, fixed_caps = memory_profile(config, policy)
        cache_ceiling = fixed_caps['max_cache_entries']
    else:
        work_caps = dict(WORK)
        work_caps['aliases'] = 0 if policy == 'lazy' else 8000000
        fixed_caps = dict(FIXED)
        fixed_caps['max_aliases_per_order'] = work_caps['aliases']
        cache_ceiling = 1000000
    require(
        policy in POLICIES
        and type(cache) is int and 0 <= cache <= cache_ceiling
        and (policy == 'lazy' or cache == 0),
        'alias_policy',
    )
    effective = min(n, k)
    if memory:
        require(all(type(config[name]) is int for name in ('n', 's', 'kmax_requested', 'kmax_effective',
                    'threads', 'seed', 'coord', 'work_counter_ceiling')), 'memory_profile_integer')
    require(
        (type(n) is int and 2 <= n <= fixed_caps['max_points_per_order'] if memory else n in (8, 8000, 16000, 32000))
        and s in (8, 10, 12)
        and k in (5, 10),
        'profile',
    )
    require(
        type(protocol['kmax']) is int and k == protocol['kmax'],
        'protocol_kmax',
    )
    schema = accounting_binding(rows, protocol)
    proposal = config[MEB_CAP_FIELD] if schema in MEMORY_SCHEMAS else None
    proposal_token = 'unlimited' if memory and config['meb_proposal_budget_kind'] == 'unlimited' else proposal
    suffix = f'_p{proposal_token}' if schema in MEMORY_SCHEMAS else ''
    planned = [n, s, policy, cache] + ([proposal_token] if schema in MEMORY_SCHEMAS else [])
    require(
        receipt['id'] == f'n{n}_s{s}_k{k}_{policy}_c{cache}' + suffix
        and any(encoded_plan == json.dumps(planned) for encoded_plan in
                (json.dumps(item) for item in protocol['planned_sequence'])),
        'planned_attempt',
    )
    argv = [
        'timeout', '--signal=TERM', '--kill-after=10s', '1200s' if memory else '600s',
        'taskset', '-c', '6', '/usr/bin/time', '-v',
        protocol['binary'], f'--n={n}', f'--s={s}', f'--kmax={k}',
        f'--alias-policy={policy}',
    ]
    if policy == 'lazy':
        argv.append(f'--cache-entries={cache}')
    if schema in MEMORY_SCHEMAS:
        argv.append(f'--meb-proposal-supports={proposal_token}')
    require(shlex.split(receipt['command']) == argv, 'command_binding')
    if memory:
        require(type(protocol.get('timeout_seconds_each')) is int and protocol['timeout_seconds_each'] == 1200 and
                type(receipt.get('outer_timeout_seconds')) is int and receipt['outer_timeout_seconds'] == 1220 and
                type(receipt.get('cpu_limit_seconds')) is int and receipt['cpu_limit_seconds'] == 1220,
                'memory_timeout_binding')
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
        and config['max_digest_scratch_logical_bytes'] == (
            25 * work_caps['certificate_nodes'] + 8 * work_caps['certificate_parent_refs'] if memory else 164000000)
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
        if schema in MEMORY_SCHEMAS:
            (memory_meb_work if memory else meb_work)(row, proposal, row['outcome'] == 'complete_relative')
        else:
            require(row['meb_calls'] == row['geometry_meb_calls'], 'meb_accounting')
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
    work_fields = MEB_WORK_FIELDS if schema in MEMORY_SCHEMAS else WORK_FIELDS
    require(
        last_work['diagnostic_only'] is True
        and set(last_work) == set(work_fields) | {'diagnostic_only'},
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
    if schema in MEMORY_SCHEMAS:
        (memory_meb_work if memory else meb_work)(last_work, proposal, code == 0)
    if orders:
        if orders[-1]['k'] == terminal['last_order']:
            require(
                {key: last_work[key] for key in work_fields}
                == {key: orders[-1][key] for key in work_fields},
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
    result = {
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
    if schema != SCHEMA:
        result.update(probe_schema=schema, successor_accounting=CURRENT_ACCOUNTING)
    if schema in MEMORY_SCHEMAS:
        result.update(meb_accounting=MEB_ACCOUNTING, **{MEB_CAP_FIELD: proposal})
    if memory:
        result.update(limits_profile=LIMITS_PROFILE, meb_proposal_budget_kind=config['meb_proposal_budget_kind'],
                      work_diagnostics='fresh_order_146_forms_and_1_to_550_virtual_supports_per_certificate')
    return result


def memory_selftest(raw: str, receipt: JsonObject, intent: JsonObject, protocol: JsonObject) -> JsonObject:
    """New-profile metadata models; old selftest outputs remain untouched."""
    real = judge(raw, receipt, intent, protocol)
    require(real['attempt_success'], 'positive_fixture')
    rows = [loads(line) for line in raw.splitlines() if line.startswith('{')]
    tail = '\n'.join(line for line in raw.splitlines() if not line.startswith('{')) + '\n'
    killed = []
    targets = {'config': 0, 'order': 1, 'terminal': -1}
    cases = [
        ('profile_missing', 'config', 'limits_profile', None, 'memory_profile'),
        ('profile_changed', 'config', 'limits_profile', 'other', 'memory_profile'),
        ('profile_order', 'order', 'limits_profile', 'other', 'limits_profile_binding'),
        ('profile_terminal', 'terminal', 'limits_profile', 'other', 'limits_profile_binding'),
        ('profile_protocol', 'protocol', 'limits_profile', 'other', 'limits_profile_binding'),
        ('kind_missing', 'config', 'meb_proposal_budget_kind', None, 'meb_budget_kind'),
        ('kind_unknown', 'config', 'meb_proposal_budget_kind', 'other', 'meb_budget_kind'),
        ('kind_order', 'order', 'meb_proposal_budget_kind', 'other', 'meb_budget_kind_binding'),
        ('kind_terminal', 'terminal', 'meb_proposal_budget_kind', 'other', 'meb_budget_kind_binding'),
        ('old_fold_field', 'config', 'historical_fold_inflight', 2, 'unused_F_guard_scope'),
        ('old_fold_guard', 'terminal', 'legacy_F_fold_guard_applied', True, 'unused_F_guard_scope'),
        ('operation_quota', 'config', 'max_meb_calls_per_order', 4000000, 'fixed_caps'),
        ('operation_overflow', 'order', 'query_nodes', U64_MAX+1, 'v5_u64_wire'),
        ('counter_float', 'order', 'query_nodes', 0.0, 'v5_u64_wire'),
        ('counter_bool', 'order', 'query_nodes', False, 'work_cap'),
        ('storage_kind', 'config', 'storage_limit_kind', 'global_RSS', 'memory_profile'),
        ('storage_size_zero', 'config', 'sizeof_full_node', 0, 'memory_size_domain'),
        ('storage_size_float', 'config', 'sizeof_full_node', 64.0, 'memory_size_domain'),
        ('difference_range', 'config', 'storage_difference_max', 17, 'representation_domain'),
        ('parent_id_size', 'config', 'sizeof_full_parent_id', 4, 'representation_domain'),
        ('node_cap', 'config', 'max_certificate_nodes_per_order', 1, 'fixed_caps'),
        ('read_cap', 'config', 'max_read_point_refs_per_order', 1, 'fixed_caps'),
        ('cache_cap', 'config', 'max_cache_entries', 1, 'fixed_caps'),
        ('digest_cap', 'config', 'max_digest_scratch_logical_bytes', 1, 'digest_configuration'),
        ('n_outside', 'config', 'n', rows[0]['max_points_per_order']+1, 'profile'),
        ('n_one', 'config', 'n', 1, 'profile'),
        ('n_float', 'config', 'n', float(rows[0]['n']), 'memory_profile_integer'),
        ('schema_mixed', 'order', 'schema', MEB_SCHEMA, 'schema'),
        ('meb_calendar', 'order', 'meb_accounting', 'unknown', 'meb_accounting'),
        ('successor_calendar', 'order', 'successor_accounting', 'unknown', 'successor_accounting'),
        ('order_digest', 'order', 'certificate_digest', 'bad', 'order_digest'),
        ('global_digest', 'terminal', 'certificate_digest', '0'*64, 'aggregate_digest'),
        ('public_promotion', 'terminal', 'public_status', 'exact', 'scope'),
        ('skipped_order', 'order', 'k', 2, 'order_sequence'),
        ('last_work', 'last', 'query_range_skips', rows[-1]['last_order_work']['query_range_skips']+1, 'last_work_binding'),
    ]
    for name, where, key, value, reason in cases:
        changed, rec, plan = copy.deepcopy(rows), copy.deepcopy(receipt), copy.deepcopy(protocol)
        target = plan if where == 'protocol' else changed[-1]['last_order_work'] if where == 'last' else changed[targets[where]]
        if value is None:
            del target[key]
        else:
            target[key] = value
        rec.update(terminal=changed[-1], orders=changed[1:-1])
        text = '\n'.join(json.dumps(row) for row in changed)+'\n'+tail
        try:
            judge(text, rec, intent, plan)
        except (ValueError, KeyError) as error:
            require(str(error) == reason, 'v5_mutant_wrong_reason:'+name+':'+str(error))
            killed.append(name)
            continue
        raise ValueError('v5_mutant_survived:'+name)
    # Keep the completed work prefix, but deny the final forest after a
    # hypothetical allocation failure. This is metadata, not an executed fault.
    refused = copy.deepcopy(rows)
    refused[-2].update(outcome='resource_exhausted', reason='full_gabriel_allocation_failed', certificate_digest='',
        certificate_nodes=0, certificate_minima=0, certificate_parent_refs=0, terminal_roots=0, terminal_coverage_points=0)
    refused[-1].update(outcome='resource_exhausted', reason='full_gabriel_allocation_failed', terminal_status='failed',
        complete_requested_horizontal_orders=False, exit_code=2, completed_orders_diagnostic=len(rows)-3,
        last_stage='full', certificate_digest='')
    rec = copy.deepcopy(receipt)
    rec.update(terminal=refused[-1], orders=refused[1:-1], exit_code=2)
    refused_tail = 'Command exited with non-zero status 2\n'+tail.replace('Exit status: 0', 'Exit status: 2')
    text = '\n'.join(json.dumps(row) for row in refused)+'\n'+refused_tail
    require(judge(text, rec, intent, protocol)['attempt_success'] is False, 'v5_refusal_promoted')
    def scalar(p, pivots, certified, fallback, actual, legacy, geometry, outer):
        return dict(zip(MEB_FIELDS, (p, pivots, certified, fallback, actual)), meb_supports=legacy,
                    geometry_meb_calls=geometry, meb_calls=outer)
    positives = [(scalar(0,0,0,0,0,0,0,0), 0, True),
                 (scalar(0,0,0,1,11,11,1,1), 0, True),
                 (scalar(6,2,1,0,0,11,1,1), 6, True),
                 (scalar(6,2,1,0,0,1,1,1), 6, False),
                 (scalar(6,2,1,1,6,17,2,2), U64_MAX, True),
                 (scalar(U64_MAX,U64_MAX,0,U64_MAX,U64_MAX,U64_MAX,U64_MAX,U64_MAX), U64_MAX, True)]
    for row, cap, complete in positives:
        memory_meb_work(row, cap, complete)
    impossible = [('erased_A_after_F', scalar(1,0,0,1,0,7,1,1), 'meb_virtual_ordinal_interval'),
                  ('form_without_call', scalar(1,0,0,0,0,0,0,0), 'meb_forms_per_call'),
                  ('virtual_551', scalar(1,0,1,0,0,551,1,1), 'meb_virtual_ordinal_interval'),
                  ('virtual_undercharge', scalar(2,0,2,0,0,1,2,2), 'meb_virtual_ordinal_interval'),
                  ('forms_147', scalar(147,0,0,1,7,7,1,1), 'meb_forms_per_call')]
    for name, row, reason in impossible:
        try:
            memory_meb_work(row, U64_MAX, False)
        except ValueError as error:
            require(str(error) == reason, 'v5_scalar_wrong_reason:'+name)
            killed.append(name)
            continue
        raise ValueError('v5_scalar_survived:'+name)
    require(len(cases) == 35 and len(killed) == 40 and len(positives) == 6, 'v5_mutant_nonvacuum')
    return dict(audit_status='selftests_passed', supplied_positive=1, synthetic_refusal=1,
        meb_scalar_positive_models=6, models_are_engine_receipts=False, mutants_killed=killed,
        probe_schema=MEMORY_SCHEMA, limits_profile=LIMITS_PROFILE, successor_accounting=CURRENT_ACCOUNTING,
        meb_accounting=MEB_ACCOUNTING, meb_proposal_budget_kind=rows[0]['meb_proposal_budget_kind'],
        **{MEB_CAP_FIELD: rows[0][MEB_CAP_FIELD]})


def selftest(
    raw: str,
    receipt: JsonObject,
    intent: JsonObject,
    protocol: JsonObject,
) -> JsonObject:
    if loads(raw.splitlines()[0]).get('schema') == MEMORY_SCHEMA:
        return memory_selftest(raw, receipt, intent, protocol)
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
    if rows[0]['schema'] == MEB_SCHEMA and rows[0][MEB_CAP_FIELD] == 0:
        refused[-2]['meb_reference_supports'] = WORK['meb_supports']
        refused[-1]['last_order_work']['meb_reference_supports'] = WORK['meb_supports']
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
    current = rows[0]['schema'] != SCHEMA
    if current:
        mutations.update({
            'schema_unknown': 'schema',
            'schema_mixed_order': 'schema',
            'schema_mixed_terminal': 'schema',
            'accounting_missing_configuration': 'successor_accounting',
            'accounting_missing_order': 'successor_accounting',
            'accounting_missing_terminal': 'successor_accounting',
            'accounting_unknown': 'successor_accounting',
            'accounting_legacy': 'successor_accounting',
            'accounting_boolean': 'successor_accounting',
            'accounting_float': 'successor_accounting',
            'protocol_schema_missing': 'protocol_accounting',
            'protocol_schema_mismatch': 'protocol_accounting',
            'protocol_accounting_missing': 'protocol_accounting',
            'protocol_accounting_mismatch': 'protocol_accounting',
            'downgrade_schema_with_accounting': 'legacy_accounting_field',
            'downgrade_schema_no_accounting': 'protocol_accounting',
        })
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
        elif name == 'schema_unknown':
            for row in altered:
                row['schema'] = 'mhgp7-full-gabriel-probe-v999'
        elif name == 'schema_mixed_order':
            altered[1]['schema'] = SCHEMA
        elif name == 'schema_mixed_terminal':
            altered[-1]['schema'] = SCHEMA
        elif name.startswith('accounting_missing_'):
            index = {'configuration': 0, 'order': 1, 'terminal': -1}[
                name.removeprefix('accounting_missing_')
            ]
            del altered[index]['successor_accounting']
        elif name in ('accounting_unknown', 'accounting_legacy'):
            for row in altered:
                row['successor_accounting'] = (
                    LEGACY_ACCOUNTING if name == 'accounting_legacy' else 'unknown'
                )
        elif name in ('accounting_boolean', 'accounting_float'):
            altered[1]['successor_accounting'] = (
                True if name == 'accounting_boolean' else 2.0
            )
        elif name == 'protocol_schema_missing':
            del plan['probe_schema']
        elif name == 'protocol_schema_mismatch':
            plan['probe_schema'] = SCHEMA
        elif name == 'protocol_accounting_missing':
            del plan['successor_accounting']
        elif name == 'protocol_accounting_mismatch':
            plan['successor_accounting'] = LEGACY_ACCOUNTING
        elif name.startswith('downgrade_schema_'):
            for row in altered:
                row['schema'] = SCHEMA
                if name == 'downgrade_schema_no_accounting':
                    del row['successor_accounting']
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
    require(len(killed) == (35 if current else 19), 'mutant_nonvacuum')
    result = {
        'audit_status': 'selftests_passed',
        'real_positive': 1,
        'synthetic_refusal': 1,
        'synthetic_unemitted_read_refusal': 1,
        'mutants_killed': killed,
    }
    if current:
        result.update(probe_schema=rows[0]['schema'], successor_accounting=CURRENT_ACCOUNTING)
    if rows[0]['schema'] == MEB_SCHEMA:
        extra = meb_selftest(rows, receipt, intent, protocol, tail)
        result['mutants_killed'] += extra['mutants_killed']
        result['meb_scalar_positive_models'] = extra['scalar_positive_models']
        result['meb_models_are_engine_receipts'] = False
        result.update(meb_accounting=MEB_ACCOUNTING, **{MEB_CAP_FIELD: rows[0][MEB_CAP_FIELD]})
    return result


def meb_selftest(rows: list[JsonObject], receipt: JsonObject, intent: JsonObject,
                 protocol: JsonObject, tail: str) -> JsonObject:
    """Pure model gates; a coherent diagnostic is not an engine certificate."""
    killed = []
    mutations = []
    for index, label in ((0, 'configuration'), (1, 'order'), (-1, 'terminal')):
        mutations.append(('meb_missing_' + label, 'row', index,
                          'meb_accounting', None, 'meb_accounting'))
        mutations.append(('meb_wrong_' + label, 'row', index,
                          'meb_accounting', 'unknown', 'meb_accounting'))
    mutations.extend([
        ('meb_boolean', 'row', 1, 'meb_accounting', True, 'meb_accounting'),
        ('meb_protocol_missing', 'plan', 0, 'meb_accounting', None, 'protocol_meb_accounting'),
        ('meb_protocol_wrong', 'plan', 0, 'meb_accounting', 'unknown', 'protocol_meb_accounting'),
        ('meb_cap_missing', 'row', 0, MEB_CAP_FIELD, None, 'meb_proposal_cap'),
        ('meb_cap_order_missing', 'row', 1, MEB_CAP_FIELD, None, 'meb_cap_binding'),
        ('meb_cap_terminal_missing', 'row', -1, MEB_CAP_FIELD, None, 'meb_cap_binding'),
        ('meb_cap_float', 'row', 0, MEB_CAP_FIELD, float(rows[0][MEB_CAP_FIELD]), 'meb_proposal_cap'),
        ('meb_cap_boolean', 'row', 0, MEB_CAP_FIELD, False, 'meb_proposal_cap'),
        ('meb_cap_overflow', 'row', 0, MEB_CAP_FIELD, MAX_MEB_PROPOSAL_SUPPORTS + 1, 'meb_proposal_cap'),
        ('meb_order_cap_mismatch', 'row', 1, MEB_CAP_FIELD, rows[0][MEB_CAP_FIELD] + 1, 'meb_cap_binding'),
        ('meb_terminal_cap_mismatch', 'row', -1, MEB_CAP_FIELD, rows[0][MEB_CAP_FIELD] + 1, 'meb_cap_binding'),
    ])
    for key in MEB_FIELDS:
        mutations.append(('missing_' + key, 'row', 1, key, None, 'meb_work_integer'))
        mutations.append(('float_' + key, 'row', 1, key, float(rows[1][key]), 'meb_work_integer'))
    mutations.extend([
        ('meb_paid_over_cap', 'row', 1, 'meb_proposal_supports', rows[0][MEB_CAP_FIELD] + 1, 'meb_work_bounds'),
        ('meb_pivots_over_paid', 'row', 1, 'meb_proposal_pivots', rows[1]['meb_proposal_supports'] + 1, 'meb_work_bounds'),
        ('meb_certified_over_paid', 'row', 1, 'meb_proposal_certified', rows[1]['meb_proposal_supports'] + 1, 'meb_work_bounds'),
        ('meb_actual_over_legacy', 'row', 1, 'meb_reference_supports', rows[1]['meb_supports'] + 1, 'meb_work_bounds'),
        ('meb_dispatch_over_calls', 'row', 1, 'meb_proposal_fallback', rows[1]['geometry_meb_calls'] + 1, 'meb_work_bounds'),
        ('meb_last_inventory', 'last', 0, 'meb_proposal_supports', None, 'work_inventory'),
        ('meb_last_extra', 'last', 0, 'meb_proposal_unknown', 0, 'work_inventory'),
    ])
    for name, kind, index, key, value, reason in mutations:
        altered, rec, plan = copy.deepcopy(rows), copy.deepcopy(receipt), copy.deepcopy(protocol)
        target = plan if kind == 'plan' else (altered[-1]['last_order_work']
                                             if kind == 'last' else altered[index])
        if value is None:
            del target[key]
        else:
            target[key] = value
        rec.update(terminal=altered[-1], orders=altered[1:-1])
        changed = '\n'.join(json.dumps(row) for row in altered) + '\n' + tail
        try:
            judge(changed, rec, intent, plan)
        except ValueError as error:
            require(str(error) == reason, 'meb_mutant_wrong_reason:' + name + ':' + str(error))
            killed.append(name)
    for name in ('missing_p', 'duplicate_p', 'id_without_p', 'plan_without_p', 'plan_float_p'):
        rec, start, plan = copy.deepcopy(receipt), copy.deepcopy(intent), copy.deepcopy(protocol)
        if name in ('missing_p', 'duplicate_p'):
            argv = shlex.split(rec['command'])
            argv = argv[:-1] if name == 'missing_p' else argv + [argv[-1]]
            rec['command'] = start['command'] = shlex.join(argv)
            reason = 'command_binding'
        elif name == 'id_without_p':
            rec['id'] = start['id'] = rec['id'].rsplit('_p', 1)[0]
            reason = 'planned_attempt'
        else:
            for entry in plan['planned_sequence']:
                if name == 'plan_without_p':
                    entry.pop()
                else:
                    entry[-1] = float(entry[-1])
            reason = 'planned_attempt'
        try:
            judge('\n'.join(json.dumps(row) for row in rows) + '\n' + tail,
                  rec, start, plan)
        except ValueError as error:
            require(str(error) == reason, 'meb_mutant_wrong_reason:' + name)
            killed.append(name)
    # Explicitly bounded prospective prefixes, including an outer call charged
    # before entry to geometry. P and L remain independent; A counts actual F.
    def model(p: int, pivots: int, certified: int, fallback: int,
              actual: int, legacy: int, geometry: int, outer: int) -> JsonObject:
        return dict(zip(MEB_FIELDS, (p, pivots, certified, fallback, actual)),
                    meb_supports=legacy, geometry_meb_calls=geometry, meb_calls=outer)
    positives = [
        (model(0, 0, 0, 0, 0, 0, 0, 0), 0, True),
        (model(0, 0, 0, 2, 7, 7, 2, 2), 0, True),
        (model(2, 0, 2, 0, 0, 7, 2, 2), 9, True),
        (model(1, 0, 1, 1, 3, 7, 2, 2), 1, True),
        (model(1, 0, 1, 0, 0, 7, 1, 2), 1, False),
        (model(0, 0, 0, 0, 7, 7, 1, 1), 0, False),
        (model(3, 1, 0, 0, 0, 0, 0, 1), 3, False),
        (model(0, 0, 0, 0, 0, 0, 0, 0), MAX_MEB_PROPOSAL_SUPPORTS, True),
    ]
    for row, cap, complete in positives:
        meb_work(row, cap, complete)
    scalars = [
        ('paid_bool', 2, 'meb_proposal_supports', True, 'meb_work_integer'),
        ('paid_negative', 2, 'meb_proposal_supports', -1, 'meb_work_integer'),
        ('success_dispatch_deficit', 2, 'meb_proposal_certified', 1, 'meb_success_identity'),
        ('disabled_A_loss', 1, 'meb_reference_supports', 6, 'meb_disabled_reference'),
        ('disabled_fallback_loss', 1, 'meb_proposal_fallback', 1, 'meb_success_identity'),
        ('outer_call_underflow', 2, 'meb_calls', 1, 'meb_work_bounds'),
    ]
    for name, index, key, value, reason in scalars:
        row, cap, complete = copy.deepcopy(positives[index])
        row[key] = value
        try:
            meb_work(row, cap, complete)
        except ValueError as error:
            require(str(error) == reason, 'meb_scalar_wrong_reason:' + name)
            killed.append(name)
    require(len(mutations) == 34 and len(killed) == 45, 'meb_mutant_nonvacuum')
    return {'mutants_killed': killed, 'scalar_positive_models': len(positives)}


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
        re.fullmatch(r'n\d+_s\d+_k\d+_(?:eager|lazy)_c\d+(?:_p(?:\d+|unlimited))?', stem) is not None,
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
    if protocol.get('probe_schema') in MEMORY_SCHEMAS:
        require(is_digest(protocol.get('meb_header_sha256')) and sources['files'][
            'morsehgp3D_v7/src/forest/meb_proposal.hpp'
        ] == protocol['meb_header_sha256'], 'declared_meb_source_binding')
    if protocol.get('probe_schema') == MEMORY_SCHEMA:
        require(is_digest(protocol.get('limits_header_sha256')) and sources['files'][
            'morsehgp3D_v7/bench/full_gabriel_probe_limits.hpp'
        ] == protocol['limits_header_sha256'], 'declared_limits_source_binding')
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
