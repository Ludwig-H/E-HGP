#!/usr/bin/env python3
"""Read-only first-C supplement; the schema's frozen judge stays mandatory.

Usage: full_gabriel_cache_policy_audit.py [--selftest] PATH.receipt.json
Exit 0: coherent supplement (a refused attempt remains refused).
Exit 1: rejected data/selftest. Exit 2: invalid invocation.
No engine, geometry oracle, benchmark, source mutation or external write.
The v2 result is unchanged. V3/v4 require explicit reviewed judge pins, never
a hash learned from the current filesystem or accepted from the receipt alone.
V4 binds P and the five MEB diagnostics without promoting this supplement to
the primary receipt judge or a geometry/causal-work oracle.
V5 has a distinct memory/representation profile and stronger necessary MEB
diagnostic bounds. No historical v2/v3/v4 predicate or output is relabelled.
"""
from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import shlex
import sys
from typing import Any

SCHEMA = 'mhgp7-full-gabriel-probe-v2'
CURRENT_SCHEMA = 'mhgp7-full-gabriel-probe-v3'
MEB_SCHEMA = 'mhgp7-full-gabriel-probe-v4'
MEMORY_SCHEMA = 'mhgp7-full-gabriel-probe-v5'
LIMITS_PROFILE = 'memory_guarded_no_operation_quotas_v1'
U64_MAX = (1 << 64) - 1
MEB_SCHEMAS = (MEB_SCHEMA, MEMORY_SCHEMA)
MEB_ACCOUNTING = 'reference_ordinal_plus_native_z_q3_q4_proposal_v2'
MEB_CAP_FIELD = 'max_meb_proposal_supports_per_order'
MAX_MEB_PROPOSAL_SUPPORTS = 584000000
MEB_FIELDS = ('meb_proposal_supports meb_proposal_pivots meb_proposal_certified '
              'meb_proposal_fallback meb_reference_supports').split()
LEGACY_ACCOUNTING = 'full_successor_reads_writes_v1'
CURRENT_ACCOUNTING = 'full_successor_reads_writes_no_last_pair_v2'
JUDGE_SHA = (
    '8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef'
)
# Fixed reviewed authority by schema, not an auto-refreshing live-file hash.
# Updating the current judge requires an explicit source change and review here.
JUDGE_SHA_BY_SCHEMA = {
    SCHEMA: JUDGE_SHA,
    CURRENT_SCHEMA: '5de5b8d20d6073d5521a4217b8d504a59d255fad514eb8f4ddc257553ee0f032',
    MEB_SCHEMA: '475b92884d4e0aac5f9a2856ab841401ea1681a3477ea53599faa9a5140b3e11',
    MEMORY_SCHEMA: '040f738770ea2f141a2d0da80872c2a62118ad9d1e81dd6040066c10b8519a14',
}
POLICIES = {
    'eager': 'eager_all_incident_facets_v1',
    'lazy': 'lazy_first_c_strict_resolutions_v1',
}
AUTHORITY = (
    'full_horizontal_relative_to_supplied_complete_exact_regular_'
    'gabriel_catalogues'
)
Json = dict[str, Any]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def unique(pairs: list[tuple[str, Any]]) -> Json:
    output: Json = {}
    for key, value in pairs:
        require(key not in output, 'duplicate_key')
        output[key] = value
    return output


def loads(text: str) -> Any:
    return json.loads(
        text, object_pairs_hook=unique,
        parse_constant=lambda _: require(False, 'nonfinite'),
    )


def read(path: Path) -> bytes:
    with path.open('rb') as stream:
        data = stream.read(1048577)
    require(len(data) <= 1048576, 'file_size_budget')
    return data


def capacity(value: Any, ceiling: int = 1000000) -> int:
    require(type(value) is int and 0 <= value <= ceiling,
            'capacity_domain')
    return value


def check_lazy(row: Json, cap: int, complete: bool, capacity_ceiling: int = 1000000,
               counter_ceiling: int = 8000000) -> None:
    """Only successful rows satisfy first-C; refusal counters are prefixes.

    cache_inserts is charged before allocation. On a failed attempt it need
    not equal residence, and an unfinished portal need not insert or skip.
    """
    capacity(cap, capacity_ceiling)
    for name in ('portal_requests', 'cache_inserts', 'cache_skips',
                 'aliases', 'alias_hits'):
        require(type(row[name]) is int and 0 <= row[name] <= counter_ceiling,
                'counter_domain')
    portals = row['portal_requests']
    inserted = row['cache_inserts']
    skipped = row['cache_skips']
    require(row['aliases'] == row['alias_hits'] == 0,
            'lazy_historical_alias')
    require(inserted <= cap, 'capacity_bound')
    require(inserted + skipped <= portals, 'resolution_bound')
    if complete:
        require(inserted == min(cap, portals)
                and skipped == max(0, portals - cap), 'first_c_success')


def accounting_binding(rows: list[Json], protocol: Json | None) -> str:
    schema = rows[0]['schema']
    require(type(schema) is str and schema in JUDGE_SHA_BY_SCHEMA, 'schema')
    require(all(row['schema'] == schema for row in rows), 'schema')
    if schema == SCHEMA:
        require(all('successor_accounting' not in row for row in rows),
                'legacy_accounting_field')
    else:
        require(all(type(row.get('successor_accounting')) is str
                    and row['successor_accounting'] == CURRENT_ACCOUNTING
                    for row in rows), 'successor_accounting')
        require(type(protocol) is dict
                and protocol.get('probe_schema') == schema
                and protocol.get('successor_accounting') == CURRENT_ACCOUNTING,
                'protocol_accounting')
        require(protocol.get('judge_sha256') == JUDGE_SHA_BY_SCHEMA[schema],
                'required_judge_pin')
    if schema in MEB_SCHEMAS:
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
                    for row in rows) and (protocol is None or 'meb_accounting' not in protocol),
                'legacy_meb_fields')
    if schema == MEMORY_SCHEMA:
        require(all(row.get('limits_profile') == LIMITS_PROFILE for row in rows) and
                protocol.get('limits_profile') == LIMITS_PROFILE, 'limits_profile_binding')
        kind = rows[0].get('meb_proposal_budget_kind')
        require(type(kind) is str and kind in ('disabled', 'finite', 'unlimited') and
                ((kind == 'disabled' and cap == 0) or (kind == 'finite' and 0 < cap <= U64_MAX) or
                 (kind == 'unlimited' and cap == U64_MAX)), 'meb_budget_kind')
        require(all(row.get('meb_proposal_budget_kind') == kind for row in rows), 'meb_budget_kind_binding')
    else:
        require(all(not {'limits_profile', 'meb_proposal_budget_kind', 'work_counter_ceiling',
                         'storage_limit_kind'}.intersection(row) for row in rows) and
                (protocol is None or 'limits_profile' not in protocol), 'legacy_limits_fields')
    return schema


def check_meb(row: Json, cap: int, complete: bool) -> None:
    require(all(type(row.get(key)) is int and row[key] >= 0 for key in MEB_FIELDS),
            'meb_work_integer')
    p, pivots, certified, fallback, actual = (row[key] for key in MEB_FIELDS)
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


def check_memory_meb(row: Json, cap: int, complete: bool) -> None:
    check_meb(row, cap, complete)
    require(all(type(row[name]) is int and 0 <= row[name] <= U64_MAX
                for name in MEB_FIELDS + ['meb_supports', 'geometry_meb_calls', 'meb_calls']), 'meb_u64')
    require(row['meb_proposal_supports'] <= 146 * row['meb_calls'], 'meb_forms_per_call')
    virtual = row['meb_supports'] - row['meb_reference_supports']
    require(row['meb_proposal_certified'] <= virtual <= 550 * row['meb_proposal_certified'],
            'meb_virtual_ordinal_interval')


def memory_cache_ceiling(config: Json) -> int:
    require(config.get('storage_limit_kind') == 'per_named_logical_arena_and_representation' and
            config.get('work_counter_ceiling') == U64_MAX and config.get('named_payload_budget_bytes') == 8 << 30,
            'memory_storage_profile')
    size, sm, dm = (config[key] for key in ('sizeof_alias_entry_payload', 'storage_size_max', 'storage_difference_max'))
    require(type(size) is int and 0 < size <= 4096 and type(sm) is int and sm in ((1 << 32)-1, U64_MAX) and
            type(dm) is int and dm == sm // 2, 'memory_representation')
    ceiling = min((8 << 30) // size, sm // size, dm // size)
    require(type(config['max_cache_entries']) is int and config['max_cache_entries'] == ceiling,
            'memory_cache_ceiling')
    return ceiling


def judge(raw: bytes, receipt: Json, protocol: Json | None = None) -> Json:
    lines = raw.decode('utf-8').splitlines()
    end = next((i for i, line in enumerate(lines)
                if not line.startswith('{')), len(lines))
    rows = [loads(line) for line in lines[:end]]
    require(len(rows) >= 2 and rows[0]['type'] == 'configuration'
            and rows[-1]['type'] == 'terminal', 'terminal')
    require([row['type'] for row in rows] == (
        ['configuration'] + ['order'] * (len(rows) - 2) + ['terminal']
    ), 'row_inventory')
    config, terminal, orders = rows[0], rows[-1], rows[1:-1]
    require(receipt['terminal'] == terminal and receipt['orders'] == orders,
            'capture_binding')
    schema = accounting_binding(rows, protocol)
    memory = schema == MEMORY_SCHEMA
    policy = next((key for key, value in POLICIES.items()
                   if config['alias_policy'] == value), '')
    require(policy in POLICIES, 'policy_domain')
    ceiling = memory_cache_ceiling(config) if memory else 1000000
    cap = capacity(config['cache_entries'], ceiling)
    require(policy == 'lazy' or cap == 0, 'eager_capacity')
    require(config['max_aliases_per_order'] == (
        0 if policy == 'lazy' else ceiling if memory else 8000000
    ) and config['cache_policy'] == (
        'first_C_resolved_nonminimum_strict_facets'
        if policy == 'lazy' else 'not_applicable'
    ), 'policy_configuration')
    for row in (config, terminal):
        require(row['public_status'] == 'not_claimed'
                and row['authority'] == AUTHORITY, 'scope')
    for row in rows:
        require(row['alias_policy'] == POLICIES[policy]
                and type(row['cache_entries']) is int
                and row['cache_entries'] == cap, 'policy_binding')
    argv = shlex.split(receipt['command'])
    require([v for v in argv if v.startswith('--alias-policy=')]
            == [f'--alias-policy={policy}'], 'command_policy')
    require([v for v in argv if v.startswith('--cache-entries=')]
            == ([f'--cache-entries={cap}'] if policy == 'lazy' else []),
            'command_capacity')
    if schema in MEB_SCHEMAS:
        token = 'unlimited' if memory and config['meb_proposal_budget_kind'] == 'unlimited' else config[MEB_CAP_FIELD]
        require([v for v in argv if v.startswith('--meb-proposal-supports=')]
                == [f'--meb-proposal-supports={token}'],
                'command_meb_capacity')
    code = {
        'complete_relative': 0, 'resource_exhausted': 2,
        'unsupported_degeneracy': 2, 'invalid_input': 2,
        'invariant_violated': 3,
    }[terminal['outcome']]
    require(type(receipt['exit_code']) is int
            and receipt['exit_code'] == terminal['exit_code'] == code,
            'exit_binding')
    require(terminal['terminal_status'] == (
        'completed' if code == 0 else 'failed'
    ) and terminal['complete_requested_horizontal_orders'] is (code == 0),
            'terminal_status')
    require(code == 0 or terminal['certificate_digest'] == '',
            'refusal_promotion')
    require([row['k'] for row in orders] == list(range(1, len(orders) + 1)),
            'order_sequence')
    completed = checked = bounded = 0
    for index, row in enumerate(orders):
        require(row['provisional'] is True
                and row['whole_tower_authority'] is False, 'provisional')
        complete = row['outcome'] == 'complete_relative'
        if schema in MEB_SCHEMAS:
            (check_memory_meb if memory else check_meb)(row, config[MEB_CAP_FIELD], complete)
        if complete:
            completed += 1
        else:
            require(index == len(orders) - 1 and code != 0
                    and row['outcome'] == terminal['outcome'],
                    'refused_order_position')
        if policy == 'lazy':
            check_lazy(row, cap, complete, ceiling, U64_MAX if memory else 8000000)
            checked += int(complete)
            bounded += int(not complete)
    require(terminal['completed_orders_diagnostic'] == completed,
            'completed_prefix')
    if code == 0:
        require(completed == len(orders) == terminal['kmax_effective']
                and completed > 0, 'success_prefix')
    # This diagnostic may describe an unprinted failed order, e.g. a read
    # allocation failure. It receives only prefix bounds, never first-C.
    if policy == 'lazy':
        check_lazy(terminal['last_order_work'], cap, False, ceiling, U64_MAX if memory else 8000000)
    if schema in MEB_SCHEMAS:
        (check_memory_meb if memory else check_meb)(terminal['last_order_work'], config[MEB_CAP_FIELD], False)
    result = {
        'supplement_status': 'valid', 'attempt_success': code == 0,
        'alias_policy': policy, 'cache_entries': cap,
        'successful_lazy_orders_checked': checked,
        'refused_lazy_orders_bounded': bounded,
        'successful_orders_diagnostic': completed,
        'requires_frozen_v2_judge_sha256': JUDGE_SHA,
        'scope': 'first_C_counter_supplement_not_v2_replacement_or_oracle',
    }
    if schema != SCHEMA:
        del result['requires_frozen_v2_judge_sha256']
        result.update(
            probe_schema=schema, successor_accounting=CURRENT_ACCOUNTING,
            requires_frozen_judge_schema=schema,
            requires_frozen_judge_sha256=JUDGE_SHA_BY_SCHEMA[schema],
            scope=f'first_C_counter_supplement_not_{schema.rsplit("-", 1)[-1]}_replacement_or_oracle',
        )
    if schema in MEB_SCHEMAS:
        result.update(meb_accounting=MEB_ACCOUNTING,
                      **{MEB_CAP_FIELD: config[MEB_CAP_FIELD]})
    if memory:
        result.update(limits_profile=LIMITS_PROFILE, meb_proposal_budget_kind=config['meb_proposal_budget_kind'],
                      work_diagnostics='fresh_order_146_forms_and_1_to_550_virtual_supports_per_certificate')
    return result


def model(portals: Any, inserted: Any, skipped: Any) -> Json:
    return {
        'portal_requests': portals, 'cache_inserts': inserted,
        'cache_skips': skipped, 'aliases': 0, 'alias_hits': 0,
    }


def memory_selftest(raw: bytes, receipt: Json, protocol: Json) -> Json:
    real = judge(raw, receipt, protocol)
    require(real['attempt_success'], 'selftest_requires_real_success')
    rows = [loads(line) for line in raw.decode().splitlines() if line.startswith('{')]
    tail = b'\n'.join(line for line in raw.splitlines() if not line.startswith(b'{')) + b'\n'
    killed = []
    cases = [
        ('profile_config', 'row', 0, 'limits_profile', 'other', 'limits_profile_binding'),
        ('profile_order', 'row', 1, 'limits_profile', 'other', 'limits_profile_binding'),
        ('profile_terminal', 'row', -1, 'limits_profile', 'other', 'limits_profile_binding'),
        ('profile_protocol', 'plan', 0, 'limits_profile', 'other', 'limits_profile_binding'),
        ('kind_missing', 'row', 0, 'meb_proposal_budget_kind', None, 'meb_budget_kind'),
        ('kind_order', 'row', 1, 'meb_proposal_budget_kind', 'other', 'meb_budget_kind_binding'),
        ('kind_terminal', 'row', -1, 'meb_proposal_budget_kind', 'other', 'meb_budget_kind_binding'),
        ('capacity_overflow', 'row', 0, 'cache_entries', U64_MAX, 'capacity_domain'),
        ('cache_ceiling', 'row', 0, 'max_cache_entries', 1, 'memory_cache_ceiling'),
        ('storage_kind', 'row', 0, 'storage_limit_kind', 'global_RSS', 'memory_storage_profile'),
        ('payload_size_zero', 'row', 0, 'sizeof_alias_entry_payload', 0, 'memory_representation'),
        ('work_quota', 'row', 0, 'work_counter_ceiling', 4000000, 'memory_storage_profile'),
        ('judge_pin', 'plan', 0, 'judge_sha256', '0'*64, 'required_judge_pin'),
        ('schema_mixed', 'row', 1, 'schema', MEB_SCHEMA, 'schema'),
        ('work_bool', 'row', 1, 'meb_proposal_pivots', False, 'meb_work_integer'),
        ('work_float', 'row', 1, 'meb_reference_supports', 0.0, 'meb_work_integer'),
    ]
    for name, target_kind, index, key, value, reason in cases:
        changed, rec, plan = copy.deepcopy(rows), copy.deepcopy(receipt), copy.deepcopy(protocol)
        target = plan if target_kind == 'plan' else changed[index]
        if value is None:
            del target[key]
        else:
            target[key] = value
        rec.update(terminal=changed[-1], orders=changed[1:-1])
        text = b'\n'.join(json.dumps(row).encode() for row in changed)+b'\n'+tail
        try:
            judge(text, rec, plan)
        except ValueError as error:
            require(str(error) == reason, 'v5_wrong_mutant_reason:'+name+':'+str(error))
            killed.append(name)
            continue
        raise ValueError('v5_mutant_survived:'+name)
    positive_first_c = [(0,0,0,0), (0,3,0,3), (1,1,1,0), (1,3,1,2),
                        (3,2,2,0), (3,3,3,0), (3,5,3,2), (1,U64_MAX,1,U64_MAX-1)]
    for cap, portals, inserted, skipped in positive_first_c:
        check_lazy(model(portals, inserted, skipped), cap, True, U64_MAX, U64_MAX)
    for name, cap, row, reason in [('underfill', 3, model(2,1,1), 'first_c_success'),
            ('overflow_counter', 1, model(U64_MAX+1,1,U64_MAX), 'counter_domain'),
            ('double_resolution', 1, model(1,1,1), 'resolution_bound'),
            ('capacity', 0, model(1,1,0), 'capacity_bound')]:
        try:
            check_lazy(row, cap, True, U64_MAX, U64_MAX)
        except ValueError as error:
            require(str(error) == reason, 'v5_first_c_wrong_reason:'+name)
            killed.append(name)
            continue
        raise ValueError('v5_first_c_survived:'+name)
    def scalar(p, pivots, cert, fallback, actual, legacy, geometry, outer):
        return dict(zip(MEB_FIELDS, (p,pivots,cert,fallback,actual)), meb_supports=legacy,
                    geometry_meb_calls=geometry, meb_calls=outer)
    positives = [(scalar(0,0,0,0,0,0,0,0),0,True), (scalar(0,0,0,1,11,11,1,1),0,True),
                 (scalar(6,2,1,0,0,11,1,1),6,True), (scalar(6,2,1,0,0,1,1,1),6,False),
                 (scalar(U64_MAX,U64_MAX,0,U64_MAX,U64_MAX,U64_MAX,U64_MAX,U64_MAX),U64_MAX,True)]
    for row, cap, complete in positives:
        check_memory_meb(row, cap, complete)
    for name, row, reason in [
            ('erased_A_after_F', scalar(1,0,0,1,0,7,1,1), 'meb_virtual_ordinal_interval'),
            ('form_without_call', scalar(1,0,0,0,0,0,0,0), 'meb_forms_per_call'),
            ('virtual_551', scalar(1,0,1,0,0,551,1,1), 'meb_virtual_ordinal_interval'),
            ('virtual_undercharge', scalar(2,0,2,0,0,1,2,2), 'meb_virtual_ordinal_interval'),
            ('forms_147', scalar(147,0,0,1,7,7,1,1), 'meb_forms_per_call')]:
        try:
            check_memory_meb(row, U64_MAX, False)
        except ValueError as error:
            require(str(error) == reason, 'v5_meb_wrong_reason:'+name)
            killed.append(name)
            continue
        raise ValueError('v5_meb_survived:'+name)
    require(len(killed) == 25 and len(positive_first_c) == 8 and len(positives) == 5, 'v5_nonvacuum')
    return dict(supplement_status='selftests_passed', supplied_positive=1, scalar_first_c_models=8,
        scalar_meb_models=5, mutants_killed=killed, models_are_engine_receipts=False,
        probe_schema=MEMORY_SCHEMA, limits_profile=LIMITS_PROFILE, meb_accounting=MEB_ACCOUNTING,
        successor_accounting=CURRENT_ACCOUNTING,
        requires_frozen_judge_sha256=JUDGE_SHA_BY_SCHEMA[MEMORY_SCHEMA],
        meb_proposal_budget_kind=rows[0]['meb_proposal_budget_kind'], **{MEB_CAP_FIELD: rows[0][MEB_CAP_FIELD]})


def selftest(raw: bytes, receipt: Json, protocol: Json | None = None) -> Json:
    if loads(raw.decode().splitlines()[0]).get('schema') == MEMORY_SCHEMA:
        return memory_selftest(raw, receipt, protocol)
    real = judge(raw, receipt, protocol)
    require(real['attempt_success'], 'selftest_requires_real_success')
    positives = [
        (0, 0, 0, 0), (0, 3, 0, 3), (1, 0, 0, 0),
        (1, 1, 1, 0), (1, 3, 1, 2), (3, 2, 2, 0),
        (3, 3, 3, 0), (3, 5, 3, 2), (1000000, 1000000, 1000000, 0),
    ]
    for cap, portals, inserted, skipped in positives:
        check_lazy(model(portals, inserted, skipped), cap, True)
    # Prefix before first allocation, admitted allocation then failure, and
    # one completed resolution followed by an unfinished second portal.
    for row in (model(1, 0, 0), model(1, 1, 0), model(2, 1, 0)):
        check_lazy(row, 1, False)
    mutations = [
        ('C1_P1_I0_S1', 1, model(1, 0, 1), 'first_c_success'),
        ('underfill', 3, model(2, 1, 1), 'first_c_success'),
        ('all_skipped', 1, model(3, 0, 3), 'first_c_success'),
        ('insert_with_C0', 0, model(1, 1, 0), 'capacity_bound'),
        ('double_resolution', 1, model(1, 1, 1), 'resolution_bound'),
        ('negative_insert', 1, model(1, -1, 2), 'counter_domain'),
        ('float_portals', 1, model(1.0, 1, 0), 'counter_domain'),
        ('boolean_insert', 1, model(1, True, 0), 'counter_domain'),
        ('capacity_over_limit', 1000001, model(1, 1, 0), 'capacity_domain'),
    ]
    alias = model(1, 1, 0)
    alias['aliases'] = 1
    mutations.append(('historical_alias', 1, alias, 'lazy_historical_alias'))
    killed = []
    for name, cap, row, reason in mutations:
        try:
            check_lazy(row, cap, True)
        except ValueError as error:
            require(str(error) == reason, 'wrong_mutant_reason:' + name)
            killed.append(name)
    # Raw/mirror and policy binding mutations use the actual supplied bytes;
    # the scalar models above are explicitly not fabricated engine receipts.
    for name, reason in (('mirror', 'capture_binding'),
                         ('policy', 'policy_binding')):
        rec = copy.deepcopy(receipt)
        rec['terminal']['cache_entries'] += 1
        altered = raw
        if name == 'policy':
            altered = b'\n'.join(
                json.dumps(rec['terminal']).encode()
                if line.startswith(b'{') and loads(line.decode()).get('type')
                == 'terminal' else line
                for line in raw.splitlines()
            ) + b'\n'
        try:
            judge(altered, rec, protocol)
        except ValueError as error:
            require(str(error) == reason, 'wrong_mutant_reason:' + name)
            killed.append(name)
    require(len(positives) == 9 and len(killed) == 12,
            'selftest_nonvacuum')
    result = {
        'supplement_status': 'selftests_passed', 'real_positive': 1,
        'scalar_success_models': 9, 'scalar_refusal_prefix_models': 3,
        'mutants_killed': killed,
        'models_are_engine_receipts': False,
        'scope': 'first_C_counter_supplement_not_v2_replacement_or_oracle',
    }
    if real.get('probe_schema') in (CURRENT_SCHEMA, MEB_SCHEMA):
        rows = [loads(line) for line in raw.decode('utf-8').splitlines()
                if line.startswith('{')]
        tail = b'\n'.join(line for line in raw.splitlines()
                          if not line.startswith(b'{')) + b'\n'
        mutations = {
            'accounting_missing_configuration': 'successor_accounting',
            'accounting_missing_order': 'successor_accounting',
            'accounting_missing_terminal': 'successor_accounting',
            'accounting_unknown': 'successor_accounting',
            'accounting_legacy': 'successor_accounting',
            'accounting_boolean': 'successor_accounting',
            'schema_unknown': 'schema',
            'schema_mixed': 'schema',
            'protocol_accounting_mismatch': 'protocol_accounting',
            'protocol_missing': 'protocol_accounting',
            'judge_pin_changed': 'required_judge_pin',
            'judge_pin_legacy': 'required_judge_pin',
            'judge_pin_missing': 'required_judge_pin',
            'judge_pin_boolean': 'required_judge_pin',
            'downgrade_schema_with_accounting': 'legacy_accounting_field',
        }
        for name, reason in mutations.items():
            altered = copy.deepcopy(rows)
            rec, plan = copy.deepcopy(receipt), copy.deepcopy(protocol)
            if name.startswith('accounting_missing_'):
                index = {'configuration': 0, 'order': 1, 'terminal': -1}[
                    name.removeprefix('accounting_missing_')
                ]
                del altered[index]['successor_accounting']
            elif name in ('accounting_unknown', 'accounting_legacy'):
                for row in altered:
                    row['successor_accounting'] = (
                        LEGACY_ACCOUNTING if name == 'accounting_legacy' else 'unknown'
                    )
            elif name == 'accounting_boolean':
                altered[1]['successor_accounting'] = True
            elif name == 'schema_unknown':
                for row in altered:
                    row['schema'] = 'mhgp7-full-gabriel-probe-v999'
            elif name == 'schema_mixed':
                altered[-1]['schema'] = SCHEMA
            elif name == 'protocol_accounting_mismatch':
                plan['successor_accounting'] = LEGACY_ACCOUNTING
            elif name == 'protocol_missing':
                plan = None
            elif name == 'judge_pin_changed':
                pin = plan['judge_sha256']
                plan['judge_sha256'] = ('0' if pin[0] != '0' else '1') + pin[1:]
            elif name == 'judge_pin_legacy':
                plan['judge_sha256'] = JUDGE_SHA
            elif name == 'judge_pin_missing':
                del plan['judge_sha256']
            elif name == 'judge_pin_boolean':
                plan['judge_sha256'] = True
            elif name == 'downgrade_schema_with_accounting':
                for row in altered:
                    row['schema'] = SCHEMA
            rec.update(terminal=altered[-1], orders=altered[1:-1])
            changed = (b'\n'.join(json.dumps(row).encode() for row in altered)
                       + b'\n' + tail)
            try:
                judge(changed, rec, plan)
            except ValueError as error:
                require(str(error) == reason, 'wrong_mutant_reason:' + name)
                killed.append(name)
        require(len(killed) == 27, 'version_selftest_nonvacuum')
        result.update(
            probe_schema=real['probe_schema'], successor_accounting=CURRENT_ACCOUNTING,
            requires_frozen_judge_schema=real['probe_schema'],
            requires_frozen_judge_sha256=JUDGE_SHA_BY_SCHEMA[real['probe_schema']],
            scope=real['scope'],
        )
    if real.get('probe_schema') == MEB_SCHEMA:
        result['mutants_killed'] += meb_selftest(raw, receipt, protocol)
        result.update(meb_accounting=MEB_ACCOUNTING,
                      **{MEB_CAP_FIELD: real[MEB_CAP_FIELD]})
    return result


def meb_selftest(raw: bytes, receipt: Json, protocol: Json) -> list[str]:
    rows = [loads(line) for line in raw.decode().splitlines() if line.startswith('{')]
    tail = b'\n'.join(line for line in raw.splitlines() if not line.startswith(b'{')) + b'\n'
    mutations = [
        ('meb_missing_config', 'row', 0, 'meb_accounting', None, 'meb_accounting'),
        ('meb_missing_order', 'row', 1, 'meb_accounting', None, 'meb_accounting'),
        ('meb_missing_terminal', 'row', -1, 'meb_accounting', None, 'meb_accounting'),
        ('meb_wrong', 'row', 1, 'meb_accounting', 'unknown', 'meb_accounting'),
        ('meb_boolean', 'row', 1, 'meb_accounting', True, 'meb_accounting'),
        ('meb_protocol_missing', 'plan', 0, 'meb_accounting', None, 'protocol_meb_accounting'),
        ('meb_protocol_wrong', 'plan', 0, 'meb_accounting', 'unknown', 'protocol_meb_accounting'),
        ('meb_cap_missing', 'row', 0, MEB_CAP_FIELD, None, 'meb_proposal_cap'),
        ('meb_cap_float', 'row', 0, MEB_CAP_FIELD, float(rows[0][MEB_CAP_FIELD]), 'meb_proposal_cap'),
        ('meb_cap_boolean', 'row', 0, MEB_CAP_FIELD, False, 'meb_proposal_cap'),
        ('meb_cap_overflow', 'row', 0, MEB_CAP_FIELD, MAX_MEB_PROPOSAL_SUPPORTS + 1, 'meb_proposal_cap'),
        ('meb_order_cap', 'row', 1, MEB_CAP_FIELD, rows[0][MEB_CAP_FIELD] + 1, 'meb_cap_binding'),
        ('meb_terminal_cap', 'row', -1, MEB_CAP_FIELD, rows[0][MEB_CAP_FIELD] + 1, 'meb_cap_binding'),
        ('meb_work_missing', 'row', 1, MEB_FIELDS[0], None, 'meb_work_integer'),
        ('meb_work_float', 'row', 1, MEB_FIELDS[1], 0.0, 'meb_work_integer'),
        ('meb_work_boolean', 'row', 1, MEB_FIELDS[2], False, 'meb_work_integer'),
        ('meb_paid_over_cap', 'row', 1, MEB_FIELDS[0], rows[0][MEB_CAP_FIELD] + 1, 'meb_work_bounds'),
        ('meb_actual_over_legacy', 'row', 1, MEB_FIELDS[4], rows[1]['meb_supports'] + 1, 'meb_work_bounds'),
    ]
    killed = []
    for name, kind, index, key, value, reason in mutations:
        altered, rec, plan = copy.deepcopy(rows), copy.deepcopy(receipt), copy.deepcopy(protocol)
        target = plan if kind == 'plan' else altered[index]
        if value is None:
            del target[key]
        else:
            target[key] = value
        rec.update(terminal=altered[-1], orders=altered[1:-1])
        changed = b'\n'.join(json.dumps(row).encode() for row in altered) + b'\n' + tail
        try:
            judge(changed, rec, plan)
        except ValueError as error:
            require(str(error) == reason, 'meb_wrong_mutant_reason:' + name + ':' + str(error))
            killed.append(name)
    for name in ('missing_p', 'duplicate_p'):
        rec = copy.deepcopy(receipt)
        argv = shlex.split(rec['command'])
        rec['command'] = shlex.join(argv[:-1] if name == 'missing_p' else argv + [argv[-1]])
        try:
            judge(raw, rec, protocol)
        except ValueError as error:
            require(str(error) == 'command_meb_capacity', 'meb_wrong_mutant_reason:' + name)
            killed.append(name)
    require(len(killed) == 20, 'meb_selftest_nonvacuum')
    return killed


def main(argv: list[str]) -> int:
    if (len(argv) not in (1, 2)
            or (len(argv) == 2 and argv[0] != '--selftest')
            or argv[-1].startswith('--')):
        print(json.dumps({'supplement_status': 'invalid_arguments'}))
        return 2
    path = Path(argv[-1])
    require(path.name.endswith('.receipt.json'), 'receipt_path')
    raw_path = path.with_name(
        path.name.removesuffix('.receipt.json') + '.raw.txt'
    )
    raw, metadata = read(raw_path), read(path)
    receipt = loads(metadata.decode('utf-8'))
    # Legacy invocation still reads exactly its historical pair of files.
    # The schema's primary judge remains mandatory; this only binds its pin.
    config = loads(raw.decode('utf-8').splitlines()[0])
    require(type(config) is dict, 'configuration')
    protocol = (loads(read(path.parent / 'protocol.json').decode('utf-8'))
                if config.get('schema') in (CURRENT_SCHEMA, MEB_SCHEMA, MEMORY_SCHEMA) else None)
    stream = receipt['streams'][raw_path.name]
    raw_sha = hashlib.sha256(raw).hexdigest()
    require(stream['bytes'] == len(raw) and stream['sha256'] == raw_sha,
            'raw_stream_binding')
    output = (selftest(raw, receipt, protocol) if len(argv) == 2
              else judge(raw, receipt, protocol))
    output.update(raw_sha256=raw_sha,
                  receipt_sha256=hashlib.sha256(metadata).hexdigest())
    print(json.dumps(output, sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except (ValueError, KeyError, TypeError, OSError, IndexError,
            OverflowError, RecursionError) as error:
        print(json.dumps(
            {'supplement_status': 'invalid', 'reason': str(error)},
            sort_keys=True,
        ))
        sys.exit(1)
