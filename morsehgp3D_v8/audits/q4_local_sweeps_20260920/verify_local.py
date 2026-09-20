#!/usr/bin/env python3
"""Independent Cartesian oracle for fragment depths, shells and useful roots."""
import atexit
from fractions import Fraction as F
import gzip
import hashlib
from itertools import combinations
import json
from pathlib import Path
import subprocess
import sys

from inputs import BASE, ball, distance, gate_fixtures, save, seeds


def require(condition,message):
    if not condition:
        raise RuntimeError(message)


def useful_roots(points,k,counts):
    d=distance(*points[:2])
    valid_seeds=seeds(points)
    roots=set()
    for x,y in combinations(range(2,len(points)),2):
        counts['tetra_presentations']+=1
        ids=(0,1,x,y)
        if any(distance(points[i],points[j])>d for i,j in combinations(ids,2)):
            continue
        value=ball([points[i] for i in ids])
        if value is None:
            continue
        center,radius=value
        counts['positive_owned_tetrahedra']+=1
        depth=sum(distance(p,center)<radius for p in points)
        if depth<k-2:
            for seed in valid_seeds.intersection((x,y)):
                roots.add((seed,tuple(center)))
    return roots


def check(points,k,data,expected,counts):
    require(data['mode']==2 and data['n']==len(points) and data['kmax']==k,'CLI/output mismatch')
    require(data['seeds']==len(seeds(points)),'Seed enumeration mismatch')
    a,b=points[:2];d=distance(a,b)
    cover=[i for i,z in enumerate(points) if sum((2*z[j]-a[j]-b[j])**2 for j in range(3))<=4*d]
    require(data['cover_sites']==len(cover),'Cover population mismatch')
    basis_a,basis_b=data['basis_A'],data['basis_B']
    emitted=set()
    hash_sum=hash_xor=0
    for record in data['roots']:
        xi,eta=F(int(record['xi_num']),int(record['den'])),F(int(record['eta_num']),int(record['den']))
        center=tuple((F(a[j]+b[j])+basis_a[j]*xi+basis_b[j]*eta)/2 for j in range(3))
        radius=distance(a,center)
        seed=record['seed']
        require(distance(b,center)==radius and distance(points[seed],center)==radius,'Root off seed family')
        depth=sum(distance(points[i],center)<radius for i in cover)
        shell=[i for i in cover if distance(points[i],center)==radius]
        require(record['depth']==depth<k-2,'Wrong strict covered depth')
        require(record['shell']==shell,'Missing, duplicated or unordered shell IDs')
        identity=(seed,center)
        require(identity not in emitted,'Root emitted by multiple cells')
        emitted.add(identity)
        counts['emitted_roots']+=1
        counts['covered_point_tests']+=len(cover)
        counts['max_shell']=max(counts['max_shell'],len(shell))
        canonical=f'{seed}|{record["xi_num"]}|{record["eta_num"]}|{record["den"]}|{depth}|'+''.join(f'{i},' for i in shell)
        digest=int.from_bytes(hashlib.sha256(canonical.encode()).digest(),'big')
        hash_sum=(hash_sum+digest) % (1<<256)
        hash_xor^=digest
    require(expected<=emitted,'An accepted positive owned root was lost')
    counts['required_positive_roots']+=len(expected)
    require(data['work']['lowdepthgroups']==len(emitted),'Root output counter mismatch')
    require(data['root_digest']==dict(count=len(emitted),sum_sha256_mod_2_256=f'{hash_sum:064x}',xor_sha256=f'{hash_xor:064x}'),
            'Record digest mismatch')


def main():
    exe,target=Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve()
    require(not target.exists(),'Refusing to overwrite a gate capture')
    records=[]
    atexit.register(lambda:target.write_bytes(gzip.compress(json.dumps(records).encode(),mtime=0)))
    counts=dict(calls=0,mode_pairs=0,tetra_presentations=0,positive_owned_tetrahedra=0,required_positive_roots=0,
                emitted_roots=0,covered_point_tests=0,max_shell=0)
    settings=[(3,0,1,0),(3,5,341,1),(5,5,341,0),(5,7,4096,1),(10,7,1,1),(10,7,4096,0),(10,7,4096,1)]
    for name,points in gate_fixtures():
        file,digest=save('gate_'+name,points)
        expected={k:useful_roots(points,k,counts) for k in (3,5,10)}
        for k,depth,budget,domain in settings:
            command=[str(exe),str(file),str(k),str(depth),str(budget),str(domain),'2']
            child=subprocess.run(command,capture_output=True,text=True,timeout=60)
            record=dict(case=name,input_sha256=digest,command=command,returncode=child.returncode,
                        stdout=child.stdout,stderr=child.stderr)
            records.append(record)
            require(child.returncode==0,'Local sweep executable failed: '+child.stderr)
            data=json.loads(child.stdout)
            require(data['input_sha256']==digest,'Input digest mismatch')
            check(points,k,data,expected[k],counts)
            counts['calls']+=1
        if name in ('lower_left_tangent','shell30','shell30_inside'):
            detailed=data
            for mode in (0,1):
                command=[str(exe),str(file),'10','7','4096','1',str(mode)]
                child=subprocess.run(command,capture_output=True,text=True,timeout=60)
                records.append(dict(case=name,input_sha256=digest,command=command,returncode=child.returncode,
                                    stdout=child.stdout,stderr=child.stderr))
                require(child.returncode==0,'Mode comparison failed')
                pair=json.loads(child.stdout)
                for field in ('I','W','A','plane_cell_tests','cell_visits','query_visits'):
                    require(pair['work'][field]==detailed['work'][field],'Count/sweep geometry differs')
                if mode==1:
                    require(pair['root_digest']==detailed['root_digest'],'Mode1/2 output digest differs')
                    for field in ('events','sort_comparisons','groups','owned_groups','lowdepthgroups','shell_ids'):
                        require(pair['work'][field]==detailed['work'][field],'Mode1/2 sweep work differs')
                else:
                    require(pair['work']['groups']==pair['work']['events']==pair['root_digest']['count']==0,'Count mode emitted roots')
                counts['mode_pairs']+=1
    require(counts['required_positive_roots']>50 and counts['max_shell']>=30,'Vacant gate')
    target.write_bytes(gzip.compress(json.dumps(records).encode(),mtime=0))
    print(json.dumps(dict(status='passed',scope='audit-only local covered roots; not a product or FULL qualification',
          binary_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
          capture_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),counts=counts),sort_keys=True))


if __name__=='__main__':
    main()
