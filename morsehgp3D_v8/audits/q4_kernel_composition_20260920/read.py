#!/usr/bin/env python3
"""Closure + replayed small rational oracle; effective also with python -O."""
from collections import Counter
import json
from pathlib import Path

from campaign import BASE, ROOT, exact_records, load, normalized, require, sha, source_pins


def records(folder):
    manifest = load(folder/'MANIFEST.json')
    closing = load(folder/'COMPLETION.json')
    require(manifest['status'] == closing['status'] == 'completed', 'Incomplete capture')
    require(closing['manifest_sha256'] == sha(folder/'MANIFEST.json'), 'Manifest changed')
    require(manifest['sources'] == source_pins(), 'Source/library changed')
    require(len(manifest['records']) == closing['records'], 'Record count mismatch')
    result = []
    for entry in manifest['records']:
        path = folder/entry['path']
        require(sha(path) == entry['sha256'], 'Record changed')
        record = load(path)
        require(record['returncode'] == 0 and not record['stderr'], 'Command failed')
        result.append(record)
    return manifest,result


def points(path):
    lines = path.read_text().splitlines()
    data = [tuple(map(int,line.split())) for line in lines[1:]]
    require(int(lines[0]) == len(data), 'Input length mismatch')
    return data


def child(record, binaries, input_cache):
    command = record['command']
    binary, path = Path(command[0]), Path(command[1])
    require(str(binary.relative_to(ROOT)) in binaries and sha(binary) == binaries[str(binary.relative_to(ROOT))],
            'Wrong executable')
    require(path.parent == BASE/'.inputs' and sha(path) == record['input_sha256'], 'Wrong input')
    data = json.loads(record['stdout'])
    require(data['schema'] == 'mhgp8_audit_lidar_product_pair_v1' and data['status'] == 'passed', 'Wrong schema/status')
    require(data['product_commit'] == '31b0243a' and data['completeness_large_cloud_claimed'] is False, 'Wrong scope')
    require(command[2:] == [str(record['kmax'])] and data['kmax'] == record['kmax'], 'Wrong K/command')
    if path not in input_cache:
        cloud = points(path)
        fnv = 14695981039346656037
        for value in [len(cloud)]+[x for p in cloud for x in p]:
            for _ in range(8):
                fnv = ((fnv ^ (value & 255))*1099511628211) & ((1<<64)-1)
                value >>= 8
        input_cache[path] = cloud,fnv
    cloud,fnv = input_cache[path]
    require(data['n'] == len(cloud) and data['input_fnv1a64_u64le_n_xyz'] == fnv, 'Parsed input mismatch')
    require(data['edge_ids'] == [0,1] and data['local_options'] == dict(domain='Positive',depth=7,node_budget=4096,
            z_test_budget=512,leaf_sites=32,clip_events=True), 'Wrong local options')
    old,new = data['local28'],data['shallow29']
    require(old['output'] == new['output'], 'Output ledgers differ')
    require(old['output']['q3'] == 0 and old['output']['q4'] == len(data['records']) == data['judge']['supports'],
            'Output count mismatch')
    require(old['sweep']['emitted'] == new['sweep']['emitted'] == len(data['records']), 'Emission ledger mismatch')
    require(data['judge']['point_tests'] == data['judge']['distinct_balls']*data['n'], 'Global census ledger mismatch')
    require(new['selection']['retained_ids']+new['selection']['discarded_ids'] == data['cover_sites'] == new['selection']['input_sites'],
            'Retained/discarded population mismatch')
    family = new['sweep']['family']
    require(family['sites'] == new['selection']['retained_ids']*new['edge']['seeds'], 'Shallow work ledger mismatch')
    require(family['sites'] == sum(family[k] for k in ('constant_inside','constant_on','constant_outside','event_count')),
            'Shallow site partition mismatch')
    require(family['event_count'] == family['entries']+family['exits'], 'Event partition mismatch')
    require(new['edge']['seeds'] <= old['edge']['seeds'], 'Reduction added seeds')
    require(sum(len(r['shell']) for r in data['records']) == old['output']['shell_ids_visited'], 'Shell ledger mismatch')
    require(len(set(normalized(data))) == len(data['records']), 'Duplicate records')
    return data,cloud


def main():
    qualification, commands = records(BASE/'qualification')
    require(len(commands) == 200 and qualification['counts']['calls'] == 192, 'Wrong qualification cardinal')
    require(json.loads(commands[4]['stdout']) == json.loads(commands[5]['stdout']), 'Composition Python modes differ')
    require(json.loads(commands[6]['stdout']) == json.loads(commands[7]['stdout']), 'Degeneracy Python modes differ')
    binaries = qualification['binaries']
    cache, expectations, reports = {},{},{}
    oracle_counts = dict(tetrahedra=0,point_tests=0)
    for record in commands[8:]:
        data,cloud = child(record,binaries,cache)
        identity = record['case'],record['kmax']
        if identity not in expectations:
            expectations[identity] = exact_records(cloud,record['kmax'],oracle_counts)
        require(normalized(data) == expectations[identity], 'Replayed Cartesian oracle differs')
        reports.setdefault(identity,[]).append((data['local28'],data['shallow29']))
    require(len(reports) == 96 and all(len(r) == 2 and r[0] == r[1] for r in reports.values()),
            'Release/sanitizer pairs differ')
    require(len(expectations[('isolated_shallow_vertex',3)]) == 1, 'Isolated positive root missing')
    require(not expectations[('independent_kernel_intersection',3)], 'False shallow root in intersection fixture')
    capture, commands = records(BASE/'lidar_capture')
    require(len(commands) == 90, 'Wrong LiDAR cardinal')
    require(capture['binary_sha256'] == binaries[str(Path(capture['binary']).relative_to(ROOT))], 'Campaign binary not qualified')
    identities = set()
    rows = []
    for record in commands:
        require(record['command'][0] == capture['binary'], 'Wrong campaign command')
        data,_ = child(record,binaries,cache)
        require(record['n'] == data['n'], 'Record n mismatch')
        for path,digest in record['provenance']['input_sha256'].items():
            require(sha(ROOT/path) == digest, 'Original LiDAR input changed')
        identity = record['case'],record['kmax']
        require(identity not in identities, 'Duplicate measurement')
        identities.add(identity)
        old,new = data['local28'],data['shallow29']
        times = data['timings_ms']
        rows.append(dict(case=record['case'],n=data['n'],kmax=data['kmax'],cover=data['cover_sites'],
            seeds28=old['edge']['seeds'],seeds29=new['edge']['seeds'],retained=new['selection']['retained_ids'],
            W28=old['sweep']['active_sites'],W29=new['sweep']['family']['sites'],
            sort28=old['sweep']['sort_comparisons'],sort29=new['sweep']['family']['sort_comparisons'],
            groups28=old['sweep']['groups'],groups29=new['sweep']['family']['groups'],
            preparation_forms29=new['selection']['form_tests'],hull_orientations29=new['selection']['orientation_tests'],
            hull_lex29=new['selection']['lex_comparisons'],cells28=old['atlas']['cells_created'],
            blocks28=old['atlas']['partition']['block_bound_tests'],points28=old['atlas']['partition']['point_tests'],
            emitted=len(data['records']),shell_ids=old['output']['shell_ids_visited'],
            peak28=old['edge']['peak_live_buffer_bytes'],peak29=new['edge']['peak_live_buffer_bytes'],
            run28_ms=times['local28_edge_and_collect'],run29_ms=times['shallow29_edge_and_collect'],
            common_ms=sum(times[k] for k in ('cloud','index','cover')),
            judge_ms=times['independent_judge']))
    expected = {(f'lidar_{scan:06d}_{n}_fixed_rank{rank}',k) for scan in (0,100,200)
                for n in (8000,16000,32000,50000) for rank in (8,32,128) for k in (5,10)}
    expected |= {(f'lidar_{scan:06d}_8000_anchor{a}_rank512',k) for scan in (0,100,200)
                 for a in (0,1000,3000) for k in (5,10)}
    require(identities == expected, 'Missing or unplanned edge/config')
    aggregates = {}
    for k in (5,10):
        selected = [r for r in rows if r['kmax'] == k]
        sums = {field:sum(r[field] for r in selected) for field in
                ('cover','seeds28','seeds29','retained','W28','W29','sort28','sort29','groups28','groups29',
                 'preparation_forms29','hull_orientations29','hull_lex29','blocks28','points28','emitted','shell_ids','run28_ms','run29_ms','common_ms')}
        sums['selections_with_removal'] = sum(r['retained'] < r['cover'] for r in selected)
        sums['edges_with_seed_removal'] = sum(r['seeds29'] < r['seeds28'] for r in selected)
        sums['W29_larger'] = sum(r['W29'] > r['W28'] for r in selected)
        sums['W29_smaller'] = sum(r['W29'] < r['W28'] for r in selected)
        sums['run29_slower'] = sum(r['run29_ms'] > r['run28_ms'] for r in selected)
        aggregates[str(k)] = sums
    print(json.dumps(dict(schema='mhgp8_audit_kernel_readback_v1',status='passed',qualification=qualification['counts'],
                         independent_oracle_replayed=True,qualification_records=200,measurement_records=90,
                         composition_model=json.loads(load(BASE/'qualification'/qualification['records'][4]['path'])['stdout']),
                         degeneracy_model=json.loads(load(BASE/'qualification'/qualification['records'][6]['path'])['stdout']),
                         scope='Actual paired ports28/29 on selected LiDAR edges; no composition or global tower measured',
                         aggregates=aggregates,rows=rows),sort_keys=True,indent=2))


if __name__ == '__main__':
    main()
