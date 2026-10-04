#!/usr/bin/env python3
"""Bounded source-pinned Python reader replay, not a native scheduling run."""
import ast
import copy
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PIN = 'e49ea46907e3c4bb9e0942e2205cf490748136e4'
SOURCE = ROOT / 'source/bench/full_acceleration_diagnostics.py'
checks = 0


def check(value, message):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(message)


class Rejected(Exception):
    pass


def need(value, message):
    if not value:
        raise Rejected(message)


def unsigned(row, fields):
    for key in fields:
        need(type(row[key]) is int and 0 <= row[key] < 2**64, 'unsigned ' + key)


before = json.loads((ROOT / 'BEFORE.json').read_text())
source = SOURCE.read_bytes()
source_sha = hashlib.sha256(source).hexdigest()
check(before['pin'] == PIN, 'pin')
check(source_sha == before['files']['bench/full_acceleration_diagnostics.py']['sha256'], 'copied source hash')
module = ast.parse(source)
actual = {}
exec(compile(module, str(SOURCE), 'exec'), actual)

# Replace precisely the one false invariant in a private AST copy. The copied source stays unchanged.
guard = "tasks['lanes_last_start_ns'] <= tasks['lanes_first_finish_ns'] <= full['forest_ns']"
new_guard = ("tasks['lanes_last_start_ns'] <= full['forest_ns'] and "
             "tasks['lanes_first_finish_ns'] <= full['forest_ns']")
patches = 0


class ReplaceGuard(ast.NodeTransformer):
    def visit_Compare(self, node):
        global patches
        if ast.unparse(node) == guard:
            patches += 1
            return ast.copy_location(ast.parse(new_guard, mode='eval').body, node)
        return self.generic_visit(node)


changed = ReplaceGuard().visit(copy.deepcopy(module))
check(patches == 1, 'single exact guard AST replacement')
proposed = {}
exec(compile(ast.fix_missing_locations(changed), '<private reader guard model>', 'exec'), proposed)

# Admissible task chronology. The Pool's first claiming thread can complete lane0 before lane1 starts.
# No barrier enforces simultaneous lane starts. Resolution lane0 can consume all blocks; lane1 is then empty.
starts, finishes = [1, 10], [5, 11]
check(all(a <= b for a, b in zip(starts, finishes)), 'each lane ends after its own start')
check(max(starts) > min(finishes), 'global invariant false on serial scheduling')
schedules = 0
for lanes in (2, 3, 5, 39, 47):
    for blocks in (0, 1, 2, 40):
        start = list(range(1, 2 * lanes, 2))
        finish = [value + 1 for value in start]
        next_block, counts = 0, []
        for lane in range(lanes):
            taken = 0
            while next_block < blocks:
                next_block += 1
                taken += 1
            counts.append(taken)
        check(sum(counts) == blocks and next_block == blocks, 'each block consumed exactly once')
        check(counts[1:] == [0] * (lanes - 1), 'late empty lanes legal')
        check(all(a <= b for a, b in zip(start, finish)), 'per-lane clock order')
        check(max(start) > min(finish), 'serial run invalidates all-start-before-any-finish')
        schedules += 1

order_fields = actual['PIPELINE_ORDER']
phases = {'classify_ns': 1, 'births_ns': 1, 'regular_ns': 11, 'publish_ns': 8, 'verticals_ns': 1}
orders = []
for k, tail in ((1, 8), (2, 7)):
    orders.append({'k': k, 'work': {'population_hits': 0},
                   'timings': {'classify_ns': 0, 'births_ns': 0, 'plateaus_ns': tail,
                               'verticals_ns': 0 if k == 1 else 1}})
rows = [dict.fromkeys(order_fields, 0) for _ in orders]
rows[0].update(k=1, publish_start_ns=12, publish_cpu_ns=4, publish_wait_ns=1)
rows[1].update(k=2, publish_start_ns=14, publish_cpu_ns=1, publish_wait_ns=0,
               vertical_start_ns=17, vertical_cpu_ns=1, vertical_wait_ns=1)
event = {'optimizations': 8192, 'population_lookup': False, 'concurrent_orders': True,
         'population_lookup_entries': 0, 'population_lookup_reserved_bytes': 0,
         'reserved_after_bytes': 0, 'memo_reserved_bytes': 0, 'parallel': {'lane_memo_reserved_bytes': 0},
         'census_workspace_reserved_bytes': 0, 'regular_vertical_reserved_bytes': 0,
         'peak_reserved_bytes': 0, 'kmax': 2, 'forest_ns': 30, 'phases': phases, 'orders': orders,
         'pipeline_tasks': {'lanes_last_start_ns': max(starts), 'lanes_first_finish_ns': min(finishes),
                            'lanes_cpu_ns': 3, 'orders': rows}}


def result(reader, value):
    try:
        reader['validate'](value, need, unsigned)
    except Rejected as error:
        return {'accepted': False, 'message': str(error)}
    return {'accepted': True}


current = result(actual, event)
fixed = result(proposed, event)
check(current == {'accepted': False, 'message': 'lane starts precede their first finish, within the forest wall'},
      'real reader rejects solely false invariant')
check(fixed == {'accepted': True}, 'all remaining real validation checks pass')
control = copy.deepcopy(event)
control['pipeline_tasks']['lanes_last_start_ns'] = 4
check(result(actual, control) == {'accepted': True}, 'same event with invariant-compatible starts passes')

negative = []
for name, field, value in [('late_start', 'lanes_last_start_ns', 31),
                           ('late_finish', 'lanes_first_finish_ns', 31),
                           ('negative_start', 'lanes_last_start_ns', -1),
                           ('bool_cpu', 'lanes_cpu_ns', True)]:
    invalid = copy.deepcopy(event)
    invalid['pipeline_tasks'][field] = value
    answer = result(proposed, invalid)
    check(not answer['accepted'], 'proposed reader rejects ' + name)
    negative.append({'case': name, **answer})
invalid = copy.deepcopy(event)
invalid['pipeline_tasks']['orders'][0]['vertical_wait_ns'] = 1
answer = result(proposed, invalid)
check(not answer['accepted'], 'order1 vertical remains forbidden')
negative.append({'case': 'order1_vertical', **answer})
staged = copy.deepcopy(event)
staged.update(optimizations=0, concurrent_orders=False, phases=dict.fromkeys(actual['PHASES'], 0))
staged['pipeline_tasks'] = {**dict.fromkeys(actual['PIPELINE_LANES'], 0),
                           'orders': [dict.fromkeys(order_fields, 0) | {'k': k} for k in (1, 2)]}
check(result(actual, staged) == result(proposed, staged) == {'accepted': True}, 'staged zero fields remain accepted')
staged['pipeline_tasks']['orders'][1]['publish_cpu_ns'] = 1
check(not result(proposed, staged)['accepted'], 'staged nonzero task timing remains rejected')

# Tail is not a task's duration: max(0,E-R) loses E when E<=R.
r, first_e, second_e, start = 20, 7, 19, 2
check(max(0, first_e-r) == max(0, second_e-r) == 0, 'zero tails indistinguishable')
check(first_e-start != second_e-start, 'full task durations differ despite equal exported tail')

print(json.dumps({'status': 'PASS', 'scope': 'Python reader replay + scalar scheduler model; no native execution',
                  'pin': PIN, 'source_sha256': source_sha, 'checks': checks, 'serial_schedules': schedules,
                  'chronology_ns': {'starts': starts, 'finishes': finishes}, 'reader_before': current,
                  'private_guard_model': fixed, 'remaining_invalid_metadata': negative,
                  'same_event_non_guard_metadata': event,
                  'tail_counterexample_ns': {'resolution_end': r, 'task_start': start,
                                            'possible_task_ends': [first_e, second_e], 'exported_tail': 0}},
                 indent=2, sort_keys=True))
