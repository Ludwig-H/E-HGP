#!/usr/bin/env python3
"""Recheck closed seed/cell join receipts and the independent small oracle."""
import argparse
from collections import defaultdict
import json
from pathlib import Path
import struct

from campaign import BASE, ROOT, ORACLE, load, require, sha


def read(folder, check_live=True):
    manifest = load(folder/'MANIFEST.json')
    completion = load(folder/'COMPLETION.json')
    require(manifest['status'] == completion['status'] == 'completed', 'Unclosed receipt')
    require(completion['manifest_sha256'] == sha(folder/'MANIFEST.json'), 'Manifest changed')
    require(completion['records'] == len(manifest['records']), 'Command count mismatch')
    if check_live:
        for name, expected in {**manifest['sources'], **manifest['binaries']}.items():
            require(sha(ROOT/name) == expected, 'Live dependency changed: '+name)
    seen = set()
    totals = defaultdict(int)
    rows = []
    oracle_work = {}
    for entry in manifest['records']:
        name = entry['path']
        require(name not in seen and Path(name).name == name, 'Duplicate/unsafe record name')
        seen.add(name)
        path = folder/name
        require(sha(path) == entry['sha256'], 'Record changed')
        record = load(path)
        require(record['returncode'] == 0 and record['stderr'] == '', 'Failed command')
        if 'case' not in record:
            continue
        command = record['command']
        require(len(command) == 6, 'Unexpected probe command')
        binary, source, k, a, b, grain = command
        require(str(Path(binary).relative_to(ROOT)) in manifest['binaries'], 'Unpinned executable')
        require([int(k), int(a), int(b), int(grain)] ==
                [record['kmax'], *record['edge'], record['grain']], 'Command/metadata mismatch')
        raw = Path(source).read_bytes()
        require(sha(Path(source)) == record['input_sha256'], 'Input changed')
        require(len(raw) == 6*record['n'], 'Wrong input size')
        data = json.loads(record['stdout'])
        require(data['status'] == 'completed', 'Probe did not complete')
        require((data['n'], data['kmax'], data['edge'], data['grain']) ==
                (record['n'], record['kmax'], record['edge'], record['grain']), 'Command/report mismatch')
        normalized = ORACLE.normalize(data['records'])
        require(len(set(normalized)) == len(normalized), 'Duplicate exact emission')
        if not record['case'].startswith('lidar_'):
            points = list(struct.iter_unpack('<HHH', raw))
            expected = ORACLE.expected(points, int(k), 4, oracle_work)
            expected = [r for r in expected if ORACLE.owner(points, tuple(r['support'])) == (int(a), int(b))]
            require(normalized == ORACLE.normalize(expected), 'Small rational oracle differs')
        for mode in ('baseline', 'alive', 'join'):
            report = data[mode]
            require(report['matches_baseline'] is True, 'Paired mode mismatch')
            require(report['output_count'] == len(normalized), 'Output count mismatch')
            for category in ('generator', 'sweep', 'extra'):
                for field, value in report[category].items():
                    require(type(value) is int and value >= 0, 'Invalid discrete counter')
                    totals[mode+'.'+category+'.'+field] += value
        # The same live leaves are swept; only reaching them changes. All
        # complete fragment scans, event sorts, shell/census and outputs agree.
        for field in data['baseline']['sweep']:
            if field in ('seed_queries', 'seed_owner_tests', 'seed_owner_rejections',
                         'query_visits', 'line_tests', 'line_skips', 'peak_buffer_bytes'):
                continue
            require(data['baseline']['sweep'][field] == data['alive']['sweep'][field] ==
                    data['join']['sweep'][field], 'Downstream work changed: '+field)
        totals['calls'] += 1
        totals['outputs'] += len(normalized)
        totals['shell_ids'] += sum(len(r[-1]) for r in normalized)
        totals['max_shell'] = max([totals['max_shell']]+[len(r[-1]) for r in normalized])
        rows.append(dict(case=record['case'], n=record['n'], kmax=record['kmax'], grain=record['grain'],
                         edge=record['edge'], preparation=data['preparation'],
                         **{m:data[m] for m in ('baseline','alive','join')}))
    return dict(status='passed', commands=len(seen), dependencies=len(manifest['sources']),
                totals=dict(sorted(totals.items())), oracle_work=oracle_work, rows=rows)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('folder', type=Path)
    parser.add_argument('--compact', action='store_true')
    args = parser.parse_args()
    result = read(args.folder.resolve())
    if args.compact:
        result.pop('rows')
    print(json.dumps(result, sort_keys=True))
