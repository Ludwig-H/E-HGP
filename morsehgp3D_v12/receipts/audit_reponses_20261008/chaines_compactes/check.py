#!/usr/bin/env python3
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess
from model import compact, collect_original, graph_oracle, check_counts, segmented, U32_NONE, U64_MAX

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def case(states, rng):
    # 0: arête certaine, 1: incertaine ordonnée, 2: incertaine inversée.
    keys = [0x3ff0000000000000]
    for x in states:
        keys.append(keys[-1] + (1 << 20 if x == 0 else 1))
    order = list(range(len(keys))); rng.shuffle(order)
    return [0] + [int(x == 2) for x in states], order, keys


def packed(bounds, order, keys):
    return bounds, [order[i] for s, e in bounds for i in range(s, e)], [keys[i] for s, e in bounds for i in range(s, e)]


def replay(repo):
    pins = json.loads((HERE / 'pins.json').read_text())
    for item in pins['sources']:
        raw = subprocess.check_output(['git', 'show', item['pin'] + ':' + item['path']], cwd=repo)
        need(hashlib.sha256(raw).hexdigest() == item['sha256'], 'source changed')
    rng, total, max_work = random.Random(880731), 0, 0
    killed = {}

    def run(v, o, k, label):
        nonlocal total, max_work
        expected = graph_oracle(v, o, k)
        need(collect_original(v, o, k) == expected, 'reference vs graph')
        got, work = compact(v, o, k)
        need(got == expected, 'scan vs graph')
        total += 1; max_work = max(max_work, work)
        bounds = expected[0]
        pairs = [(i - 1, i + 1) for i in range(1, len(k)) if v[i] == 1]
        mutants = {
            'pairs_only': packed(pairs, o, k),
            'drop_left_endpoint': packed([(s + 1, e) for s, e in bounds], o, k),
            'drop_right_endpoint': packed([(s, e - 1) for s, e in bounds], o, k),
            'repeat_component_per_inversion': packed([b for b in bounds for i in range(b[0] + 1, b[1]) if v[i]], o, k),
            'cross_certain_boundaries': packed([(bounds[0][0], bounds[-1][1])] if bounds else [], o, k),
            'first_component_only': packed(bounds[:1], o, k),
            'reverse_packed_payload': (bounds, list(reversed(expected[1])), list(reversed(expected[2]))),
            'select_ordered_uncertainty': graph_oracle([0] + [int(b - a == 1) for a, b in zip(k, k[1:])], o, k)
        }
        for name, value in mutants.items():
            if value != expected and name not in killed:
                killed[name] = label

    run([], [], [], 'empty')
    for n in range(1, 10):
        for states in itertools.product(range(3), repeat=n - 1):
            run(*case(states, rng), 'exhaustive_n%d_%s' % (n, ''.join(map(str, states))))
    for i in range(600):
        n = rng.choice([2, 3, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255])
        states = [rng.randrange(3) for _ in range(n - 1)]
        if i % 4 == 0:
            states = [1] * (n - 1); states[rng.randrange(n - 1)] = 2
        run(*case(states, rng), 'random_%d_n%d' % (i, n))
    for n in (31, 32, 33, 1023, 1024, 1025, 2049):
        states = [1] * (n - 1); states[n // 2] = 2
        run(*case(states, rng), 'whole_component_boundary_n%d' % n)
    need(len(killed) == 8, 'surviving mutant')
    states = list(itertools.product([False, True], repeat=2))
    associativity = 0
    for a, b, c in itertools.product(states, repeat=3):
        need(segmented(segmented(a, b), c) == segmented(a, segmented(b, c)), 'not associative')
        associativity += 1
    refusals = {}
    v, o, k = case([2, 1], rng)
    probes = {
        'duplicate_verdict': lambda: compact([0, 2, 0], o, k),
        'bool_verdict': lambda: compact([0, True, 0], o, k),
        'unknown_verdict': lambda: compact([0, 3, 0], o, k),
        'fault_duplicate': lambda: compact(v, o, k, fault=2),
        'fault_missing': lambda: compact(v, o, k, fault=0),
        'first_verdict': lambda: compact([1, 0, 0], o, k),
        'unsorted_keys': lambda: compact(v, o, list(reversed(k))),
        'certain_inversion': lambda: compact([0, 1], [0, 1], [k[0], k[0] + (1 << 20)]),
        'nonfinite': lambda: compact([0], [0], [0x7ff0000000000000]),
        'duplicate_id': lambda: compact(v, [0, 0, 2], k),
        'length': lambda: compact(v[:-1], o, k),
        'element_capacity': lambda: compact(v, o, k, max_elements=2),
        'chain_capacity': lambda: compact(v, o, k, max_chains=0),
        'payload_budget': lambda: compact(v, o, k, payload_budget=67),
        'u32_sentinel': lambda: check_counts(U32_NONE, 0, 0, U32_NONE, 0, U64_MAX),
        'bool_count': lambda: check_counts(3, True, 0, 3, 1, U64_MAX),
        'u64_overflow': lambda: check_counts(3, 3, 1, 3, 1, 1 << 64),
        'too_many_chains': lambda: check_counts(3, 3, 2, 3, 2, U64_MAX),
    }
    for name, probe in probes.items():
        try:
            probe()
        except ValueError as e:
            refusals[name] = str(e)
        else:
            raise RuntimeError('refusal missing: ' + name)
    need(check_counts(U32_NONE - 1, U32_NONE - 1, 1, U32_NONE - 1, 1, U64_MAX) > 0, 'last valid u32')
    report_item = next(s for s in pins['sources'] if s['path'].endswith('/report.json'))
    report = json.loads(subprocess.check_output(['git', 'show', report_item['pin'] + ':' + report_item['path']], cwd=repo))
    payloads = []
    for item in report['steps']['identity']:
        if item['case'] != 'ng02':
            continue
        row = item['device']['passes'][0]; d = row['diagnostics']
        c, r, q = row['balls'], d['chain_elements'], d['chains_repaired']
        payloads.append(dict(k=item['k'], C=c, R=r, Q=q, old_bytes=16*c,
                             proposed_bytes=check_counts(c, r, q, c, c, U64_MAX)))
    return dict(valid_cases=total, exhaustive_max_vertices=9, random_cases=600, boundary_cases=7,
                scan_associativity_cases=associativity, scan_operation_bound='<=20*C',
                max_scan_operations_seen=max_work, killed_mutants=killed, refusals=refusals,
                payload_estimates_only=payloads, native_or_gpu_execution=False)


if __name__ == '__main__':
    p = argparse.ArgumentParser(); p.add_argument('--check', action='store_true')
    p.add_argument('--repo-root', type=Path, default=HERE.parents[3]); args = p.parse_args()
    result = replay(args.repo_root)
    if args.check:
        need(result == json.loads((HERE / 'results.json').read_text()), 'different results')
        print('chaines_compactes_ok: %d cas, 8 mutants, 18 refus ; modele seulement' % result['valid_cases'])
    else:
        print(json.dumps(result, sort_keys=True, indent=1))
