"""Bounded in-memory adversarial review; never executes native code or writes origins."""
from copy import deepcopy
from hashlib import sha256
import importlib.util
import json
from math import nextafter, inf
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
ORIGIN = Path('/workspaces/E-HGP/build/v10-relative-filter-audit-20260930.n81Su40Q')
EXPECTED = {
    'judge.py': '11b45a27025040e345a8d297813e56c50fdbea813bcbd4041fba1ecae9cc49cb',
    'requests.json': '0e21bdd1fa4eb1d6e0670bb571253f33a0e569db00e98ec80a65f805be608cd5',
    'normal.stdout': '6aa46b6400aa5fc74613a2496aad8a96fc5603dbe4a9a6ef1081d62498ef77d1',
    'ubsan.stdout': '6aa46b6400aa5fc74613a2496aad8a96fc5603dbe4a9a6ef1081d62498ef77d1',
}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def hashes():
    return {name: sha256((ORIGIN / name).read_bytes()).hexdigest() for name in EXPECTED}


before = hashes()
require(before == EXPECTED, 'origin changed before audit')
spec = importlib.util.spec_from_file_location('original_relative_judge', ORIGIN / 'judge.py')
judge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(judge)
requests = judge.load((ORIGIN / 'requests.json').read_text())
answers = [judge.load(line) for line in (ORIGIN / 'normal.stdout').read_text().splitlines()]
require(len(requests) == len(answers) == 3600, 'unexpected fixed panel')
baseline = judge.judge(requests, answers)
require(baseline['status'] == 'PASS' and baseline['exact_rank_checks'] == 147,
        'baseline did not pass')


def bits(value):
    return struct.unpack('<Q', struct.pack('<d', value))[0]


def number(value):
    return struct.unpack('<d', struct.pack('<Q', value))[0]


site = next(i for i, row in enumerate(requests) if row['op'] == 'S')
box = next(i for i, row in enumerate(requests) if row['op'] == 'B')
inside = next(i for i, row in enumerate(answers) if row['status'] == 'INTERIOR')
outside = next(i for i, row in enumerate(answers) if row['status'] == 'EXTERIOR')
contact = next(i for i, row in enumerate(requests) if row.get('exact_contact'))
translation = next(i for i, row in enumerate(requests) if 'equal_to' in row)
inexact_c = next(i for i, row in enumerate(answers)
                 if row['op'] == 'C' and row['value'][0] != row['value'][1])
radius_span = next(i for i, row in enumerate(answers)
                   if row['op'] != 'C' and row['radius'][0] != row['radius'][1])
distance_span = next(i for i, row in enumerate(answers)
                     if row['op'] != 'C' and row['distance'][0] != row['distance'][1])
rho_span = next((i, axis) for i, row in enumerate(answers) if row['op'] != 'C'
                for axis in range(3) if row['rho'][axis][0] != row['rho'][axis][1])

cases = []


def answer_case(name, index, field=None, value=None, delete=False):
    def mutate(rs, rows):
        if delete:
            del rows[index][field]
        else:
            rows[index][field] = deepcopy(value)
    cases.append((name, mutate, True, 'answer'))


def add(name, mutation, reject=True, scope='answer'):
    cases.append((name, mutation, reject, scope))


add('missing_first_conversion_line', lambda rs, rows: rows.pop(0))
add('missing_last_site_line', lambda rs, rows: rows.pop())
add('missing_all_conversion_lines',
    lambda rs, rows: rows.__setitem__(slice(None), [row for row in rows if row['op'] != 'C']))
add('extra_duplicate_line', lambda rs, rows: rows.append(deepcopy(rows[0])))
add('swapped_signed_conversion_lines',
    lambda rs, rows: rows.__setitem__(slice(1, 3), [deepcopy(rows[2]), deepcopy(rows[1])]))
answer_case('missing_op', 0, 'op', delete=True)
answer_case('wrong_op', 0, 'op', 'S')
answer_case('missing_conversion_status', 0, 'status', delete=True)
answer_case('misleading_conversion_status', 0, 'status', 'PASS')
answer_case('missing_conversion_value', 0, 'value', delete=True)
answer_case('ready_but_wrong_conversion', 1, 'value', [0, 0])
answer_case('conversion_interval_wrong_shape', 1, 'value', [0])
answer_case('conversion_interval_reversed', inexact_c, 'value',
            list(reversed(answers[inexact_c]['value'])))
answer_case('conversion_boolean_endpoint', 0, 'value', [False, 0])
answer_case('conversion_nan_endpoint', 0, 'value', [float('nan'), 0])
answer_case('conversion_nan_bits', 0, 'value', [0x7ff8000000000001]*2)
answer_case('conversion_inf_bits', 0, 'value', [0x7ff0000000000000]*2)
for field in ('rho', 'radius', 'distance', 'status'):
    answer_case('missing_site_'+field, site, field, delete=True)
    answer_case('missing_box_'+field, box, field, delete=True)
answer_case('rho_wrong_shape', site, 'rho', answers[site]['rho'][:2])
answer_case('rho_nan_bits', site, 'rho', [[0x7ff8000000000001]*2]*3)
answer_case('rho_inf_bits', site, 'rho', [[0x7ff0000000000000]*2]*3)
answer_case('radius_nan_bits', site, 'radius', [0x7ff8000000000001]*2)
answer_case('radius_inf_bits', site, 'radius', [0x7ff0000000000000]*2)
answer_case('distance_nan_bits', site, 'distance', [0x7ff8000000000001]*2)
answer_case('distance_inf_bits', site, 'distance', [0x7ff0000000000000]*2)
answer_case('radius_interval_reversed', radius_span, 'radius',
            list(reversed(answers[radius_span]['radius'])))
answer_case('distance_interval_reversed', distance_span, 'distance',
            list(reversed(answers[distance_span]['distance'])))
ri, axis = rho_span
changed_rho = deepcopy(answers[ri]['rho'])
changed_rho[axis].reverse()
answer_case('rho_interval_reversed', ri, 'rho', changed_rho)
answer_case('radius_negative_lower', site, 'radius',
            [bits(-1.), answers[site]['radius'][1]])
answer_case('distance_negative_lower', site, 'distance',
            [bits(-1.), answers[site]['distance'][1]])
answer_case('false_exterior_on_interior', inside, 'status', 'EXTERIOR')
answer_case('false_interior_on_exterior', outside, 'status', 'INTERIOR')
answer_case('box_all_inside_certificate', box, 'status', 'INTERIOR')
answer_case('contact_interior', contact, 'status', 'INTERIOR')
answer_case('contact_exterior', contact, 'status', 'EXTERIOR')
answer_case('query_ready_status', site, 'status', 'READY')
answer_case('query_status_nan', site, 'status', float('nan'))
answer_case('query_status_inf', site, 'status', float('inf'))
add('all_queries_ambiguous_unchanged_intervals',
    lambda rs, rows: [row.__setitem__('status', 'AMBIGU') for row in rows if row['op'] != 'C'])


def make_uninformative(rs, rows):
    positive = [bits(0.), 0x7fefffffffffffff]
    signed = [0xffefffffffffffff, 0x7fefffffffffffff]
    for row in rows:
        if row['op'] != 'C':
            row['status'] = 'AMBIGU'
            row['rho'] = [signed[:] for _ in range(3)]
            row['radius'] = positive[:]
            row['distance'] = positive[:]


add('all_queries_ambiguous_valid_broad_enclosures_floor', make_uninformative)


def alter_translation(rs, rows):
    old = rows[translation]['rho'][0]
    rows[translation]['rho'][0] = [
        bits(nextafter(number(old[0]), -inf)),
        bits(nextafter(number(old[1]), inf)),
    ]


add('translation_bit_change_still_enclosing', alter_translation)
add('request_missing_id', lambda rs, rows: rs[0].pop('id'), scope='trusted_request_mutation')
add('request_wrong_id', lambda rs, rows: rs[0].__setitem__('id', 10),
    scope='trusted_request_mutation')
add('request_boolean_zero_id', lambda rs, rows: rs[0].__setitem__('id', False),
    reject=False, scope='trusted_request_typing_outside_output_contract')
add('removed_one_translation_marker',
    lambda rs, rows: rs[translation].pop('equal_to'), scope='trusted_request_mutation')
add('removed_all_contact_markers',
    lambda rs, rows: [row.pop('exact_contact', None) for row in rs],
    scope='trusted_request_mutation')
add('spoofed_contact_marker',
    lambda rs, rows: rs[inside].__setitem__('exact_contact', True),
    scope='trusted_request_mutation')
add('removed_one_contact_marker',
    lambda rs, rows: rs[contact].pop('exact_contact'), reject=False,
    scope='trusted_request_marker_outside_output_contract')
add('unused_extra_nonfinite_field',
    lambda rs, rows: rows[0].__setitem__('unused_extra', float('nan')),
    reject=False, scope='ignored_unknown_field_not_a_numeric_certificate')

results = []
failures = []
for name, mutate, expect_reject, scope in cases:
    rs, rows = deepcopy(requests), deepcopy(answers)
    mutate(rs, rows)
    try:
        result = judge.judge(rs, rows)
        outcome = dict(rejected=False, returned_status=result.get('status'),
                       counts=result.get('counts'), exact_rank_checks=result.get('exact_rank_checks'))
    except Exception as error:
        outcome = dict(rejected=True, error_type=type(error).__name__, error=str(error))
    expected = outcome['rejected'] == expect_reject
    if name == 'all_queries_ambiguous_valid_broad_enclosures_floor':
        expected = expected and outcome.get('error') == 'non-vacuity floors'
    if name == 'translation_bit_change_still_enclosing':
        expected = expected and outcome.get('error') == 'translation changes native bits'
    if not expected:
        failures.append(name)
    results.append(dict(name=name, scope=scope, expected_rejection=expect_reject,
                        expectation_met=expected, **outcome))
try:
    judge.load('{"op":"C","op":"S"}')
    duplicate_refused = False
except ValueError:
    duplicate_refused = True
require(duplicate_refused, 'duplicate JSON key not refused')
require(before == hashes(), 'origin changed during audit')
output = dict(status='PASS' if not failures else 'FAIL',
              scope='bounded in-memory output judge audit, not a general request/schema validator',
              optimize_flag=sys.flags.optimize, baseline=baseline,
              cases=len(results), expected_rejections=sum(row['expected_rejection'] for row in results),
              observed_rejections=sum(row['rejected'] for row in results),
              explicitly_scoped_acceptances=[row['name'] for row in results if not row['rejected']],
              duplicate_json_key_refused=duplicate_refused,
              results=results, failures=failures,
              original_source_and_outputs_unchanged=before == hashes(), origin=str(ORIGIN),
              source_hashes=before, native_executions=0, GCP_used=False)
print(json.dumps(output, allow_nan=False, sort_keys=True, indent=2))
raise SystemExit(bool(failures))

