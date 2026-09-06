#!/usr/bin/env python3
"""Read-only v4 P=0/opt-in pair comparison; not a geometry or SLO certificate.

Both arms require their independently pinned primary and first-C judges.
Only common successful order rows are compared as forests; closed matching
refusals additionally compare their old terminal work prefix. Censored or
missing-terminal captures are rejected, never repaired or promoted.
Binary identity is the admitted capture hash, not a reread of a live ELF.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import re
import shlex
import sys
import types

SCHEMA = 'mhgp7-full-gabriel-probe-v4'
SUCCESSOR = 'full_successor_reads_writes_no_last_pair_v2'
ACCOUNTING = 'reference_ordinal_plus_native_z_q3_q4_proposal_v2'
CAP = 'max_meb_proposal_supports_per_order'
MAX_P = 584000000
# Explicitly reviewed/frozen after the 48 historical + 72 v4 reader models.
# Never accept a replacement judge merely by hashing its current live bytes.
PRIMARY_SHA = '475b92884d4e0aac5f9a2856ab841401ea1681a3477ea53599faa9a5140b3e11'
FIRST_C_SHA = '9f54cb46518390942079379168813da84a4789fae482cf851627122a857799b6'
WORK = set(('input_records aliases face_visits alias_hits portal_requests chain_steps terminal_direct '
    'max_chain_length normalized_anchors successor_steps no_op_connections meb_calls geometry_meb_calls '
    'meb_supports query_nodes query_leaves query_range_skips minimum_lookups minimum_hits cache_lookups '
    'cache_hits cache_inserts cache_skips singleton_intruder_resolutions direct_lookups').split())
MEB = set(('meb_proposal_supports meb_proposal_pivots meb_proposal_certified '
           'meb_proposal_fallback meb_reference_supports').split())
ORDER_MEASURES = set('build_ms digest_ms expand_ms read_ms release_ms rss_mib_sample hwm_mib_sample'.split())
TERMINAL_MEASURES = set(('stage_ms compute_read_release_ms_subtracted_diagnostic digest_ms '
    'elapsed_before_terminal_ms generation_rects_ms generation_wspd_ms provisional_output_ms '
    'rss_mib_sample hwm_mib_sample').split())
STRINGS = set(('schema type successor_accounting meb_accounting phase backend profile mode public_status '
    'authority family input_generator input_digest_kind certificate_digest_kind s_comparison_scope '
    'alias_policy cache_policy digest_scratch_scope digest_timing_scope read_kind outcome reason '
    'certificate_digest terminal_status input_digest last_stage reference_timing subtracted_timing_scope').split())
FLAGS = set(('gpu archive vertical provisional whole_tower_authority complete_requested_horizontal_orders '
    'integrated_inter_k_tower certificate_retained digest_proves_catalogue_completeness '
    'terminal_root_coverage_proves_equality frontier_ledger_closed rank_window_regular diagnostic_only').split())
AUTH = 'full_horizontal_relative_to_supplied_complete_exact_regular_gabriel_catalogues'
FILES = {'producer_sha256': 'morsehgp3D_v7/src/forest/full_gabriel.hpp',
         'meb_header_sha256': 'morsehgp3D_v7/src/forest/meb_proposal.hpp',
         'probe_sha256': 'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp',
         'digest_header_sha256': 'morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp'}


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def is_sha(value: object) -> bool:
    return type(value) is str and re.fullmatch('[0-9a-f]{64}', value) is not None


def canonical(value: object) -> str:
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


def equal(a: object, b: object, reason: str) -> None:
    require(canonical(a) == canonical(b), reason)  # Distinguish True, 1, 1.0.


def loads(raw: bytes | str) -> object:
    def pairs(items: list) -> dict:
        result = {}
        for key, value in items:
            require(key not in result, 'duplicate_json_key')
            result[key] = value
        return result
    value = json.loads(raw, object_pairs_hook=pairs,
                       parse_constant=lambda _: require(False, 'nonfinite_json'))
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


def unsigned(value: object, name: str) -> None:
    require(type(value) is int and 0 <= value < 1 << 64, 'integer_field:' + name)


def measure(value: object) -> None:
    if type(value) is dict:
        for item in value.values():
            measure(item)
    else:
        require(type(value) in (int, float) and math.isfinite(value) and value >= 0, 'measurement_type')


def typed(row: dict) -> None:
    require(type(row) is dict, 'row_object')
    for name, value in row.items():
        if name in STRINGS:
            require(type(value) is str, 'string_field:' + name)
        elif name in FLAGS:
            require(type(value) is bool, 'boolean_field:' + name)
        elif name in ORDER_MEASURES | TERMINAL_MEASURES:
            measure(value)
        elif name == 'last_order_work':
            require(type(value) is dict and set(value) == WORK | MEB | {'diagnostic_only'} and value['diagnostic_only'] is True,
                    'work_inventory')
            typed(value)
        else:
            unsigned(value, name)


def coherent(config: dict, receipt: dict, cap: int) -> None:
    require(type(cap) is int and 0 <= cap <= MAX_P, 'proposal_cap_domain')
    require(receipt.get('status') == 'completed' and receipt.get('error') is None, 'capture_not_closed')
    terminal, orders = receipt['terminal'], receipt['orders']
    require(type(terminal) is dict and type(orders) is list, 'censored_or_missing_terminal')
    for row in [config, *orders, terminal]:
        typed(row)
        require(row['schema'] == SCHEMA and row['successor_accounting'] == SUCCESSOR and
                row['meb_accounting'] == ACCOUNTING and row[CAP] == cap, 'accounting_or_cap_binding')
    require(config['type'] == 'configuration' and terminal['type'] == 'terminal' and
            all(row['type'] == 'order' for row in orders), 'row_inventory')
    require(type(config['kmax_effective']) is int and 1 <= config['kmax_effective'] <= 10 and
            [r['k'] for r in orders] == list(range(1, len(orders)+1)) and
            len(orders) <= config['kmax_effective'], 'order_sequence')
    complete = [r for r in orders if r['outcome'] == 'complete_relative']
    require(all(r['outcome'] == 'complete_relative' for r in orders[:-1]), 'nonterminal_refusal')
    for row in orders:
        require(WORK | MEB <= row.keys() and row['provisional'] is True and
                row['whole_tower_authority'] is False, 'order_scope')
        require((is_sha(row['certificate_digest']) and row['reason'] == AUTH) if row in complete else
                row['certificate_digest'] == '', 'order_digest_scope')
    code = {'complete_relative': 0, 'invalid_input': 2, 'unsupported_degeneracy': 2,
            'resource_exhausted': 2, 'invariant_violated': 3}[terminal['outcome']]
    require(type(receipt['exit_code']) is int and receipt['exit_code'] == terminal['exit_code'] == code and
            terminal['terminal_status'] == ('completed' if code == 0 else 'failed') and
            terminal['complete_requested_horizontal_orders'] is (code == 0), 'terminal_promotion')
    require(terminal['completed_orders_diagnostic'] == len(complete) and
            len(complete) <= terminal['last_order'] <= min(len(complete)+1, config['kmax_effective']), 'completed_prefix')
    require(code != 0 or len(complete) == config['kmax_effective'], 'global_completion')
    require(is_sha(terminal['input_digest']), 'paired_input_digest_required')
    require(is_sha(terminal['certificate_digest']) if code == 0 else terminal['certificate_digest'] == '', 'refusal_promotion')
    require(terminal['public_status'] == 'not_claimed' and terminal['integrated_inter_k_tower'] is False and
            terminal['certificate_retained'] is False and terminal['digest_proves_catalogue_completeness'] is False and
            terminal['terminal_root_coverage_proves_equality'] is False, 'public_scope')


def project(row: dict, excluded: set[str]) -> dict:
    result = {key: value for key, value in row.items() if key not in excluded}
    if 'last_order_work' in result:
        result['last_order_work'] = project(result['last_order_work'], MEB)
    return result


def differences(left: dict, right: dict, fields: set[str]) -> dict:
    def delta(a: object, b: object) -> object:
        if type(a) is dict:
            require(type(b) is dict and set(a) == set(b), 'measurement_inventory')
            return {k: delta(a[k], b[k]) for k in sorted(a)}
        return {'baseline': a, 'opt_in': b, 'opt_in_minus_baseline': b-a}
    return {k: delta(left[k], right[k]) for k in sorted(fields)}


def compare_objects(lc: dict, left: dict, rc: dict, right: dict, proposal_cap: int,
                    require_complete: bool = False) -> dict:
    require(type(proposal_cap) is int and 0 < proposal_cap <= MAX_P, 'explicit_opt_in_cap')
    coherent(lc, left, 0)
    coherent(rc, right, proposal_cap)
    equal(project(lc, {CAP}), project(rc, {CAP}), 'pair_configuration_differs')
    lt, rt = left['terminal'], right['terminal']
    equal(lt['input_digest'], rt['input_digest'], 'pair_input_differs')
    whole = left['exit_code'] == right['exit_code'] == 0
    require(not require_complete or whole, 'complete_pair_required')
    successful = min(lt['completed_orders_diagnostic'], rt['completed_orders_diagnostic'])
    orders = []
    for lrow, rrow in zip(left['orders'][:successful], right['orders'][:successful]):
        require(lrow['outcome'] == rrow['outcome'] == 'complete_relative', 'failed_order_not_success')
        equal(project(lrow, ORDER_MEASURES | MEB | {CAP}),
              project(rrow, ORDER_MEASURES | MEB | {CAP}), 'successful_order_nonmeasure_differs')
        orders.append(dict(k=lrow['k'], certificate_digest=lrow['certificate_digest'],
            meb_diagnostics=differences(lrow, rrow, MEB), measurements=differences(lrow, rrow, ORDER_MEASURES)))
    require(len(orders) == successful, 'common_prefix_length')
    refusal_identity = ('exit_code', 'outcome', 'reason', 'last_order', 'last_stage')
    same_refusal = not whole and left['exit_code'] != 0 and right['exit_code'] != 0 and all(
        lt[k] == rt[k] for k in refusal_identity)
    terminal_checked = whole or same_refusal
    if terminal_checked:
        equal(project(lt, TERMINAL_MEASURES | {CAP}), project(rt, TERMINAL_MEASURES | {CAP}),
              'terminal_nonmeasure_or_work_prefix_differs')
    return dict(schema='mhgp7-full-meb-pair-comparison-v1', comparison_status='valid',
        scope='common_successful_horizontal_order_prefix', public_status='not_claimed',
        all_requested_horizontal_orders_compared=whole, global_FULL_successful=False,
        successful_orders_compared=successful, no_common_successful_order=successful == 0,
        input_digest=lt['input_digest'], global_certificate_digest=lt['certificate_digest'] if whole else None,
        baseline_exit_code=left['exit_code'], opt_in_exit_code=right['exit_code'],
        baseline_outcome=lt['outcome'], opt_in_outcome=rt['outcome'], proposal_cap=proposal_cap,
        terminal_old_nonmeasure_and_work_checked=terminal_checked, same_refusal_prefix_checked=same_refusal,
        orders=orders, terminal_measurements=differences(lt, rt, TERMINAL_MEASURES),
        last_meb_diagnostics=differences(lt['last_order_work'], rt['last_order_work'], MEB),
        exclusions=dict(cap=[CAP], proposal_diagnostics=sorted(MEB), order_measures=sorted(ORDER_MEASURES),
                        terminal_measures=sorted(TERMINAL_MEASURES)),
        old_work_fields_per_compared_order=len(WORK), unserialized_F_geometry_fields_not_compared=8,
        timings_are_diagnostics_only=True, timing_ratio_computed=False, slo_claim=False,
        geometry_or_catalogue_completeness_proved=False, integrated_inter_k_tower=False,
        engine_invoked=False, gcp_used=False)


def relative(name: str) -> str:
    require(type(name) is str and name and str(PurePosixPath(name)) == name and
            not name.startswith('/') and '..' not in PurePosixPath(name).parts, 'unsafe_relative_path')
    return name


class Reader:
    """Portable pinned metadata reader. No ELF or source tree is opened."""
    def __init__(self) -> None:
        self.watches: dict[Path, tuple[str, int]] = {}
        self.total = 0

    def read(self, path: Path, pin: str | None = None) -> bytes:
        require(path.is_absolute() and '..' not in path.parts, 'absolute_input_required')
        require(all(not p.is_symlink() for p in (path, *path.parents)), 'input_symlink')
        require(path.is_file() and path.stat().st_size <= 2 << 20, 'input_size_or_kind')
        with path.open('rb') as stream:
            raw = stream.read((2 << 20) + 1)
        require(len(raw) <= 2 << 20 and (pin is None or is_sha(pin) and sha(raw) == pin), 'input_pin_or_size')
        stamp = (sha(raw), len(raw))
        require(path not in self.watches or self.watches[path] == stamp, 'input_drift')
        if path not in self.watches:
            self.total += len(raw)
            require(self.total <= 32 << 20, 'total_input_budget')
        self.watches[path] = stamp
        return raw

    def argument(self, argument: str) -> tuple[Path, bytes]:
        written, separator, pin = argument.rpartition('=')
        require(separator and is_sha(pin), 'explicit_PATH_SHA256_required')
        path = Path(written)
        return path, self.read(path, pin)

    def stable(self) -> None:
        for path, (pin, size) in list(self.watches.items()):
            require(len(self.read(path, pin)) == size, 'final_input_drift')


def source_binding(source: dict, protocol: dict) -> None:
    require(type(source) is dict and type(source.get('files')) is dict and
            4 <= len(source['files']) <= 1024, 'source_inventory')
    for name, pin in source['files'].items():
        relative(name)
        require(is_sha(pin), 'source_pin')
    require(is_sha(source['source_sha256']), 'source_map_pin')
    encoded = (json.dumps(source['files'], sort_keys=True, indent=2) + '\n').encode()
    require(source['source_sha256'] == sha(encoded), 'source_map_digest')
    for field, name in FILES.items():
        require(is_sha(protocol[field]) and source['files'].get(name) == protocol[field], 'source_protocol_binding')
    require(relative(source['binary']) == protocol['binary'] and is_sha(source['binary_sha256']) and
            source['binary_sha256'] == protocol['binary_sha256'], 'binary_source_binding')
    require(source['probe_schema'] == protocol['probe_schema'] == SCHEMA and
            source['successor_accounting'] == protocol['successor_accounting'] == SUCCESSOR and
            protocol['meb_accounting'] == ACCOUNTING, 'source_accounting_binding')
    if 'meb_accounting' in source:
        require(source['meb_accounting'] == ACCOUNTING, 'source_meb_accounting')
    require(is_sha(PRIMARY_SHA) and is_sha(FIRST_C_SHA) and
            protocol['judge_sha256'] == PRIMARY_SHA and protocol['first_c_sha256'] == FIRST_C_SHA,
            'fixed_judge_protocol_binding')


def pair_sources(left: dict, right: dict) -> None:
    equal(left, right, 'pair_source_map_or_captured_binary_differs')


def load_module(reader: Reader, argument: str, pin: str, name: str) -> object:
    path, raw = reader.argument(argument)
    require(is_sha(pin) and sha(raw) == pin, 'fixed_import_pin')
    module = types.ModuleType(name)
    module.__file__ = str(path)
    # Neither frozen judge executes its CLI on import. No subprocess, pycache,
    # dynamic live-hash acceptance, or historical-v2 accounting transformation.
    exec(compile(raw, str(path), 'exec'), module.__dict__)
    return module


def load_arm(reader: Reader, receipt_arg: str, protocol_arg: str, sources_arg: str,
             primary: object, first_c: object) -> tuple[dict, dict, dict, dict]:
    path, receipt_raw = reader.argument(receipt_arg)
    require(path.name.endswith('.receipt.json'), 'attempt_receipt_required')
    stem = path.name.removesuffix('.receipt.json')
    protocol_path, protocol_raw = reader.argument(protocol_arg)
    sources_path, sources_raw = reader.argument(sources_arg)
    require(protocol_path == path.parent / 'protocol.json' and
            sources_path == path.parent / (stem + '.sources_before.json'), 'attempt_metadata_scope')
    receipt, protocol, source = loads(receipt_raw), loads(protocol_raw), loads(sources_raw)
    source_binding(source, protocol)
    require(reader.read(path.parent / (stem + '.sources_after.json')) == sources_raw, 'attempt_source_drift')
    raw_name = stem + '.raw.txt'
    require(type(receipt['streams']) is dict and set(receipt['streams']) == {raw_name}, 'stream_inventory')
    stream = receipt['streams'][raw_name]
    require(type(stream) is dict and set(stream) == {'bytes', 'sha256'} and is_sha(stream['sha256']), 'stream_binding')
    unsigned(stream['bytes'], 'stream_bytes')
    raw = reader.read(path.parent / raw_name, stream['sha256'])
    require(len(raw) == stream['bytes'], 'stream_byte_count')
    command = loads(reader.read(path.parent / (stem + '.command.json')))
    intent = loads(reader.read(path.parent / (stem + '.intent.json')))
    equal(command, {k: v for k, v in receipt.items() if k not in {'orders', 'terminal'}}, 'command_receipt_binding')
    require(type(intent) is dict and set(intent) == set(('argv capture command cwd drain_term_grace_seconds '
            'expected_rc id outer_timeout_seconds process_vm_max_bytes started').split()), 'intent_inventory')
    equal(intent, {k: command[k] for k in intent}, 'intent_command_binding')
    equal(receipt['argv'], shlex.split(receipt['command']), 'argv_command_binding')
    require(receipt['id'] == stem and receipt['capture'] == 'merged_exact_bytes' and
            type(receipt['cwd']) is str and Path(receipt['cwd']).is_absolute(), 'capture_contract')
    for field in ('exit_code', 'outer_timeout_seconds', 'process_vm_max_bytes', 'drain_term_grace_seconds'):
        unsigned(receipt[field], field)
    measure(receipt['elapsed_seconds'])
    parsed = []
    for line in raw.decode('utf-8').splitlines():
        if not line.startswith('{'):
            break
        parsed.append(loads(line))
    require(2 <= len(parsed) <= 12 and parsed[0].get('type') == 'configuration' and
            parsed[-1].get('type') == 'terminal', 'censored_or_missing_terminal')
    equal(receipt['orders'], parsed[1:-1], 'typed_receipt_raw_orders')
    equal(receipt['terminal'], parsed[-1], 'typed_receipt_raw_terminal')
    first = primary.judge(raw.decode('utf-8'), receipt, intent, protocol)
    second = first_c.judge(raw, receipt, protocol)
    require(first['audit_status'] == 'valid' and second['supplement_status'] == 'valid' and
            first['attempt_success'] is (receipt['exit_code'] == 0) and
            second['attempt_success'] is (receipt['exit_code'] == 0), 'independent_judge_rejection')
    gnu = {}
    labels = {'User time (seconds)': 'user_seconds', 'System time (seconds)': 'system_seconds',
              'Elapsed (wall clock) time (h:mm:ss or m:ss)': 'wall_clock_text',
              'Maximum resident set size (kbytes)': 'peak_rss_kbytes'}
    for line in raw.decode('utf-8').splitlines()[len(parsed):]:
        for label, key in labels.items():
            prefix = label + ': '
            if line.strip().startswith(prefix):
                require(key not in gnu, 'duplicate_gnu_measure')
                value = line.strip()[len(prefix):]
                gnu[key] = value if key == 'wall_clock_text' else int(value) if key == 'peak_rss_kbytes' else float(value)
    require(set(gnu) == set(labels.values()), 'gnu_measure_inventory')
    return parsed[0], receipt, source, dict(primary=first, first_c=second, gnu_time=gnu,
        capture_elapsed_seconds=receipt['elapsed_seconds'], source_map_sha256=source['source_sha256'])


def model() -> tuple[dict, dict, dict, dict]:
    """Minimal pure comparison models, deliberately not engine receipts."""
    base = dict(schema=SCHEMA, successor_accounting=SUCCESSOR, meb_accounting=ACCOUNTING)
    config = dict(base, type='configuration', n=8, s=8, kmax_requested=5, kmax_effective=5,
                  alias_policy='lazy', cache_entries=1, max_meb_calls_per_order=4000000, **{CAP: 0})
    orders = []
    for k in range(1, 6):
        row = dict(base, **{field: 0 for field in WORK | MEB}, **{field: 1.0 for field in ORDER_MEASURES})
        row.update(type='order', k=k, provisional=True, whole_tower_authority=False,
                   outcome='complete_relative', reason=AUTH, certificate_digest=sha(str(k).encode()), **{CAP: 0})
        orders.append(row)
    terminal = dict(base, **{field: 1.0 for field in TERMINAL_MEASURES})
    terminal.update(type='terminal', exit_code=0, complete_requested_horizontal_orders=True,
        terminal_status='completed', outcome='complete_relative', reason=AUTH, last_order=5,
        last_stage='release', completed_orders_diagnostic=5, input_digest=sha(b'input'),
        certificate_digest=sha(b'global'), stage_ms={'full': 1.0}, public_status='not_claimed',
        integrated_inter_k_tower=False, certificate_retained=False, digest_proves_catalogue_completeness=False,
        terminal_root_coverage_proves_equality=False,
        last_order_work=dict(diagnostic_only=True, **{field: orders[-1][field] for field in WORK | MEB}), **{CAP: 0})
    left = dict(status='completed', error=None, exit_code=0, orders=orders, terminal=terminal)
    rc, right = copy.deepcopy(config), copy.deepcopy(left)
    for row in [rc, *right['orders'], right['terminal']]:
        row[CAP] = 100
    return config, left, rc, right


def refusal(arm: dict, completed: int = 2) -> None:
    arm['exit_code'] = 2
    arm['orders'] = arm['orders'][:completed]
    arm['terminal'].update(exit_code=2, outcome='resource_exhausted', reason='max_meb_calls',
        terminal_status='failed', complete_requested_horizontal_orders=False, certificate_digest='',
        completed_orders_diagnostic=completed, last_order=completed+1, last_stage='full')


def selftest() -> dict:
    positives, mutants = [], []
    def success(name: str, case: tuple, expected: int, complete: bool) -> None:
        result = compare_objects(*case, 100)
        require(result['successful_orders_compared'] == expected and
                result['all_requested_horizontal_orders_compared'] is complete and
                result['global_FULL_successful'] is False and result['slo_claim'] is False, 'model_scope')
        positives.append(name)
    success('complete_identical', model(), 5, True)
    case = model()
    for row in case[3]['orders']:
        for field in ORDER_MEASURES:
            row[field] += 2.0
        for field in MEB:
            row[field] += 1
    for field in TERMINAL_MEASURES:
        case[3]['terminal'][field] = {'full': 4.0} if field == 'stage_ms' else 4.0
    for field in MEB:
        case[3]['terminal']['last_order_work'][field] += 1
    success('explicit_measure_and_diagnostic_exclusions', case, 5, True)
    for label, counts in [('same_refusal', (2, 2)), ('different_refusal', (2, 3)), ('zero_prefix', (0, 0))]:
        case = model()
        refusal(case[1], counts[0])
        refusal(case[3], counts[1])
        success(label, case, min(counts), False)
    def rejected(name: str, change, reason: str, base=None, complete=False) -> None:
        case = model() if base is None else copy.deepcopy(base)
        change(case)
        try:
            compare_objects(*case, 100, require_complete=complete)
        except (ValueError, KeyError, TypeError) as error:
            require(reason in str(error), 'wrong_mutant_rejection:' + name + ':' + str(error))
            mutants.append(name)
            return
        raise ValueError('accepted_mutant:' + name)
    for field, value in [('certificate_digest', sha(b'wrong')), ('successor_steps', 1), ('meb_supports', 1),
                         ('query_nodes', 1), ('cache_skips', 1)]:
        rejected('order_' + field, lambda c, f=field, v=value: c[3]['orders'][0].__setitem__(f, v),
                 'successful_order_nonmeasure_differs')
    for field, value in [('n', 8000), ('s', 10), ('cache_entries', 2), ('alias_policy', 'eager'),
                         ('max_meb_calls_per_order', 4000001), ('kmax_requested', 10)]:
        rejected('config_' + field, lambda c, f=field, v=value: c[2].__setitem__(f, v), 'pair_configuration_differs')
    rejected('proposal_cap', lambda c: c[2].__setitem__(CAP, 101), 'accounting_or_cap_binding')
    rejected('input_digest', lambda c: c[3]['terminal'].__setitem__('input_digest', sha(b'other')), 'pair_input_differs')
    rejected('global_digest', lambda c: c[3]['terminal'].__setitem__('certificate_digest', sha(b'other')),
             'terminal_nonmeasure_or_work_prefix_differs')
    for name, field, value, reason in [('counter_bool', 'meb_calls', True, 'integer_field'),
            ('counter_float', 'meb_calls', 0.0, 'integer_field'), ('schema', 'schema', 'v3', 'accounting_or_cap_binding'),
            ('calendar', 'successor_accounting', 'v1', 'accounting_or_cap_binding')]:
        rejected(name, lambda c, f=field, v=value: c[3]['orders'][0].__setitem__(f, v), reason)
    partial = model()
    refusal(partial[1])
    refusal(partial[3])
    rejected('refused_work_prefix', lambda c: c[3]['terminal']['last_order_work'].__setitem__('meb_calls', 1),
             'terminal_nonmeasure_or_work_prefix_differs', partial)
    rejected('partial_promotion_flag', lambda c: c[3]['terminal'].__setitem__('complete_requested_horizontal_orders', True),
             'terminal_promotion', partial)
    rejected('partial_promotion_digest', lambda c: c[3]['terminal'].__setitem__('certificate_digest', sha(b'bad')),
             'refusal_promotion', partial)
    rejected('require_complete_on_refusal', lambda c: None, 'complete_pair_required', partial, True)
    rejected('censored_exit', lambda c: c[3].__setitem__('exit_code', 124), 'terminal_promotion')
    rejected('missing_terminal', lambda c: c[3].__setitem__('terminal', None), 'censored_or_missing_terminal')
    rejected('unclosed_capture', lambda c: c[3].__setitem__('status', 'failed'), 'capture_not_closed')
    require(len(positives) == 5 and len(mutants) == 25, 'model_non_vacuity')
    files = {name: sha(name.encode()) for name in FILES.values()}
    source = dict(files=files, source_sha256=sha((json.dumps(files, sort_keys=True, indent=2)+'\n').encode()),
                  binary='build/model/not_an_ELF', binary_sha256=sha(b'binary_model'),
                  probe_schema=SCHEMA, successor_accounting=SUCCESSOR)
    protocol = dict(source, meb_accounting=ACCOUNTING, judge_sha256=PRIMARY_SHA, first_c_sha256=FIRST_C_SHA,
                    **{field: files[name] for field, name in FILES.items()})
    source_binding(source, protocol)
    pair_sources(source, copy.deepcopy(source))
    provenance_mutants = []
    for name, target, field, value, reason in [
            ('map_digest', 'source', 'source_sha256', sha(b'bad'), 'source_map_digest'),
            ('producer_pin', 'protocol', 'producer_sha256', sha(b'bad'), 'source_protocol_binding'),
            ('meb_header_pin', 'protocol', 'meb_header_sha256', sha(b'bad'), 'source_protocol_binding'),
            ('binary_pin', 'protocol', 'binary_sha256', sha(b'bad'), 'binary_source_binding'),
            ('schema_binding', 'protocol', 'probe_schema', 'v3', 'source_accounting_binding'),
            ('primary_pin', 'protocol', 'judge_sha256', sha(b'bad'), 'fixed_judge_protocol_binding'),
            ('first_c_pin', 'protocol', 'first_c_sha256', sha(b'bad'), 'fixed_judge_protocol_binding')]:
        s, p = copy.deepcopy(source), copy.deepcopy(protocol)
        (s if target == 'source' else p)[field] = value
        try:
            source_binding(s, p)
        except ValueError as error:
            require(str(error) == reason, 'wrong_provenance_mutant_reason:' + name)
            provenance_mutants.append(name)
            continue
        raise ValueError('accepted_provenance_mutant:' + name)
    changed = copy.deepcopy(source)
    changed['binary_sha256'] = sha(b'other_binary')
    try:
        pair_sources(source, changed)
    except ValueError as error:
        require(str(error) == 'pair_source_map_or_captured_binary_differs', 'wrong_pair_binary_reason')
        provenance_mutants.append('different_pair_binary')
    require(len(provenance_mutants) == 8, 'provenance_non_vacuity')
    return dict(schema='mhgp7-full-meb-pair-models-v1', status='passed', positives=positives,
                mutants=mutants, positive_count=len(positives), mutant_count=len(mutants),
                provenance_positive_count=1, provenance_mutants=provenance_mutants,
                provenance_mutant_count=len(provenance_mutants),
                scope='pure_comparison_models_not_engine_receipts_or_primary_judge_qualification',
                engine_invoked=False, global_FULL_successful=False, public_status='not_claimed')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selftest', action='store_true')
    for arm in ('baseline', 'opt-in'):
        for kind in ('receipt', 'protocol', 'sources'):
            parser.add_argument('--' + arm + '-' + kind, metavar='PATH=SHA256')
    parser.add_argument('--primary', metavar='PATH=SHA256')
    parser.add_argument('--first-c', metavar='PATH=SHA256')
    parser.add_argument('--expected-self-sha256')
    parser.add_argument('--proposal-cap', type=int)
    parser.add_argument('--require-complete', action='store_true')
    args = parser.parse_args()
    try:
        if args.selftest:
            require(not any(v for k, v in vars(args).items() if k != 'selftest'), 'selftest_has_no_capture_arguments')
            result = selftest()
        else:
            require(all(v is not None for k, v in vars(args).items() if k not in {'selftest', 'require_complete'}),
                    'explicit_pair_arguments_required')
            require(is_sha(args.expected_self_sha256), 'explicit_self_pin_required')
            reader = Reader()
            reader.read(Path(__file__).absolute(), args.expected_self_sha256)
            primary = load_module(reader, args.primary, PRIMARY_SHA, '_mhgp7_meb_pair_primary')
            first_c = load_module(reader, args.first_c, FIRST_C_SHA, '_mhgp7_meb_pair_first_c')
            lc, left, ls, la = load_arm(reader, args.baseline_receipt, args.baseline_protocol,
                                      args.baseline_sources, primary, first_c)
            rc, right, rs, ra = load_arm(reader, args.opt_in_receipt, args.opt_in_protocol,
                                       args.opt_in_sources, primary, first_c)
            pair_sources(ls, rs)
            equal(left['cwd'], right['cwd'], 'pair_capture_cwd_differs')
            result = compare_objects(lc, left, rc, right, args.proposal_cap, args.require_complete)
            result.update(baseline_admission=la, opt_in_admission=ra, binary_sha256=ls['binary_sha256'],
                binary_identity_scope='captured_hash_only_no_ELF_read_or_build_requalification',
                source_identity_scope='captured_source_maps_before_after_not_hermetic_build_proof',
                primary_sha256=PRIMARY_SHA, first_c_sha256=FIRST_C_SHA,
                gnu_peak_rss_distinct_from_sampled_rss=True)
            reader.stable()
            result['inputs_stable'] = True
            result['input_files'] = {str(p): dict(sha256=pin, bytes=size)
                                     for p, (pin, size) in sorted(reader.watches.items())}
        print(json.dumps(result, sort_keys=True, indent=2, allow_nan=False))
        return 0
    except (ValueError, KeyError, TypeError, OSError, UnicodeError, OverflowError, RecursionError) as error:
        print(json.dumps(dict(schema='mhgp7-full-meb-pair-comparison-v1', comparison_status='invalid',
            reason=str(error), global_FULL_successful=False, public_status='not_claimed', engine_invoked=False),
            sort_keys=True, indent=2))
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
