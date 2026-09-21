#!/usr/bin/env python3
"""Read immutable relay receipts and recheck payloads against rational spheres."""
from collections import Counter
import gzip
import json
from pathlib import Path
import struct
import sys

import campaign as c


def empty_counts():
    return dict(calls=0, seeds=0, relays=0, positive_prefixes=0,
                accepted_relays=0, global_oracle_point_tests=0, max_shell=0)


def require(condition, why):
    c.require(condition, why)


def work_check(row, threshold, incoming, roots, accepted):
    w = row['work']
    require(len(w) == 26 and all(type(v) is int and v >= 0 for v in w), 'Work schema')
    require(w[0] == 1 and w[3] == int(accepted) and w[4] == int(not accepted), 'Query ledger')
    require(w[15] == int(not accepted), 'Saturation ledger')
    require(w[10] + incoming == row['depth'], 'Credit ledger')
    require(w[5] == w[9] + w[11] + w[13], 'Count visit partition')
    require(w[6] == w[5] + w[14] == w[7] + w[8] == roots + 2*w[13], 'Prepared count partition')
    require(w[16] == w[17] == w[18] + w[19], 'Shell bound partition')
    require(w[17] == int(accepted) + 2*w[21], 'Shell tree partition')
    require(w[16] == w[20] + w[21] + w[22], 'Shell visit partition')
    require(w[22] == len(row['shell']), 'Shell size')
    require(w[23] <= 49 and w[24] <= 49, 'Stack capacity')
    if incoming >= threshold:
        require(all(v == (1 if i in (0, 4, 15) else 0) for i, v in enumerate(w)), 'Saturated prefix work')
    else:
        require(w[1] == 1 and w[2] == 3 and w[25] == 2352, 'Single preparation/scratch')
    if not accepted:
        require(not row['shell'] and row['sort_comparisons'] == 0, 'Rejected payload work')


def inspect(data, counters):
    threshold = data['K'] - 1
    require(data['kind'] == 'q3_prefix_relay_contract', 'Probe kind')
    require(data['index_nodes'] == 2*data['n'] - 1, 'Binary singleton tree size')
    cuts = {x['name']: x for x in data['cuts']}
    require(len(cuts) == len(data['cuts']) == 5, 'Duplicate/missing cuts')
    require(cuts['root'] == dict(name='root', cursor=0, rank=0), 'Root seam')
    require(cuts['EOF'] == dict(name='EOF', cursor=data['index_nodes'], rank=data['n']), 'EOF seam')
    for name, factor in (('q25', 1), ('q50', 2), ('q75', 3)):
        require(cuts[name]['rank'] == factor*data['n']//4, 'Quartile rank')
        require(0 < cuts[name]['cursor'] < data['index_nodes'], 'Leaf cursor range')
    selection = data['selection']
    require(selection['point_tests'] == data['n'] - 2, 'Uncapped selection scan')
    require(selection['owner_edge_tests'] == 2*selection['acute_seeds'], 'Owner checks')
    require(selection['selected'] == len(data['records']) <= selection['owned_seeds'] <= selection['acute_seeds'], 'Selection ledger')
    for record in data['records']:
        ref = record['reference']
        work_check(ref, threshold, 0, 1, ref['accepted'])
        require({r['cut'] for r in record['relays']} == set(cuts), 'Repeated/missing relay')
        for row in record['relays']:
            incoming = row['incoming_count']
            require(row['prefix_point_tests'] == row['rank'], 'Test-only prefix work')
            require(incoming == min(row['prefix_exact_count'], threshold), 'Prefix saturation')
            require(row['prefix_saturated'] == int(incoming >= threshold), 'Prefix early exit')
            require(row['shell_global_roots'] == int(row['accepted']), 'Global shell restart')
            work_check(row, threshold, incoming, row['forest_roots'], row['accepted'])
            if row['cut'] == 'root':
                require(row['work'] == ref['work'] and row['sort_comparisons'] == ref['sort_comparisons'], 'Root reference identity')
            if row['cut'] == 'EOF':
                require(row['forest_roots'] == 0 and not any(row['work'][5:15]), 'EOF extension')
            counters['saturated_prefixes'] += bool(row['prefix_saturated'])
            counters['accepted_EOF'] += row['cut'] == 'EOF' and row['accepted']
            counters['accepted_positive_prefix'] += bool(incoming) and row['accepted']
            counters['multi_root_suffix'] += row['forest_roots'] > 1
            counters['test_only_prefix_point_tests'] += row['prefix_point_tests']
            counters['maximum_stack_entries'] = max(counters['maximum_stack_entries'], *row['work'][23:25])


def read(folder):
    manifest = json.loads((folder/'MANIFEST.json').read_text())
    completion = json.loads((folder/'COMPLETION.json').read_text())
    require(manifest['status'] == completion['status'] == 'completed', 'Incomplete receipt')
    require(completion['manifest_sha256'] == c.sha(folder/'MANIFEST.json'), 'Manifest closure')
    require(manifest['commands_sha256'] == c.sha(folder/'COMMANDS.jsonl.gz'), 'Command closure')
    for name, expected in manifest['sources'].items():
        path = c.ROOT/name
        if c.sha(path) != expected:
            prior = c.BASE/'history/by_sha'/expected
            require(path.is_relative_to(c.BASE) and prior.is_file() and c.sha(prior) == expected,
                    'Changed dependency without preserved source: '+name)
    for name, expected in manifest['binaries'].items():
        require(c.sha(c.ROOT/name) == expected, 'Changed binary: '+name)
    small = dict(c.cases())
    counts, counters, models, pairs = empty_counts(), Counter(), [], {}
    commands = 0
    with gzip.open(folder/'COMMANDS.jsonl.gz', 'rt') as stream:
        for line in stream:
            commands += 1
            record = json.loads(line)
            require(record['returncode'] == 0 and not record['stderr'], 'Failed command')
            command = record['command']
            if 'case' not in record:
                if command[-1] == str(c.BASE/'forest_model.py'):
                    model = json.loads(record['stdout'])
                    require(model['status'] == 'PASS' and model['source_sha256'] == c.sha(c.BASE/'forest_model.py'), 'Model provenance')
                    require(len(model['mutants']) == 4 and all(x['caught'] for x in model['mutants']), 'Model mutants')
                    models.append(model)
                continue
            data = json.loads(record['stdout'])
            binary, source = Path(command[0]), Path(command[1])
            require(str(binary.relative_to(c.ROOT)) in manifest['binaries'], 'Unpinned binary')
            require(command[2:] == list(map(str, (record['k'], *record['edge'], record['sample_limit']))), 'Command arguments')
            if record['case'] in small:
                points = small[record['case']]
                raw = b''.join(struct.pack('<HHH', *p) for p in points)
                require(source == c.BASE/'.inputs'/(record['case']+'.u16le'), 'Small input path')
                group = binary.name
            else:
                require(record['case'].startswith('lidar_'), 'Unknown fixture')
                _, scan, n = record['case'].split('_')
                require(source == c.BASE.parent/'lidar08_20260914/prepared'/('single_'+scan)/('n'+n+'.u16le'), 'LiDAR input path')
                raw = source.read_bytes()
                points = list(struct.iter_unpack('<HHH', raw))
                group = 'lidar'
            require(c.hashlib.sha256(raw).hexdigest() == record['input_sha256'], 'Input hash')
            require(record['n'] == len(points), 'Input size')
            c.check(data, points, *record['edge'], record['k'], record['sample_limit'], counts)
            inspect(data, counters)
            counters[group+'_calls'] += 1
            counters[group+'_seeds'] += len(data['records'])
            if group != 'lidar':
                key = (record['case'], record['k'], *record['edge'])
                if key in pairs:
                    require(pairs.pop(key) == data, 'Release/sanitizer difference')
                else:
                    pairs[key] = data
    require(commands == manifest['commands'], 'Command count')
    require(counts == manifest['counts'], 'Recomputed counts')
    require(not pairs, 'Unpaired small gate')
    require(len(models) == 2 and models[0] == models[1], 'Model normal/-O mismatch')
    require(counters['accepted_EOF'] > 0 and counters['multi_root_suffix'] > 0, 'Unexercised relay branches')
    if manifest['scope'] == 'Additional shell30 non-vacuity; no source changes or rebuild':
        require(counts['max_shell'] == 30, 'Large shell not exercised')
    else:
        require(counters['accepted_positive_prefix'] > 0, 'Unexercised positive accepted prefix')
    return dict(status='PASS', manifest_sha256=c.sha(folder/'MANIFEST.json'),
                commands=commands, counts=counts, branches=dict(counters), model=models[0])


if __name__ == '__main__':
    require(len(sys.argv) == 2, 'usage: read_closed.py receipt_folder')
    print(json.dumps(read(Path(sys.argv[1]).resolve()), sort_keys=True, indent=2))
