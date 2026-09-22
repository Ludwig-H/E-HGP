"""Bounded-pool microbenchmark. Not HGP, not LiDAR, no pool-acquisition timing.
Usage: python3 microbench.py --binary ./collective_probe --output /tmp/new-dir
The output directory must not already exist. Five rotated native repetitions.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess
import sys
from proof_checks import cert, encode, invoke, require

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--binary',required=True,type=Path)
    ap.add_argument('--output',required=True,type=Path); ap.add_argument('--rounds',type=int,default=100)
    args=ap.parse_args(); args.output.mkdir(parents=True,exist_ok=False)
    rng=random.Random(2026092202); report={}
    for family in ('volume','two_planes','eight_clusters','sphere'):
        points=[]
        while len(points)<512:
            if family=='volume': p=tuple(rng.randrange(131072) for _ in range(3))
            elif family=='two_planes': p=(1000 if len(points)%2==0 else 120000,rng.randrange(64000),rng.randrange(64000))
            elif family=='eight_clusters':
                p=tuple((20000 if (len(points)%8)&(1<<i) else 110000)+rng.randrange(-4000,4001) for i in range(3))
            else:
                d=[rng.gauss(0,1) for _ in range(3)]; norm=math.sqrt(sum(x*x for x in d))
                p=tuple(round(65535+50000*x/norm) for x in d)
            if p not in points: points.append(p)
        queries=[]
        # Diagnostic pools: 32 nearest points to the midpoint, selected by a
        # full Python scan. That deliberately optimistic acquisition is NOT
        # included in the measured kernel and is NOT a proposed production path.
        for _ in range(256):
            a,b=rng.sample(points,2)
            pool=sorted((p for p in points if p not in (a,b)),
                        key=lambda p:sum((2*p[i]-a[i]-b[i])**2 for i in range(3)))[:32]
            for q in (3,4):
                for k in (5,10):
                    if sum(cert(a,b,[p],q) for p in pool)<k-q+2:
                        queries.append((q,k,a,b,pool))
        require(queries,'empty benchmark corpus')
        payload=encode(queries); (args.output/f'{family}.txt').write_text(payload)
        rows=invoke(args.binary.resolve(),queries)
        raw=invoke(args.binary.resolve(),queries,['--unguarded'])
        for query,x,y in zip(queries,rows,raw):
            t=query[1]-query[0]+2; require((x['bound']>=t)==(y['bound']>=t),'guard decision drift')
        captures=[]
        for repeat in range(5):
            p=subprocess.run([str(args.binary.resolve()),'--bench',str(args.rounds),str(repeat)],
                             input=payload,text=True,capture_output=True,timeout=90)
            require(p.returncode==0,p.stderr)
            capture=[json.loads(s) for s in p.stdout.splitlines()]; captures.append(capture)
        medians={str(mode):statistics.median(next(x for x in c if x['mode']==mode)['ns'] /
                    (len(queries)*args.rounds) for c in captures) for mode in range(4)}
        report[family]={'queries_surviving_individual_pool_test':len(queries),'pool_sites':32,
            'pool_sha256':hashlib.sha256(payload.encode()).hexdigest(),
            'extra_rejections':sum(x['bound']>=q[1]-q[0]+2 for q,x in zip(queries,rows)),
            'guard_skips':sum(x['guard_skip'] for x in rows),
            'pair_tests_unguarded':sum(x['pair_tests'] for x in raw),
            'pair_tests_guarded':sum(x['pair_tests'] for x in rows),
            'median_ns_per_query':medians,'repetitions':captures}
    metadata={'scope':'Standalone C++ proof prototype; preselected pools; no HGP pipeline or real LiDAR; outputs allocate audit groups',
        'modes':{'0':'individual pool test','1':'pairs without guards','2':'pairs plus triangles without guards',
                 '3':'pairs plus triangles with impossibility guards'},
        'python':sys.version,'platform':platform.platform(),'load_average':os.getloadavg(),
        'rounds':args.rounds,'binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        'families':report}
    (args.output/'BENCH.json').write_text(json.dumps(metadata,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:{a:b for a,b in v.items() if a!='repetitions'} for k,v in report.items()},indent=2))

if __name__=='__main__': main()
