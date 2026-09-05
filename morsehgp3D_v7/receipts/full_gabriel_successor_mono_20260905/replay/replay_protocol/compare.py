#!/usr/bin/env python3
"""Read-only historical FULL comparison, never an appaired timing claim.

Only complete per-order observations support S_new = S_old - 2*A. A global
refusal is accepted solely in the explicit successful-prefix-diagnostic scope.
All inputs are bounded and pinned; no engine, compiler, subprocess, publication
or source mutation is performed. The pure receipt judge is reused under a
closed source pin, not imported from an unreviewed live module.

The measure exclusion inventory is explicitly ported from singleton capture
910a67bfb014ca0a213483046430a9ca1d29f49b8117c7db4979cd4a9ff9f05f.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import re
import sys
import types

ROOT = Path('/workspaces/E-HGP')
PRODUCER = 'morsehgp3D_v7/src/forest/full_gabriel.hpp'
PROBE = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp'
DIGEST = 'morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp'
JUDGE = ROOT / 'morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py'
JUDGE_SHA = '5de5b8d20d6073d5521a4217b8d504a59d255fad514eb8f4ddc257553ee0f032'
OLD = {
    'singleton21b77': (
        '21b77d29a4ba2bca453b602a8faa4564a978f4ba71af5167c164faae4ef0e1a5',
        '57c598bfea861166bd8a58311addc5d120d684081e80dbac295b4f4a653aa5b8'),
    'lazy13c6': (
        '13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627',
        '1d5a38cea99555fd2db474ee43aff6ba1ee708208508cfa97c540774d0bb7e78'),
}
OLD_PROBE = 'f21e3c70bda4cc9adaa0960ed19c9a6a8aa6b09413f6c6d63bae884c98e9486a'
OLD_JUDGE = '8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef'
SCHEMAS = ('mhgp7-full-gabriel-probe-v2', 'mhgp7-full-gabriel-probe-v3')
ACCOUNTING = ('full_successor_reads_writes_v1', 'full_successor_reads_writes_no_last_pair_v2')
AUTH = 'full_horizontal_relative_to_supplied_complete_exact_regular_gabriel_catalogues'
ORDER_MEASURES = set('build_ms digest_ms expand_ms read_ms release_ms rss_mib_sample hwm_mib_sample'.split())
TERMINAL_MEASURES = set(('compute_read_release_ms_subtracted_diagnostic digest_ms '
    'elapsed_before_terminal_ms generation_rects_ms generation_wspd_ms provisional_output_ms '
    'rss_mib_sample hwm_mib_sample stage_ms').split())
WORK = set(('input_records aliases face_visits alias_hits portal_requests chain_steps terminal_direct '
    'max_chain_length normalized_anchors successor_steps no_op_connections meb_calls geometry_meb_calls '
    'meb_supports query_nodes query_leaves query_range_skips minimum_lookups minimum_hits cache_lookups '
    'cache_hits cache_inserts cache_skips singleton_intruder_resolutions direct_lookups').split())
ORDER_COUNTS = WORK | set(('k cache_entries certificate_minima certificate_nodes certificate_parent_refs '
    'connection_catalogue_records minimum_catalogue_records terminal_coverage_points terminal_roots').split())
COMMON_STRINGS = set(('schema type alias_policy authority public_status input_digest input_digest_kind '
    'certificate_digest certificate_digest_kind s_comparison_scope successor_accounting').split())
CONFIG_STRINGS = COMMON_STRINGS - {'input_digest', 'certificate_digest'} | set(('phase backend profile '
    'mode family input_generator cache_policy digest_scratch_scope digest_timing_scope read_kind').split())
CONFIG_FLAGS = {'gpu', 'archive', 'vertical'}
ORDER_STRINGS = set('schema type alias_policy certificate_digest outcome reason successor_accounting'.split())
ORDER_FLAGS = {'provisional', 'whole_tower_authority'}
TERMINAL_STRINGS = COMMON_STRINGS | set(('outcome reason last_stage reference_timing '
    'subtracted_timing_scope terminal_status').split())
TERMINAL_FLAGS = set(('certificate_retained complete_requested_horizontal_orders '
    'digest_proves_catalogue_completeness frontier_ledger_closed integrated_inter_k_tower '
    'rank_window_regular terminal_root_coverage_proves_equality').split())
PREFIX_TERMINAL_EXCLUSIONS = set(('certificate_digest complete_requested_horizontal_orders '
    'completed_orders_diagnostic exit_code last_order last_order_work last_stage outcome reason terminal_status').split())
MAX_JSON = 2 << 20
MAX_BINARY = 32 << 20
MAX_WATCHED_BYTES = 96 << 20


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def is_sha(value: object) -> bool:
    return type(value) is str and re.fullmatch('[0-9a-f]{64}', value) is not None


def unsigned(value: object, name: str) -> None:
    require(type(value) is int and 0 <= value < 1 << 64, 'integer_count:' + name)


def unique(pairs: list) -> dict:
    out = {}
    for key, value in pairs:
        require(key not in out, 'duplicate_json_key')
        out[key] = value
    return out


def loads(raw: bytes | str) -> object:
    def constant(_: str) -> None:
        require(False, 'nonfinite_json')
    value = json.loads(raw, object_pairs_hook=unique, parse_constant=constant)
    remaining = 100000
    def bounded(item: object, depth: int) -> None:
        nonlocal remaining
        remaining -= 1
        require(depth <= 64 and remaining >= 0, 'json_shape_budget')
        if type(item) is dict:
            for entry in item.values():
                bounded(entry, depth + 1)
        elif type(item) is list:
            for entry in item:
                bounded(entry, depth + 1)
        elif type(item) is float:
            require(math.isfinite(item), 'nonfinite_json')
    bounded(value, 0)
    return value


def canonical(value: object) -> str:
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


def equal(a: object, b: object, reason: str) -> None:
    # Do not identify Python's True, 1 and 1.0 in source or counter comparisons.
    require(canonical(a) == canonical(b), reason)


def relative(name: str) -> str:
    require(type(name) is str and name and str(PurePosixPath(name)) == name
            and not name.startswith('/') and '..' not in PurePosixPath(name).parts,
            'unsafe_relative_path')
    return name


class Reader:
    def __init__(self) -> None:
        self.watches: dict[Path, tuple[str, int]] = {}
        self.total = 0

    def read(self, path: Path, pin: str | None = None, limit: int = MAX_JSON) -> bytes:
        require(path.is_absolute() and path.is_relative_to(ROOT), 'input_scope')
        for component in (path, *path.parents):
            if component == ROOT.parent:
                break
            require(not component.is_symlink(), 'input_symlink')
        require(path.is_file() and path.stat().st_size <= limit, 'input_size_or_kind')
        with path.open('rb') as stream:
            raw = stream.read(limit + 1)
        require(len(raw) <= limit and (pin is None or is_sha(pin) and sha(raw) == pin), 'input_pin_or_size')
        stamp = (sha(raw), len(raw))
        require(path not in self.watches or self.watches[path] == stamp, 'input_drift')
        if path not in self.watches:
            self.total += len(raw)
            require(self.total <= MAX_WATCHED_BYTES, 'total_input_budget')
        self.watches[path] = stamp
        return raw

    def argument(self, argument: str) -> tuple[Path, bytes]:
        written, separator, pin = argument.rpartition('=')
        require(separator and is_sha(pin), 'explicit_PATH_SHA256_required')
        path = Path(written)
        require(path == path.absolute() and '..' not in path.parts, 'absolute_canonical_input_required')
        return path, self.read(path, pin)

    def stable(self) -> None:
        for path, (pin, size) in list(self.watches.items()):
            require(len(self.read(path, pin, max(MAX_JSON, size))) == size, 'final_input_drift')


def typed_row(row: dict, kind: str, arm: int) -> None:
    require(type(row) is dict and row.get('type') == kind and row.get('schema') == SCHEMAS[arm], 'row_schema')
    if arm == 0:
        require('successor_accounting' not in row, 'legacy_calendar_must_be_implicit_v1')
    else:
        require(row.get('successor_accounting') == ACCOUNTING[1], 'current_calendar_required')
    strings = CONFIG_STRINGS if kind == 'configuration' else ORDER_STRINGS if kind == 'order' else TERMINAL_STRINGS
    flags = CONFIG_FLAGS if kind == 'configuration' else ORDER_FLAGS if kind == 'order' else TERMINAL_FLAGS
    measures = set() if kind == 'configuration' else ORDER_MEASURES if kind == 'order' else TERMINAL_MEASURES
    require((strings - {'successor_accounting'}) | flags | measures <= row.keys(), 'row_inventory')
    if kind == 'order':
        require(set(row) == ORDER_COUNTS | ORDER_MEASURES | (ORDER_STRINGS - {'successor_accounting'}) |
                ORDER_FLAGS | ({'successor_accounting'} if arm else set()), 'order_inventory')
    for key, value in row.items():
        if key in strings:
            require(type(value) is str, 'string_field:' + key)
        elif key in flags:
            require(type(value) is bool, 'boolean_field:' + key)
        elif key in measures:
            values = value.values() if key == 'stage_ms' and type(value) is dict else [value]
            require(all(type(v) in (int, float) and math.isfinite(v) and v >= 0 for v in values), 'measure_type:' + key)
        elif key == 'last_order_work' and kind == 'terminal':
            require(type(value) is dict and set(value) == WORK | {'diagnostic_only'}
                    and value['diagnostic_only'] is True, 'last_work_inventory')
            for field in WORK:
                unsigned(value[field], field)
        else:
            unsigned(value, key)
    require(WORK <= row.keys() if kind == 'order' else True, 'work_inventory')


def unversioned(row: dict, measures: set[str]) -> dict:
    return {key: value for key, value in row.items()
            if key not in measures | {'schema', 'successor_accounting'}}


def relation(old: dict, new: dict) -> dict:
    for field in WORK:
        unsigned(old[field], field)
        unsigned(new[field], field)
    require(old['normalized_anchors'] == new['normalized_anchors'], 'anchor_count_differs')
    saving = 2 * old['normalized_anchors']
    require(old['successor_steps'] >= saving and new['successor_steps'] == old['successor_steps'] - saving,
            'successor_success_identity')
    transformed = copy.deepcopy(old)
    transformed['successor_steps'] = new['successor_steps']
    equal(transformed, new, 'other_nonmeasure_fields_differ')
    return {'old_steps': old['successor_steps'], 'new_steps': new['successor_steps'],
            'normalized_anchors': old['normalized_anchors'], 'saved_steps': saving}


def coherent_model(config: dict, record: dict, arm: int) -> None:
    typed_row(config, 'configuration', arm)
    require(type(record) is dict and record.get('status') == 'completed'
            and record.get('error') is None, 'closed_transport_required')
    unsigned(record['exit_code'], 'engine_exit_code')
    require(type(record['orders']) is list and len(record['orders']) <= 10, 'order_count_budget')
    terminal = record['terminal']
    typed_row(terminal, 'terminal', arm)
    require(record['exit_code'] == terminal['exit_code'], 'terminal_exit_binding')
    for index, row in enumerate(record['orders']):
        typed_row(row, 'order', arm)
        require(row['k'] == index + 1, 'sequential_orders')
    complete = record['exit_code'] == 0
    require(terminal['complete_requested_horizontal_orders'] is complete
            and terminal['terminal_status'] == ('completed' if complete else 'failed'), 'success_promotion')
    succeeded = [row for row in record['orders'] if row['outcome'] == 'complete_relative']
    require(terminal['completed_orders_diagnostic'] == len(succeeded), 'completed_order_count')
    if complete:
        require(len(succeeded) == len(record['orders']) == config['kmax_effective']
                and terminal['outcome'] == 'complete_relative' and terminal['reason'] == AUTH
                and is_sha(terminal['certificate_digest']), 'whole_requested_completion')
    else:
        require(terminal['certificate_digest'] == '', 'refusal_global_digest')
    require(is_sha(terminal['input_digest']), 'input_digest')
    for row in succeeded:
        require(is_sha(row['certificate_digest']) and row['reason'] == AUTH
                and row['provisional'] is True and row['whole_tower_authority'] is False,
                'successful_order_scope')
    if record['orders'] and terminal['last_order'] == record['orders'][-1]['k']:
        equal({key: terminal['last_order_work'][key] for key in WORK},
              {key: record['orders'][-1][key] for key in WORK}, 'last_work_binding')


def compare_objects(old_config: dict, old: dict, new_config: dict, new: dict,
                    scope: str, prefix: int | None = None) -> dict:
    coherent_model(old_config, old, 0)
    coherent_model(new_config, new, 1)
    equal(unversioned(old_config, set()), unversioned(new_config, set()), 'configuration_differs')
    require(old['terminal']['input_digest'] == new['terminal']['input_digest'], 'input_digest_differs')
    whole = scope == 'complete'
    require(whole or scope == 'successful-prefix-diagnostic', 'scope_required')
    if whole:
        require(prefix is None and old['exit_code'] == new['exit_code'] == 0, 'global_completeness_required')
        count = old_config['kmax_effective']
    else:
        require(type(prefix) is int and 1 <= prefix <= min(len(old['orders']), len(new['orders'])), 'explicit_prefix_required')
        count = prefix
    compared = []
    for left, right in zip(old['orders'][:count], new['orders'][:count]):
        require(left['outcome'] == right['outcome'] == 'complete_relative', 'refused_order_not_comparable')
        delta = relation(unversioned(left, ORDER_MEASURES), unversioned(right, ORDER_MEASURES))
        compared.append({'k': left['k'], 'certificate_digest': left['certificate_digest'], **delta})
    require(len(compared) == count and count > 0, 'comparison_nonvacuum')
    ta, tb = old['terminal'], new['terminal']
    last_checked = whole or (ta['last_order'] == tb['last_order'] and 1 <= ta['last_order'] <= count)
    if last_checked:
        relation(ta['last_order_work'], tb['last_order_work'])
    exclude = TERMINAL_MEASURES | (set() if whole else PREFIX_TERMINAL_EXCLUSIONS)
    a, b = unversioned(ta, exclude), unversioned(tb, exclude)
    if whole:
        a = copy.deepcopy(a)
        a['last_order_work']['successor_steps'] = b['last_order_work']['successor_steps']
    equal(a, b, 'terminal_nonmeasure_fields_differ')
    return {
        'schema': 'mhgp7-successor-historical-functional-comparison-v1', 'comparison_status': 'valid',
        'scope': scope, 'global_complete_comparison': whole, 'prefix_is_not_global_success': not whole,
        'old_global_exit_code': old['exit_code'], 'new_global_exit_code': new['exit_code'],
        'old_global_outcome': ta['outcome'], 'new_global_outcome': tb['outcome'],
        'old_producer_accounting': ACCOUNTING[0], 'new_producer_accounting': ACCOUNTING[1],
        'orders_compared': count, 'orders': compared, 'last_order_work_relation_checked': last_checked,
        'reported_work_fields_compared_except_successor_steps': len(WORK) - 1,
        'unserialized_geometry_fields_not_compared': 8, 'all_other_scoped_nonmeasure_fields_equal': True,
        'input_digest': ta['input_digest'],
        'global_certificate_digest': ta['certificate_digest'] if whole else None,
        'historical_timing_pair_claim': False, 'timing_ratio_computed': False,
        'geometry_or_catalogue_completeness_proved': False, 'integrated_inter_k_tower': False,
        'public_status': 'not_claimed', 'gcp_used': False,
    }


def source_binding(source: dict, protocol: dict, producer: str, kind: str | None) -> None:
    require(type(source) is dict and type(source.get('files')) is dict and 3 <= len(source['files']) <= 512,
            'source_inventory')
    for name, pin in source['files'].items():
        relative(name)
        require(is_sha(pin), 'source_pin')
    require(is_sha(producer) and source['files'].get(PRODUCER) == producer
            and protocol['producer_sha256'] == producer, 'expected_producer_binding')
    for name, field in ((PROBE, 'probe_sha256'), (DIGEST, 'digest_header_sha256')):
        require(source['files'].get(name) == protocol[field] and is_sha(protocol[field]), 'source_protocol_binding')
    require(source.get('binary') == protocol['binary'] and is_sha(source.get('binary_sha256'))
            and source['binary_sha256'] == protocol['binary_sha256'], 'binary_source_binding')
    if kind is not None:
        require(kind in OLD and (producer, source['binary_sha256']) == OLD[kind]
                and protocol['probe_sha256'] == OLD_PROBE and protocol['judge_sha256'] == OLD_JUDGE,
                'historical_source_kind_binding')
    else:
        require(producer not in {entry[0] for entry in OLD.values()}
                and source['binary_sha256'] not in {entry[1] for entry in OLD.values()}
                and protocol['probe_sha256'] != OLD_PROBE
                and protocol.get('probe_schema') == SCHEMAS[1]
                and protocol.get('successor_accounting') == ACCOUNTING[1]
                and protocol['judge_sha256'] == JUDGE_SHA, 'new_source_calendar_binding')


def load_arm(reader: Reader, receipt_arg: str, protocol_arg: str, source_arg: str,
             producer: str, kind: str | None, judge: object) -> tuple[dict, dict, dict]:
    path, raw_receipt = reader.argument(receipt_arg)
    require(path.name.endswith('.receipt.json'), 'attempt_receipt_required')
    stem = path.name.removesuffix('.receipt.json')
    protocol_path, protocol_raw = reader.argument(protocol_arg)
    sources_path, sources_raw = reader.argument(source_arg)
    require(protocol_path == path.parent / 'protocol.json'
            and sources_path == path.parent / (stem + '.sources_before.json'), 'attempt_metadata_scope')
    record, protocol, source = loads(raw_receipt), loads(protocol_raw), loads(sources_raw)
    source_binding(source, protocol, producer, kind)
    require(reader.read(path.parent / (stem + '.sources_after.json')) == sources_raw, 'attempt_source_drift')
    raw_name = stem + '.raw.txt'
    require(type(record['streams']) is dict and set(record['streams']) == {raw_name}, 'stream_inventory')
    stream = record['streams'][raw_name]
    require(type(stream) is dict and set(stream) == {'bytes', 'sha256'} and is_sha(stream['sha256']), 'stream_binding')
    unsigned(stream['bytes'], 'stream_bytes')
    raw = reader.read(path.parent / raw_name, stream['sha256'])
    require(len(raw) == stream['bytes'], 'stream_byte_count')
    command = loads(reader.read(path.parent / (stem + '.command.json')))
    intent = loads(reader.read(path.parent / (stem + '.intent.json')))
    equal(command, {key: value for key, value in record.items() if key not in {'orders', 'terminal'}}, 'command_receipt_binding')
    equal(intent, {key: command[key] for key in intent}, 'intent_command_binding')
    for field in ('exit_code', 'outer_timeout_seconds', 'process_vm_max_bytes', 'drain_term_grace_seconds'):
        unsigned(record[field], field)
    require(record['capture'] == 'merged_exact_bytes' and record['cwd'] == str(ROOT)
            and record['argv'] == command['argv'], 'capture_contract')
    binary = ROOT / relative(source['binary'])
    reader.read(binary, source['binary_sha256'], MAX_BINARY)
    parsed = []
    for line in raw.decode('utf-8').splitlines():
        if not line.startswith('{'):
            break
        parsed.append(loads(line))
    require(len(parsed) >= 2, 'raw_rows')
    equal(record['orders'], parsed[1:-1], 'typed_receipt_raw_orders')
    equal(record['terminal'], parsed[-1], 'typed_receipt_raw_terminal')
    verdict = judge.judge(raw.decode('utf-8'), record, intent, protocol)
    require(verdict['audit_status'] == 'valid', 'pinned_receipt_judge_rejected')
    if kind == 'singleton21b77':
        require(parsed[0]['n'] in (8, 8000), 'historical_singleton_scope_not_16k_or_32k')
    return parsed[0], record, source


def synthetic() -> tuple[dict, dict, dict, dict]:
    config = {key: 'fixture' for key in CONFIG_STRINGS - {'successor_accounting'}}
    config.update({key: False for key in CONFIG_FLAGS})
    config.update(schema=SCHEMAS[0], type='configuration', n=8, kmax_effective=5)
    orders = []
    for k in range(1, 6):
        row = {key: 0 for key in ORDER_COUNTS}
        row.update({key: 1.0 for key in ORDER_MEASURES})
        row.update({key: 'fixture' for key in ORDER_STRINGS - {'successor_accounting'}})
        row.update({key: False for key in ORDER_FLAGS})
        row.update(schema=SCHEMAS[0], type='order', k=k, provisional=True, outcome='complete_relative',
                   reason=AUTH, certificate_digest=sha(str(k).encode()), normalized_anchors=2, successor_steps=9)
        orders.append(row)
    terminal = {key: 'fixture' for key in TERMINAL_STRINGS - {'successor_accounting'}}
    terminal.update({key: False for key in TERMINAL_FLAGS})
    terminal.update({key: 1.0 for key in TERMINAL_MEASURES})
    terminal.update(schema=SCHEMAS[0], type='terminal', exit_code=0, complete_requested_horizontal_orders=True,
        terminal_status='completed', outcome='complete_relative', reason=AUTH, last_order=5,
        completed_orders_diagnostic=5, input_digest=sha(b'input'), certificate_digest=sha(b'global'),
        stage_ms={'full': 1.0}, last_order_work={key: orders[-1][key] for key in WORK})
    terminal['last_order_work']['diagnostic_only'] = True
    old = {'status': 'completed', 'error': None, 'exit_code': 0, 'orders': orders, 'terminal': terminal}
    new_config, new = copy.deepcopy(config), copy.deepcopy(old)
    for row in [new_config, *new['orders'], new['terminal']]:
        row['schema'] = SCHEMAS[1]
        row['successor_accounting'] = ACCOUNTING[1]
    for row in [*new['orders'], new['terminal']['last_order_work']]:
        row['successor_steps'] -= 2 * row['normalized_anchors']
    return config, old, new_config, new


def selftest() -> dict:
    base = synthetic()
    compare_objects(*base, 'complete')
    names = []
    def killed(name: str, mutate: object, scope: str = 'complete', prefix: int | None = None) -> None:
        values = copy.deepcopy(base)
        mutate(values)
        try:
            compare_objects(*values, scope, prefix)
        except (ValueError, KeyError, TypeError):
            names.append(name)
        else:
            raise ValueError('surviving_model:' + name)
    killed('calendar_missing', lambda x: x[3]['orders'][0].pop('successor_accounting'))
    killed('calendar_downgrade', lambda x: x[3]['orders'][0].update(successor_accounting=ACCOUNTING[0]))
    killed('schema_downgrade', lambda x: x[3]['orders'][0].update(schema=SCHEMAS[0]))
    killed('legacy_calendar_injected', lambda x: x[1]['orders'][0].update(successor_accounting=ACCOUNTING[0]))
    killed('steps_counter', lambda x: x[3]['orders'][0].update(successor_steps=6))
    killed('anchors_counter', lambda x: x[3]['orders'][0].update(normalized_anchors=3))
    killed('other_counter', lambda x: x[3]['orders'][0].update(query_nodes=1))
    killed('order_digest', lambda x: x[3]['orders'][0].update(certificate_digest=sha(b'wrong')))
    killed('global_digest', lambda x: x[3]['terminal'].update(certificate_digest=sha(b'wrong')))
    killed('last_work_steps', lambda x: x[3]['terminal']['last_order_work'].update(successor_steps=6))
    killed('last_work_counter', lambda x: x[3]['terminal']['last_order_work'].update(cache_hits=1))
    killed('configuration', lambda x: x[2].update(n=16000))
    killed('integer_bool', lambda x: [r['orders'][0].update(query_nodes=False) for r in (x[1], x[3])])
    killed('integer_float', lambda x: [r['orders'][0].update(query_nodes=0.0) for r in (x[1], x[3])])
    killed('flag_integer', lambda x: x[3]['orders'][0].update(provisional=1))
    killed('completion_flag_integer', lambda x: x[3]['terminal'].update(complete_requested_horizontal_orders=1))
    killed('float_configuration', lambda x: [c.update(n=8.0) for c in (x[0], x[2])])
    killed('terminal_count_bool', lambda x: [r['terminal'].update(last_order=True) for r in (x[1], x[3])])
    killed('missing_order', lambda x: x[3]['orders'].pop())
    killed('prefix_without_count', lambda _: None, 'successful-prefix-diagnostic')
    def refused(values: tuple) -> None:
        record = values[1]
        record['orders'].pop()
        record.update(exit_code=2)
        record['terminal'].update(exit_code=2, terminal_status='failed', outcome='resource_exhausted',
            reason='full_gabriel_successor_budget', certificate_digest='', complete_requested_horizontal_orders=False,
            completed_orders_diagnostic=4, last_order=5)
    killed('global_refusal_not_complete', refused)
    values = copy.deepcopy(base)
    refused(values)
    result = compare_objects(*values, 'successful-prefix-diagnostic', 4)
    require(result['global_complete_comparison'] is False and result['global_certificate_digest'] is None
            and result['orders_compared'] == 4 and not result['last_order_work_relation_checked'], 'prefix_not_promoted')
    killed('success_promotion', lambda x: (refused(x), x[1]['terminal'].update(complete_requested_horizontal_orders=True)),
           'successful-prefix-diagnostic', 4)
    killed('refused_order_in_prefix', lambda x: x[1]['orders'][0].update(outcome='resource_exhausted'),
           'successful-prefix-diagnostic', 4)
    for raw in (b'{"x":1,"x":2}', b'{"x":NaN}', b'{"x":Infinity}'):
        try:
            loads(raw)
        except ValueError:
            names.append('strict_json_' + str(len(names)))
        else:
            raise ValueError('surviving_json_model')
    producer, binary = OLD['singleton21b77']
    source = {'files': {PRODUCER: producer, PROBE: OLD_PROBE, DIGEST: sha(b'digest')},
              'binary': 'build/model', 'binary_sha256': binary}
    protocol = {'producer_sha256': producer, 'probe_sha256': OLD_PROBE,
                'digest_header_sha256': sha(b'digest'), 'binary': 'build/model',
                'binary_sha256': binary, 'judge_sha256': OLD_JUDGE}
    source_binding(source, protocol, producer, 'singleton21b77')
    new_source, new_protocol = copy.deepcopy(source), copy.deepcopy(protocol)
    new_source['files'][PRODUCER] = 'e' * 64
    new_source['files'][PROBE] = 'd' * 64
    new_source['binary_sha256'] = 'b' * 64
    new_protocol.update(producer_sha256='e' * 64, probe_sha256='d' * 64,
                        binary_sha256='b' * 64, probe_schema=SCHEMAS[1],
                        successor_accounting=ACCOUNTING[1], judge_sha256=JUDGE_SHA)
    source_binding(new_source, new_protocol, 'e' * 64, None)
    source_mutants = [
        ('expected_old_producer', lambda s, p: s['files'].update({PRODUCER: 'f' * 64}), False),
        ('old_binary_binding', lambda s, p: s.update(binary_sha256='f' * 64), False),
        ('probe_source_binding', lambda s, p: p.update(probe_sha256='f' * 64), False),
        ('new_protocol_calendar_missing', lambda s, p: p.pop('successor_accounting'), True),
        ('new_protocol_calendar_downgrade', lambda s, p: p.update(successor_accounting=ACCOUNTING[0]), True),
        ('new_protocol_schema_downgrade', lambda s, p: p.update(probe_schema=SCHEMAS[0]), True),
        ('old_binary_relabelled_new', lambda s, p: (s.update(binary_sha256=binary), p.update(binary_sha256=binary)), True),
        ('old_probe_relabelled_new', lambda s, p: (s['files'].update({PROBE: OLD_PROBE}), p.update(probe_sha256=OLD_PROBE)), True),
    ]
    for name, mutate, current in source_mutants:
        s, p = copy.deepcopy((new_source, new_protocol) if current else (source, protocol))
        mutate(s, p)
        try:
            source_binding(s, p, 'e' * 64 if current else producer, None if current else 'singleton21b77')
        except (ValueError, KeyError, TypeError):
            names.append(name)
        else:
            raise ValueError('surviving_source_model:' + name)
    require(len(names) == 34, 'model_nonvacuum')
    return {'selftest_status': 'passed', 'mutants_killed': len(names), 'models': names,
            'complete_positive': 1, 'diagnostic_prefix_positive': 1,
            'engine_invoked': False, 'gcp_used': False}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selftest', action='store_true')
    parser.add_argument('--expected-comparator-sha256')
    parser.add_argument('--old')
    parser.add_argument('--new')
    parser.add_argument('--old-protocol')
    parser.add_argument('--new-protocol')
    parser.add_argument('--old-sources')
    parser.add_argument('--new-sources')
    parser.add_argument('--old-kind', choices=sorted(OLD))
    parser.add_argument('--new-producer-sha256')
    parser.add_argument('--judge')
    parser.add_argument('--scope', choices=('complete', 'successful-prefix-diagnostic'))
    parser.add_argument('--prefix-orders', type=int)
    args = parser.parse_args()
    try:
        options = [arg.split('=', 1)[0] for arg in sys.argv[1:] if arg.startswith('--')]
        require(len(options) == len(set(options)), 'duplicate_cli_option')
        if args.selftest:
            require(all(value is None for key, value in vars(args).items() if key != 'selftest'), 'selftest_arguments')
            result = selftest()
        else:
            require(all(getattr(args, key) is not None for key in (
                'expected_comparator_sha256', 'old', 'new', 'old_protocol', 'new_protocol',
                'old_sources', 'new_sources', 'old_kind', 'new_producer_sha256', 'judge', 'scope')), 'explicit_inputs_required')
            reader = Reader()
            reader.read(Path(__file__).absolute(), args.expected_comparator_sha256)
            judge_path, judge_raw = reader.argument(args.judge)
            require(judge_path == JUDGE and sha(judge_raw) == JUDGE_SHA, 'closed_pure_judge_pin')
            module = types.ModuleType('pinned_successor_comparison_judge')
            module.__file__ = str(judge_path)
            exec(compile(judge_raw, str(judge_path), 'exec'), module.__dict__)
            oc, old, osrc = load_arm(reader, args.old, args.old_protocol, args.old_sources,
                                    OLD[args.old_kind][0], args.old_kind, module)
            nc, new, nsrc = load_arm(reader, args.new, args.new_protocol, args.new_sources,
                                    args.new_producer_sha256, None, module)
            for name in osrc['files'].keys() & nsrc['files'].keys():
                if name.startswith('morsehgp3D_v7/src/') and name != PRODUCER:
                    require(osrc['files'][name] == nsrc['files'][name], 'other_common_product_source_changed:' + name)
            result = compare_objects(oc, old, nc, new, args.scope, args.prefix_orders)
            result.update(old_source_kind=args.old_kind, old_producer_sha256=OLD[args.old_kind][0],
                          new_producer_sha256=args.new_producer_sha256, receipt_judge_sha256=JUDGE_SHA,
                          compiled_source_claim='explicitly_pinned_captured_source_maps_not_live_historical_sources')
            reader.stable()
            result['inputs'] = {str(path): {'sha256': pin, 'bytes': size}
                                for path, (pin, size) in sorted(reader.watches.items())}
        print(canonical(result))
        return 0
    except (ValueError, KeyError, TypeError, OSError, UnicodeError, RecursionError) as error:
        print(canonical({'comparison_status': 'invalid', 'reason': str(error), 'engine_invoked': False}), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
