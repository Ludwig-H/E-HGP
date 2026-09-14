#!/usr/bin/env python3
"""Recompute paired work and time summaries from closed sibling campaigns."""
import importlib.util
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('sibling_measure', BASE / 'measure.py')
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def summary():
    rows = []
    pins = {}
    paired = []
    repeated = {}
    for directory in sorted(BASE.glob('campaign_*')):
        RUNNER.validate(directory)
        for name in ('MANIFEST.json', 'MEASURES.jsonl', 'COMPLETION.json'):
            path = directory / name
            pins[str(path.relative_to(BASE))] = RUNNER.sha(path)
        group = {}
        for line in (directory / 'MEASURES.jsonl').read_text().splitlines():
            raw = json.loads(line)
            result = raw['result']
            c = result['census_work']
            w = result['sibling_work']
            row = dict(campaign=directory.name, key=raw['key'],
                pipeline_ms=result['pipeline_total_ms'], total_ms=result['total_ms'],
                payload_ms=result['payload_ms'], count_nodes=c['count_node_visits'],
                count_bounds=c['count_bound_tests'], count_points=c['count_point_tests'],
                query_tasks=c['query_tasks'], query_splits=c['query_splits'],
                sibling_work=w, digest=result['digest'])
            row['count_plus_sibling_bounds'] = row['count_bounds'] + w['bound_tests']
            rows.append(row)
            identity = tuple(raw['key'])
            discrete = {k: v for k, v in row.items() if k not in (
                'campaign', 'pipeline_ms', 'total_ms', 'payload_ms')}
            require(identity not in repeated or repeated[identity] == discrete,
                    'repeated configuration changed discrete work')
            repeated[identity] = discrete
            group.setdefault(tuple(raw['key'][:-1]), {})[raw['key'][-1]] = row
        for key, modes in group.items():
            require(set(modes) == {'baseline', 'sibling', 'sibling_remaining'},
                    'campaign lacks one variant')
            base = modes['baseline']
            previous = base
            for mode in ('sibling', 'sibling_remaining'):
                current = modes[mode]
                require(current['count_nodes'] <= previous['count_nodes']
                        and current['query_tasks'] <= previous['query_tasks'],
                        'supplemental rejection increased traversal work')
                paired.append(dict(campaign=directory.name, input=list(key), mode=mode,
                    pipeline_ratio=current['pipeline_ms'] / base['pipeline_ms'],
                    count_node_ratio=current['count_nodes'] / base['count_nodes'],
                    total_bound_ratio=current['count_plus_sibling_bounds'] / base['count_bounds'],
                    query_task_ratio=current['query_tasks'] / base['query_tasks'],
                    sibling_success_rate=(current['sibling_work']['rejected_children'] /
                        current['sibling_work']['bound_tests']
                        if current['sibling_work']['bound_tests'] else None)))
                previous = current
    require(rows, 'no completed measurements')
    return dict(schema='mhgp8_q2_sibling_summary_v1', status='passed',
        scope='paired_audit_prototype_not_product_or_full_tower',
        report_sha256=RUNNER.sha(Path(__file__)), runner_sha256=RUNNER.sha(BASE / 'measure.py'),
        calls=len(rows), configurations=len(repeated), campaigns=len(pins) // 3,
        pins=pins, rows=rows, comparisons=paired)


if __name__ == '__main__':
    value = summary()
    path = BASE / 'SUMMARY.json'
    if sys.argv[1:] == ['--check']:
        require(json.loads(path.read_text()) == value, 'changed summary')
        print(json.dumps(dict(status='passed', calls=value['calls'],
                              configurations=value['configurations'], campaigns=value['campaigns'])))
    elif sys.argv[1:] == ['--write']:
        require(not path.exists(), 'refuse to overwrite summary')
        path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')
        print(json.dumps(dict(status='written', calls=value['calls'], campaigns=value['campaigns'])))
    else:
        raise SystemExit('usage: report.py --write|--check')
