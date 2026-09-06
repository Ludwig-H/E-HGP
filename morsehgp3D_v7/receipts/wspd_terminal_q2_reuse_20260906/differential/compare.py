#!/usr/bin/env python3
"""Compare two private front-gate outputs; no engine/provenance promotion."""
import copy
import hashlib
import json
from pathlib import Path
import sys


def require(ok, why):
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
    result = json.loads(raw)
    require(result['schema'] == 'mhgp7-private-wspd-q2-front-v1'
            and result['status'] == 'passed' and result['public_status'] == 'not_claimed'
            and result['scope'] == 'differential_front_only', 'capture_metadata')
    require(type(result['calls']) is int and result['calls'] == 174
            and type(result['refusals']) is int and result['refusals'] == 6
            and len(result['cases']) == 174, 'capture_floors')
    return result, hashlib.sha256(raw).hexdigest()


def main(argv):
    require(len(argv) == 2, 'arguments: reference.json candidate.json')
    old, old_sha = read(argv[0])
    new, new_sha = read(argv[1])
    projected_old, projected_new = copy.deepcopy(old), copy.deepcopy(new)
    saved_nodes = pair_witnesses = 0
    for a, b in zip(projected_old['cases'], projected_new['cases']):
        x = a['work'].pop('witness_nodes')
        y = b['work'].pop('witness_nodes')
        require(type(x) is int and type(y) is int and 0 <= y <= x, 'witness_node_monotonicity')
        saved_nodes += x - y
        if a['scene'] == 0 and a['mask'] == 1 and a['semantic']['cap_refus'] == 0:
            require(x == 6 and y == 3, 'q2_terminal_reuse_not_exercised')
            pair_witnesses += 1
    require(equal(projected_old, projected_new), 'literal_front_or_other_work_changed')
    require(pair_witnesses == 6 and saved_nodes >= 18, 'causal_nonvacuity')
    return dict(schema='mhgp7-private-wspd-q2-front-comparison-v1', status='passed',
                public_status='not_claimed', engine_invoked=False, timing_claim=False,
                reference_json_sha256=old_sha, candidate_json_sha256=new_sha,
                calls=174, refusals=6, exact_pair_witnesses=pair_witnesses,
                saved_witness_nodes=saved_nodes,
                scope='given_JSON_outputs_only_source_and_process_bindings_external')


if __name__ == '__main__':
    try:
        print(json.dumps(main(sys.argv[1:]), sort_keys=True))
    except (ValueError, KeyError, TypeError, OSError) as error:
        print(json.dumps(dict(status='failed', reason=str(error), public_status='not_claimed'), sort_keys=True))
        sys.exit(2 if len(sys.argv) != 3 else 1)
