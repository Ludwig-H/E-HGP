#!/usr/bin/env python3
"""Recount singleton roots in closed measurements; no new census invocation."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
PREVIOUS = BASE.parent / 'q2_sibling_20260914'


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def inspect():
    rows = []
    pins = {}
    for name in ('pilot', 'other_scans', 'growth', 'check50k', 'clusters8k', 'clusters16k'):
        folder = PREVIOUS / ('campaign_' + name)
        path = folder / 'MEASURES.jsonl'
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        completion_path = folder / 'COMPLETION.json'
        completion = json.loads(completion_path.read_text())
        require(completion['status'] == 'completed' and completion['measures_sha256'] == digest,
                'unclosed reference measurement')
        pins[str(path.relative_to(BASE.parents[2]))] = digest
        pins[str(completion_path.relative_to(BASE.parents[2]))] = hashlib.sha256(
            completion_path.read_bytes()).hexdigest()
        for line, text in enumerate(path.read_text().splitlines(), 1):
            row = json.loads(text)
            if row['key'][-1] != 'baseline':
                continue
            r = row['result']
            require(row['status'] == 'completed' and row['returncode'] == 0
                    and r['active_lane_mask'] == 1, 'wrong reference scope')
            singleton = r['front_work']['leaf_pair_rectangles']
            roots = r['anchor_queries']
            require(0 < singleton <= roots == r['census_work']['count_root_starts'],
                    'wrong root ledger')
            rows.append(dict(dataset=row['key'][0], n=row['key'][1], kmax=r['kmax'],
                             s=r['s'], singleton_roots=singleton, roots=roots,
                             fraction=singleton / roots, campaign=name, line=line))
    require(len(rows) == 8, 'missing input regime')
    return dict(status='passed', scope='existing_root_counts_not_new_timing',
                source_commit='f7edd6463adfeba559f305d14361bf85a8c25703',
                derivation='The smaller factor supplies anchors, so B is singleton exactly for a leaf-pair rectangle, which supplies one root.',
                script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                pins=pins, rows=rows)


if __name__ == '__main__':
    print(json.dumps(inspect(), sort_keys=True))
