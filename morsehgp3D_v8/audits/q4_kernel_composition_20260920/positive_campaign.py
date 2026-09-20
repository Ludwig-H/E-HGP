#!/usr/bin/env python3
"""Separate targeted LiDAR capture: productive8k edges, same physical edges at50k."""
import json
from pathlib import Path
import sys

from campaign import BASE, ROOT, append, close, edge_first, execute, lidar, load, require, save, sha, source_pins, write
sys.path.insert(0, str(BASE))
from read import child, records


def extra_pins():
    return {p.name:sha(p) for p in (BASE/'positive_campaign.py',BASE/'positive_discovery.py',
                                   BASE/'POSITIVE_FIXTURES.json',BASE/'POSITIVE_FIXTURES_optimized.json')}


def fixture_checks():
    normal = load(BASE/'POSITIVE_FIXTURES.json')
    optimized = load(BASE/'POSITIVE_FIXTURES_optimized.json')
    require(normal['status'] == optimized['status'] == 'completed' and len(normal['fixtures']) == 9, 'Discovery failed')
    for field in ('recipe','sources','input_files','fixtures','searches','all_targets_met','result_sha256'):
        require(normal[field] == optimized[field], 'Discovery Python modes differ')
    require(optimized['comparison']['sha256'] == sha(BASE/'POSITIVE_FIXTURES.json'), 'Discovery comparison changed')
    for group in ('sources','input_files'):
        for path,digest in normal[group].items():
            require(sha(ROOT/path) == digest, 'Discovery source/input changed')
    return normal


def census(cloud, coefficients):
    a,b,c,d,e = map(int,coefficients)
    depth,shell = 0,[]
    for idx,(x,y,z) in enumerate(cloud):
        value=a*(x*x+y*y+z*z)+b*x+c*y+d*z+e
        depth += value < 0
        if value == 0:
            shell.append(idx)
    return depth,shell


def check_target(data,record,cloud):
    depth,shell=census(cloud,record['required_ball_key'])
    require(depth == record['required_depth'] and shell == record['required_shell'], 'Target census changed')
    matches=[r for r in data['records'] if list(map(int,r['coefficients'])) == record['required_ball_key']]
    require(bool(matches) == (depth < record['kmax']-2), 'Product lost/added the independently selected target ball')
    require(all(r['depth'] == depth and r['shell'] == shell for r in matches), 'Target payload differs')


def run():
    folder=BASE/sys.argv[2]
    require(folder.parent == BASE and not folder.exists(),'Fresh direct-child capture required')
    folder.mkdir()
    discovered=fixture_checks()
    before=source_pins()
    qualified=load(BASE/'qualification/MANIFEST.json')
    binary=BASE/'.build/qualification/release'
    require(sha(binary) == qualified['binaries'][str(binary.relative_to(ROOT))], 'Binary not qualified')
    manifest=dict(schema='mhgp8_audit_productive_lidar_pairs_v1',status='started',sources=before,
                  extra_sources=extra_pins(),binary=str(binary),binary_sha256=sha(binary),records=[],
                  scope='Nine edges chosen for positive shallow8k tetrahedra; same edges50k; not an unbiased sample or generator')
    for index,fixture in enumerate(discovered['fixtures']):
        scan=fixture['scan']
        for n in (8000,50000):
            original,pin=lidar(scan,n)
            cloud=edge_first(original,*fixture['edge_ids'])
            key=list(map(int,fixture['coefficients']))
            depth,shell=census(cloud,key)
            if n == 8000:
                require(depth == fixture['depth'] < 3 and len(shell) == 4, 'Selected fixture is no longer productive')
            name=f'positive_{scan:06d}_{n}_fixture{index}'
            path=save(name,cloud)
            for k in (5,10):
                record=dict(case=name,n=n,kmax=k,fixture_index=index,input_sha256=sha(path),
                            provenance=dict(input_sha256=pin,original_edge_ids=fixture['edge_ids'],selected_at_n=8000),
                            required_ball_key=key,required_depth=depth,required_shell=shell,
                            **execute([str(binary),str(path),str(k)]))
                append(folder,manifest,record)
                check_target(json.loads(record['stdout']),record,cloud)
            print('PASS productive pair',name,flush=True)
    require(len(manifest['records']) == 36,'Wrong targeted campaign size')
    require(extra_pins() == manifest['extra_sources'] and sha(binary) == manifest['binary_sha256'], 'Supplement changed')
    close(folder,manifest,before)


def readback():
    discovered=fixture_checks()
    manifest,commands=records(BASE/'positive_capture')
    require(manifest['extra_sources'] == extra_pins() and len(commands) == 36,'Supplement changed')
    qualified=load(BASE/'qualification/MANIFEST.json')
    cache={}
    rows=[]
    seen=set()
    for record in commands:
        require(record['command'][0] == manifest['binary'] and sha(Path(manifest['binary'])) == manifest['binary_sha256'],
                'Wrong targeted executable')
        data,cloud=child(record,qualified['binaries'],cache)
        check_target(data,record,cloud)
        identity=record['fixture_index'],record['n'],record['kmax']
        require(identity not in seen,'Duplicate targeted case')
        seen.add(identity)
        fixture=discovered['fixtures'][record['fixture_index']]
        require(record['required_ball_key'] == fixture['coefficients'] and
                record['provenance']['original_edge_ids'] == fixture['edge_ids'], 'Target fixture mismatch')
        for path,digest in record['provenance']['input_sha256'].items():
            require(sha(ROOT/path) == digest, 'Original LiDAR changed')
        old,new=data['local28'],data['shallow29']
        rows.append(dict(case=record['case'],n=data['n'],kmax=data['kmax'],required_depth=record['required_depth'],
            required_present=record['required_depth'] < data['kmax']-2,cover=data['cover_sites'],
            seeds28=old['edge']['seeds'],seeds29=new['edge']['seeds'],retained=new['selection']['retained_ids'],
            W28=old['sweep']['active_sites'],W29=new['sweep']['family']['sites'],
            sort28=old['sweep']['sort_comparisons'],sort29=new['sweep']['family']['sort_comparisons'],
            points28=old['atlas']['partition']['point_tests'],blocks28=old['atlas']['partition']['block_bound_tests'],
            forms29=new['selection']['form_tests'],hull_lex29=new['selection']['lex_comparisons'],
            orientations29=new['selection']['orientation_tests'],
            emitted=len(data['records']),shell_ids=old['output']['shell_ids_visited'],
            peak28=old['edge']['peak_live_buffer_bytes'],peak29=new['edge']['peak_live_buffer_bytes'],
            run28_ms=data['timings_ms']['local28_edge_and_collect'],run29_ms=data['timings_ms']['shallow29_edge_and_collect']))
    require(seen == {(i,n,k) for i in range(9) for n in (8000,50000) for k in (5,10)},'Missing productive case')
    sums={}
    for n in (8000,50000):
        for k in (5,10):
            selected=[r for r in rows if r['n'] == n and r['kmax'] == k]
            sums[f'{n}_K{k}']={field:sum(r[field] for r in selected) for field in (
                'cover','seeds28','seeds29','retained','W28','W29','sort28','sort29','points28','blocks28',
                'forms29','hull_lex29','orientations29','emitted','shell_ids','run28_ms','run29_ms','required_present')}
    print(json.dumps(dict(schema='mhgp8_audit_productive_lidar_readback_v1',status='passed',records=36,
                         independent_selected_ball_rechecks=36,scope=manifest['scope'],aggregates=sums,rows=rows),
                     sort_keys=True,indent=2))


if __name__ == '__main__':
    if sys.argv[1] == 'run':
        run()
    elif sys.argv[1] == 'read':
        readback()
    else:
        raise RuntimeError('Unknown mode')
