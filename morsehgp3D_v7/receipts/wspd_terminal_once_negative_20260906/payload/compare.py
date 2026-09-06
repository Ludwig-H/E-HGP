#!/usr/bin/env python3
"""Literal 174-front comparison, excluding exactly two work counters."""
import copy
import hashlib
import json
from pathlib import Path
import sys


def need(ok, why):
    if not ok:
        raise ValueError(why)


def equal(a, b):
    if type(a) is not type(b):
        return False
    if isinstance(a, dict):
        return a.keys() == b.keys() and all(equal(a[k], b[k]) for k in a)
    if isinstance(a, list):
        return len(a) == len(b) and all(equal(x, y) for x, y in zip(a, b))
    return a == b


def read(path):
    raw = Path(path).read_bytes()
    value = json.loads(raw)
    need(value['schema'] == 'mhgp7-private-wspd-q2-front-v1' and value['status'] == 'passed'
         and value['scope'] == 'differential_front_only' and value['public_status'] == 'not_claimed',
         'capture_metadata')
    need(type(value['calls']) is int and value['calls'] == 174
         and type(value['refusals']) is int and value['refusals'] == 6
         and len(value['cases']) == 174, 'capture_floors')
    return value, hashlib.sha256(raw).hexdigest()


def main(argv):
    need(len(argv) == 2, 'arguments: reference.json candidate.json')
    old, old_sha = read(argv[0])
    new, new_sha = read(argv[1])
    left, right = copy.deepcopy(old), copy.deepcopy(new)
    metrics = {k: dict(reference=0, candidate=0, saved=0, extra=0, increased_cases=0)
               for k in ('witness_nodes', 'corner_evals')}
    terminal_pairs = q2_pairs = 0
    for a, b in zip(left['cases'], right['cases']):
        need(a['work'].keys() == b['work'].keys() == metrics.keys(), 'work_schema')
        for name, row in metrics.items():
            x, y = a['work'].pop(name), b['work'].pop(name)
            need(type(x) is int and type(y) is int and min(x, y) >= 0, 'work_integer')
            row['reference'] += x
            row['candidate'] += y
            row['saved'] += max(x - y, 0)
            row['extra'] += max(y - x, 0)
            row['increased_cases'] += int(y > x)
            if name == 'witness_nodes' and a['scene'] == 0 and a['semantic']['cap_refus'] == 0:
                if a['mask'] == 1:
                    need(x == y == 3, 'existing_q2_reuse_preserved')
                    q2_pairs += 1
                else:
                    need(x == 6 and y == 3, 'terminal_single_pass_not_exercised')
                    terminal_pairs += 1
    need(equal(left, right), 'literal_front_changed')
    need(terminal_pairs == 36 and q2_pairs == 6, 'causal_nonvacuity')
    return dict(schema='mhgp7-private-wspd-terminal-once-comparison-v1', status='passed',
                public_status='not_claimed', engine_invoked=False, timing_claim=False,
                scope='given_JSON_outputs_only_source_and_process_bindings_external',
                reference_json_sha256=old_sha, candidate_json_sha256=new_sha,
                calls=174, refusals=6, exact_terminal_pair_witnesses=terminal_pairs,
                preserved_q2_pair_witnesses=q2_pairs, work=metrics)


if __name__ == '__main__':
    try:
        print(json.dumps(main(sys.argv[1:]), sort_keys=True))
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(json.dumps(dict(status='failed', reason=str(error), public_status='not_claimed'), sort_keys=True))
        sys.exit(2 if len(sys.argv) != 3 else 1)
