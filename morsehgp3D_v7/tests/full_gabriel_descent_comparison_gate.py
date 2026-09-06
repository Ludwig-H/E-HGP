#!/usr/bin/env python3
"""Independent rational descent calendars: exact models, not engine timings.

The explicitly pinned quotient oracle is the only imported math helper.
P=0/F support ordinals are model counts, never latency or product qualification.
"""
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import hashlib
import importlib.util
import json
import sys

ROOT = Path(__file__).resolve().parents[2]
GATE = ROOT / 'morsehgp3D_v7/tests/full_gabriel_minima_quotient_gate.py'
PIN = 'bee615b5f8b937e11104597fd674d868828d6b850616582f5163b44454ab9434'


def require(ok, why):
    if not ok:
        raise ValueError(why)


def load():
    require(hashlib.sha256(GATE.read_bytes()).hexdigest() == PIN, 'frozen_oracle_pin')
    spec = importlib.util.spec_from_file_location('quotient_oracle', GATE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def morton(point):
    return sum(((point[axis] >> bit) & 1) << (3 * bit + axis)
               for bit in range(16) for axis in range(3))


def text(label):
    return ''.join(chr(65 + i) for i in label)


def intruders(gate, model, label):
    radius, center, _ = model['balls'][label]
    points = model['points']
    return sorted((i for i, p in enumerate(points) if i not in label and
                   gate.dot(gate.subtract(p, center), gate.subtract(p, center)) < radius),
                  key=lambda i: morton(points[i]))


def ordinal(label, support):
    # Exactly the charged combination positions of F at P=0 on regular inputs.
    # This does NOT count arithmetic, early containment exits or bit complexity.
    count = 0
    for size in range(2, min(4, len(label)) + 1):
        for candidate in combinations(label, size):
            count += 1
            if candidate == support:
                return count
    raise ValueError('support_ordinal_missing')


def component(gate, model, facet, cut, closed=False):
    groups = gate.partition(model['facets'], model['elementary'], model['births'], cut, closed)
    group = next(group for group in groups if facet in group)
    return tuple(g for g in group if g in model['minima'])


def resolve(gate, model, initial, cut, variant):
    balls, gabriel = model['balls'], model['gabriel']
    require(balls[initial][0] < cut, 'strict_requested_facet')
    work = dict(meb_calls=0, meb_k_calls=0, meb_k_plus_1_calls=0,
                f_support_ordinals=0, census_calls=0, descent_steps=0,
                minimum_lookups=0, direct_lookups=0, singleton_J1=0)
    labels, path = [], [initial]
    def meb(label):
        work['meb_calls'] += 1
        work['meb_k_calls' if len(label) == model['order'] else 'meb_k_plus_1_calls'] += 1
        work['f_support_ordinals'] += ordinal(label, balls[label][2])
        labels.append(label)
    def census(label):
        work['census_calls'] += 1
        result = intruders(gate, model, label)
        require(bool(result), 'missing_minimum_or_terminal')
        return result
    terminal = initial
    if variant == 'facets':
        while True:
            work['minimum_lookups'] += 1
            if terminal in gabriel:
                require(balls[terminal][0] < cut, 'terminal_minimum_prior')
                break
            meb(terminal)
            z = census(terminal)[0]
            removed = balls[terminal][2][0]
            following = tuple(sorted((set(terminal) - {removed}) | {z}))
            require(balls[following][0] < balls[terminal][0], 'facet_strict_descent')
            require(balls[tuple(sorted(set(terminal) | {z}))][0] == balls[terminal][0],
                    'same_level_elementary_path')
            terminal = following
            path.append(terminal)
            work['descent_steps'] += 1
            require(len(path) <= len(model['facets']), 'finite_facet_bound')
        anchor = terminal
    else:
        work['minimum_lookups'] += 1
        if initial in gabriel:
            anchor = initial
        else:
            meb(initial)
            foreign = census(initial)
            terminal = tuple(sorted(initial + (foreign[0],)))
            require(balls[terminal][0] == balls[initial][0]
                    and balls[terminal][2] == balls[initial][2], 'Q0_inherited_ball')
            path.append(terminal)
            if len(foreign) == 1:
                work['direct_lookups'] += 1
                work['singleton_J1'] += 1
                require(terminal in gabriel, 'J1_direct_terminal')
            else:
                next_intruder = foreign[1]
                while True:
                    previous = balls[terminal][0]
                    removed = balls[terminal][2][0]
                    terminal = tuple(sorted((set(terminal) - {removed}) | {next_intruder}))
                    work['descent_steps'] += 1
                    path.append(terminal)
                    meb(terminal)
                    require(balls[terminal][0] < previous, 'coface_strict_descent')
                    work['direct_lookups'] += 1
                    if terminal in gabriel:
                        break
                    next_intruder = census(terminal)[0]
                    require(len(path) <= len(balls), 'finite_coface_bound')
            require(balls[terminal][0] < cut, 'direct_terminal_prior')
            strict = tuple(i for i in terminal if i != balls[terminal][2][0])
            anchor = min(component(gate, model, strict, balls[terminal][0], True))
    require(component(gate, model, anchor, cut) == component(gate, model, initial, cut),
            'normalized_parent_equivalence')
    return dict(work=work, path=[text(label) for label in path],
                meb_labels=[text(label) for label in labels], terminal=text(terminal), anchor=text(anchor))


def compare(gate, model, capacity):
    requests = [(coface, tuple(i for i in coface if i != removed), model['balls'][coface][0])
                for coface in sorted((label for label in model['gabriel']
                                      if len(label) == model['order'] + 1),
                                     key=lambda label: (model['balls'][label][0], label))
                for removed in model['balls'][coface][2]]
    cache, rows, hits, initial_minima = set(), [], 0, 0
    for coface, facet, cut in requests:
        if facet in model['minima']:
            initial_minima += 1
            continue
        if facet in cache:
            hits += 1
            continue
        old = resolve(gate, model, facet, cut, 'cofaces')
        new = resolve(gate, model, facet, cut, 'facets')
        rows.append(dict(coface=text(coface), facet=text(facet), cut=str(cut), old=old, new=new))
        if len(cache) < capacity:
            cache.add(facet)
    totals = {}
    for variant in ('old', 'new'):
        totals[variant] = {field: sum(row[variant]['work'][field] for row in rows)
                           for field in ('meb_calls', 'meb_k_calls', 'meb_k_plus_1_calls',
                                         'f_support_ordinals', 'census_calls', 'descent_steps',
                                         'minimum_lookups', 'direct_lookups', 'singleton_J1')}
        totals[variant]['minimum_lookups'] += initial_minima + hits
    return dict(order=model['order'], capacity=capacity, requests=len(requests),
                initial_minimum_hits=initial_minima, cache_hits=hits, misses=len(rows),
                cache_entries=len(cache), totals=totals, traces=rows)


def main(argv):
    require(argv == ['--selftest'], 'invalid_arguments')
    gate = load()
    rows = []
    for scene, points in gate.SCENES.items():
        for order in range(1, len(points) + 1):
            model = gate.data(points, order)
            for capacity in (0, 1, 1000000):
                row = compare(gate, model, capacity)
                row['scene'] = scene
                rows.append(row)
    require(len(rows) == 51 and sum(row['misses'] for row in rows) == 12, 'nonvacuity')
    counter_points = [(0, 3, 3), (3, 2, 9), (8, 6, 12), (12, 9, 3), (13, 6, 11)]
    counter_model = gate.data(counter_points, 2)
    counter = compare(gate, counter_model, 0)
    witness = next(row for row in counter['traces'] if row['coface'] == 'ABD' and row['facet'] == 'BD')
    require(witness['cut'] == '1909/41' and witness['old']['path'] == ['BD', 'BCD']
            and witness['new']['path'] == ['BD', 'CD', 'DE'], 'longer_facet_path')
    require(witness['old']['work']['meb_calls'] == 1 and witness['new']['work']['meb_calls'] == 2
            and witness['old']['work']['singleton_J1'] == 1, 'more_MEB_than_J1')
    examples = [row for row in rows if row['capacity'] == 0 and row['scene'] == 'E5_silent_cofaces'
                and row['order'] == 2]
    require(len(examples) == 1 and examples[0]['totals']['old']['f_support_ordinals'] == 5
            and examples[0]['totals']['new']['f_support_ordinals'] == 1, 'smaller_F_work_E5')
    require(hashlib.sha256(GATE.read_bytes()).hexdigest() == PIN, 'oracle_stable')
    return dict(schema='mhgp7-rational-resolver-descent-comparison-v1', status='passed',
                public_status='not_claimed', engine_invoked=False, timing_claim=False,
                source_sha256=PIN, models=17, cache_configurations=3, rows=rows,
                additional_longer_path_fixture=dict(points=counter_points, result=counter),
                scope='exact_conditional_calendars_no_query_nodes_successor_reads_or_walltime')


if __name__ == '__main__':
    try:
        print(json.dumps(main(sys.argv[1:]), sort_keys=True))
    except (ValueError, KeyError, StopIteration) as error:
        print(json.dumps(dict(status='failed', reason=str(error), public_status='not_claimed'), sort_keys=True))
        sys.exit(2 if str(error) == 'invalid_arguments' else 1)
