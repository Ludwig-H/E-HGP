#!/usr/bin/env python3
"""Fresh audit executables, exact small oracle and actual paired LiDAR ports28/29."""
import gzip
import hashlib
from itertools import combinations
import json
from math import gcd, lcm
import os
from pathlib import Path
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
REFERENCE = BASE.parent/'q4_center_blocks_20260920'
sys.path.insert(0, str(REFERENCE))
from fixtures import cases, distance, edge_first, lidar
from oracle_gate import ball, fixtures, seeds


def require(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, data):
    path.write_text(json.dumps(data, sort_keys=True, indent=2)+'\n')


def load(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def source_pins():
    # Product source inputs are frozen to31b0243a even if later CMake/docs evolve.
    old = ROOT/'morsehgp3D_v8/receipts/q4_shallow_20260920/scale__b3aquyb/MANIFEST.json'
    declared = load(old)['source_sha256']
    names = [p for p in declared if p.startswith('morsehgp3D_v8/src/')]
    names += ['morsehgp3D_v8/bench/q34_cover_probe.cpp', 'morsehgp3D_v8/tests/exact_ball_oracle.hpp']
    for name in names:
        require(sha(ROOT/name) == declared[name], 'Product input no longer matches31b0243a: '+name)
    paths = [ROOT/p for p in names]
    paths += [BASE/p for p in ('lidar_product_probe.cpp','campaign.py','composition_gate.py','degeneracy_gate.py')]
    paths += [REFERENCE/'fixtures.py', REFERENCE/'oracle_gate.py']
    paths += [ROOT/'build'/b/'libmhgp8_p0.a' for b in ('v8_q4_shallow_20260920','v8_q4_shallow_sanitize_20260920')]
    # Explicit library reuse from frozen constructor builds; no rebuild or inherited qualification.
    paths += [ROOT/'build/v7_boost_gate/extracted/usr/include/boost/multiprecision/cpp_int.hpp']
    return {str(p.relative_to(ROOT)): sha(p) for p in paths}


def execute(command, timeout=180):
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    start = time.monotonic()
    try:
        child = subprocess.run(command, capture_output=True, text=True, timeout=timeout, env=env)
        return dict(command=command, returncode=child.returncode, stdout=child.stdout, stderr=child.stderr,
                    wall_seconds=time.monotonic()-start)
    except BaseException as error:
        return dict(command=command, returncode=None, error=repr(error),
                    stdout=str(getattr(error,'stdout','')), stderr=str(getattr(error,'stderr','')),
                    wall_seconds=time.monotonic()-start)


def save(name, points):
    folder = BASE/'.inputs'
    folder.mkdir(exist_ok=True)
    path = folder/(name+'.txt')
    raw = (str(len(points))+'\n'+'\n'.join(' '.join(map(str,p)) for p in points)+'\n').encode()
    require(not path.exists() or path.read_bytes() == raw, 'Fixture overwrite refused')
    path.write_bytes(raw)
    return path


def small_cases():
    yield from fixtures()
    regular = [(30,30,30),(36,36,30),(30,36,24),(36,30,24)]
    yield 'regular', regular
    yield 'isolated_shallow_vertex', regular+[(30,36,30),(36,30,30),(30,30,24),(32,32,32)]
    yield 'independent_kernel_intersection', regular+[(31,31,30),(32,32,30)]
    sphere = [(x,y,z) for x in range(-5,6) for y in range(-5,6) for z in range(-5,6) if x*x+y*y+z*z==25]
    a,b = (5,0,0),(-3,4,0)
    sphere = [a,b]+[p for p in sphere if p not in (a,b)]
    sphere = [tuple(x+20 for x in p) for p in sphere]
    yield 'shell30', sphere
    yield 'shell30_inside', sphere+[(20,20,20)]


def key(center, radius):
    raw = [1]+[-2*x for x in center]+[sum(x*x for x in center)-radius]
    denominator = lcm(*(getattr(x,'denominator',1) for x in raw))
    values = [int(x*denominator) for x in raw]
    divisor = gcd(*values)
    return tuple(x//divisor for x in values)


def exact_records(points, k, counts):
    # Cartesian rational sphere solve; enumerate all partners, then apply the
    # documented one-presentation-per-seed/root rule. No dual hull or atlas.
    good = seeds(points)
    diameter = distance(*points[:2])
    census = {}
    chosen = {}
    for x,y in combinations(range(2,len(points)),2):
        counts['tetrahedra'] += 1
        ids = (0,1,x,y)
        if any(distance(points[i],points[j]) > diameter for i,j in combinations(ids,2)):
            continue
        result = ball([points[i] for i in ids])
        if result is None:
            continue
        center,radius = result
        coefficients = key(center,radius)
        if coefficients not in census:
            depth = sum(distance(p,center) < radius for p in points)
            shell = tuple(i for i,p in enumerate(points) if distance(p,center) == radius)
            census[coefficients] = depth,shell
            counts['point_tests'] += len(points)
        depth,shell = census[coefficients]
        if depth >= k-2:
            continue
        for seed,partner in ((x,y),(y,x)):
            if seed not in good or (partner < seed and partner in good):
                continue
            record = (tuple(sorted(ids)), coefficients, depth, shell)
            identity = seed,coefficients
            if identity not in chosen or partner < chosen[identity][0]:
                chosen[identity] = partner,record
    return sorted(v[1] for v in chosen.values())


def normalized(data):
    return sorted((tuple(r['support']),tuple(map(int,r['coefficients'])),r['depth'],tuple(r['shell']))
                  for r in data['records'])


def append(folder, manifest, record):
    path = folder/f'{len(manifest["records"]):03d}.json.gz'
    path.write_bytes(gzip.compress(json.dumps(record,sort_keys=True).encode(),mtime=0))
    manifest['records'].append(dict(path=path.name,sha256=sha(path)))
    write(folder/'MANIFEST.json',manifest)
    require(record['returncode'] == 0 and not record['stderr'], 'Failed command preserved: '+str(record['command']))


def close(folder, manifest, before):
    require(before == source_pins(), 'Source/library changed during campaign')
    manifest['status'] = 'completed'
    write(folder/'MANIFEST.json',manifest)
    write(folder/'COMPLETION.json',dict(status='completed',manifest_sha256=sha(folder/'MANIFEST.json'),
                                     records=len(manifest['records'])))


def qualify(folder):
    build = BASE/'.build'/folder.name
    require(not build.exists(), 'Build already exists')
    build.mkdir(parents=True)
    before = source_pins()
    manifest = dict(schema='mhgp8_audit_kernel_qualification_v1',status='started',sources=before,records=[])
    flags = ['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'morsehgp3D_v8/src'),
             '-isystem',str(ROOT/'build/v7_boost_gate/extracted/usr/include')]
    commands = [
        ['g++','--version'], ['clang++','--version'],
        ['g++',*flags,'-O2',str(BASE/'lidar_product_probe.cpp'),str(ROOT/'build/v8_q4_shallow_20260920/libmhgp8_p0.a'),'-pthread','-o',str(build/'release')],
        ['clang++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(BASE/'lidar_product_probe.cpp'),
         str(ROOT/'build/v8_q4_shallow_sanitize_20260920/libmhgp8_p0.a'),'-pthread','-o',str(build/'sanitize')]]
    commands += [['python3','-B',*option,str(BASE/name)] for name in ('composition_gate.py','degeneracy_gate.py') for option in ([],['-O'])]
    for command in commands:
        append(folder,manifest,execute(command))
        print('PASS command',len(manifest['records']),flush=True)
    binary_pins = {str((build/name).relative_to(ROOT)):sha(build/name) for name in ('release','sanitize')}
    counts = dict(calls=0,tetrahedra=0,point_tests=0,outputs=0,max_shell=0)
    for name,points in small_cases():
        path=save('gate_'+name,points)
        for k in (3,5,10):
            expected = exact_records(points,k,counts)
            reports=[]
            for binary in (build/'release',build/'sanitize'):
                record = dict(case=name,kmax=k,input_sha256=sha(path),**execute([str(binary),str(path),str(k)]))
                append(folder,manifest,record)
                data=json.loads(record['stdout'])
                require(normalized(data)==expected, 'Independent small oracle differs: '+name)
                reports.append(data)
                counts['calls']+=1
                counts['outputs']+=len(expected)
                counts['max_shell']=max([counts['max_shell']]+[len(r[-1]) for r in expected])
            require(reports[0]['local28']==reports[1]['local28'] and reports[0]['shallow29']==reports[1]['shallow29'],
                    'Release/sanitizer discrete work differs')
        print('PASS oracle',name,flush=True)
    require(counts['outputs']>0 and counts['max_shell']==30,'Vacuous oracle')
    manifest.update(binaries=binary_pins,counts=counts)
    close(folder,manifest,before)


def lidar_cases():
    for name,points,provenance,_ in cases():
        if name.startswith('lidar_'):
            yield name,points,provenance
    for scan in (0,100,200):
        small,_=lidar(scan,8000)
        nearest=sorted(range(1,len(small)),key=lambda i:(distance(small[0],small[i]),i))
        for rank in (8,32,128):
            b=nearest[rank-1]
            for n in (16000,32000):
                points,pin=lidar(scan,n)
                require(points[:8000]==small,'Fixed edge prefix changed')
                yield f'lidar_{scan:06d}_{n}_fixed_rank{rank}',edge_first(points,0,b),dict(
                    input_sha256=pin,original_edge_ids=[0,b],rank_selected_at_n=8000)


def measure(folder,binary):
    before=source_pins()
    manifest=dict(schema='mhgp8_audit_kernel_lidar_v1',status='started',sources=before,binary=str(binary),
                  binary_sha256=sha(binary),records=[],scope='45 selected separate-scan edges, K5/10; actual ports28/29, not their composition or global WSPD')
    for name,points,provenance in lidar_cases():
        path=save(name,points)
        for k in (5,10):
            record=dict(case=name,n=len(points),kmax=k,provenance=provenance,input_sha256=sha(path),
                        **execute([str(binary),str(path),str(k)]))
            append(folder,manifest,record)
        print('PASS pair',name,flush=True)
    require(len(manifest['records'])==90,'Unexpected LiDAR case count')
    require(manifest['binary_sha256']==sha(binary),'Binary changed')
    close(folder,manifest,before)


if __name__ == '__main__':
    mode=sys.argv[1]
    folder=(BASE/sys.argv[2]).resolve()
    require(folder.parent==BASE and not folder.exists(),'Fresh direct child capture required')
    folder.mkdir()
    if mode=='qualify':
        qualify(folder)
    elif mode=='measure':
        measure(folder,Path(sys.argv[3]).resolve())
    else:
        raise RuntimeError('Unknown mode')
