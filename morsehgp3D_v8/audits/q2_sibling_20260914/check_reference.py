#!/usr/bin/env python3
"""Check unchanged baseline work against the earlier independent/product receipts."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
FIELDS = ('front_work', 'census_work', 'callback_work',
          'digest', 'total_unordered_pairs', 'active_lane_mask', 'input_rectangles',
          'anchor_queries', 'candidate_pairs', 'accepted_pairs', 'rejected_pairs')


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run():
    references = {}
    pins = {}
    previous = BASE.parent / 'q2_front_20260914'
    for path in sorted(previous.glob('campaign_*/MEASURES.jsonl')):
        pins[str(path.relative_to(ROOT))] = sha(path)
        for line, raw in enumerate(path.read_text().splitlines(), 1):
            row = json.loads(raw)
            if row['key'][3:] == ['samples', 'shared']:
                references.setdefault(tuple(row['key'][:3]), (path, line, row['result']))
    path = ROOT / 'morsehgp3D_v8/receipts/wspd_q2_census_20260914/shared_matrix/MEASURES.jsonl'
    pins[str(path.relative_to(ROOT))] = sha(path)
    inputs_path = BASE / 'INPUTS.json'
    pins[str(inputs_path.relative_to(ROOT))] = sha(inputs_path)
    clusters = {row['n']: row for row in json.loads(inputs_path.read_text())['datasets']}
    for line, raw in enumerate(path.read_text().splitlines(), 1):
        row = json.loads(raw)
        r = row['result']
        if r['family'] == 'clusters' and r['n'] in clusters and r['s'] == 8:
            require(row['status'] == 'completed' and row['exit_code'] == 0, 'failed reference')
            require(r['input_hash'] == clusters[r['n']]['constructor_input_hash_encoding1'],
                    'cluster recipe input hash mismatch')
            references[('clusters_seed3', r['n'], 8)] = (path, line, r)
    checked = []
    for path in sorted(BASE.glob('campaign_*/MEASURES.jsonl')):
        pins[str(path.relative_to(ROOT))] = sha(path)
        for line, raw in enumerate(path.read_text().splitlines(), 1):
            row = json.loads(raw)
            if row['key'][-1] != 'baseline':
                continue
            key = tuple(row['key'][:3])
            ref_path, ref_line, reference = references[key]
            require(row['status'] == 'completed' and row['returncode'] == 0, 'failed baseline')
            result = row['result']
            require(all(result[field] == reference[field] for field in FIELDS),
                    'baseline changed discrete work or output: ' + str(key))
            expected_fnv = (clusters[key[1]]['input_fnv64'] if key[0] == 'clusters_seed3'
                            else reference['input_fnv64'])
            require(result['input_fnv64'] == expected_fnv, 'baseline changed physical input')
            checked.append(dict(key=list(key), baseline=str(path.relative_to(ROOT)), line=line,
                                reference=str(ref_path.relative_to(ROOT)), reference_line=ref_line))
    require(len(checked) == 10, 'missing baseline comparisons')
    return dict(status='passed', scope='baseline_discrete_work_and_physical_output_not_timings',
                checker_sha256=sha(Path(__file__)), fields=FIELDS, pins=pins, comparisons=checked)


if __name__ == '__main__':
    print(json.dumps(run(), sort_keys=True))
