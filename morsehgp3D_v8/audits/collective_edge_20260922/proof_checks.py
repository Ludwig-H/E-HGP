"""Exact audit tests for a standalone prototype, not a native HGP qualification.
Usage: python3 -B proof_checks.py --binary ./collective_probe > RESULTS.json
No external package, no cloud, no network, no changes to the engine.
"""
import argparse
from fractions import Fraction as F
from itertools import combinations, product
import json
from pathlib import Path
import random
import subprocess

M = 262143

def require(ok, message):
    if not ok:
        raise RuntimeError(message)

def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def dist(a,b): return dot(sub(a,b),sub(a,b))
def hv(a,b,z): return dot(sub(z,a),sub(b,z)), cross(sub(b,a),sub(z,a))
def cert(a,b,group,q):
    values=[hv(a,b,z) for z in group]
    h=sum(x[0] for x in values)
    v=tuple(sum(x[1][i] for x in values) for i in range(3))
    return h>0 and (3 if q==3 else 2)*h*h>dot(v,v)

def solve(g,b):
    a=[[F(x) for x in row]+[F(y)] for row,y in zip(g,b)]
    for j in range(len(b)):
        p=next((i for i in range(j,len(b)) if a[i][j]),None)
        if p is None: return None
        a[j],a[p]=a[p],a[j]
        scale=a[j][j]; a[j]=[x/scale for x in a[j]]
        for i in range(len(b)):
            if i!=j:
                scale=a[i][j]; a[i]=[x-scale*y for x,y in zip(a[i],a[j])]
    return [row[-1] for row in a]

def ball(points):
    d=[sub(p,points[0]) for p in points[1:]]
    w=solve([[dot(x,y) for y in d] for x in d],[F(dot(x,x),2) for x in d])
    if w is None or min(w)<=0 or sum(w)>=1: return None
    c=tuple(F(points[0][i])+sum(wj*dj[i] for wj,dj in zip(w,d)) for i in range(3))
    return c,dist(c,points[0])

def encode(queries):
    return ''.join(' '.join(map(str,[q,k,*a,*b,len(pool),*(v for p in pool for v in p)]))+'\n'
                   for q,k,a,b,pool in queries)

def invoke(binary,queries,extra=()):
    p=subprocess.run([str(binary),*extra],input=encode(queries),text=True,capture_output=True,timeout=90)
    require(p.returncode==0, f'probe failed: {p.stderr[:1500]}')
    rows=[json.loads(s) for s in p.stdout.splitlines()]
    require(len(rows)==len(queries),'result count')
    return rows

def validate_row(query,row):
    q,k,a,b,pool=query
    require(row['single']==sum(cert(a,b,[p],q) for p in pool),'singleton mismatch')
    used=set(); credits=0
    for ids in row['groups']:
        require(len(ids) in (1,2,3) and len(set(ids))==len(ids),'invalid group')
        require(not used.intersection(ids),'overlapping groups')
        require(all(0<=i<len(pool) for i in ids),'invalid id')
        used.update(ids)
        if len(ids)<=2:
            require(cert(a,b,[pool[i] for i in ids],q),'false group certificate')
            credits+=1
        else:
            require(all(cert(a,b,[pool[i],pool[j]],q) for i,j in combinations(ids,2)), 'false triangle certificate')
            credits+=2
    require(credits==row['bound'],'incorrect credit count')
    require(row['bound']>=row['single'],'weakened bound')


def special(binary):
    a,b=(400,400,400),(600,600,600); m=(500,500,500)
    pool=[tuple(m[i]+r*v[i] for i in range(3))
          for r in range(80,85) for v in ((1,-1,0),(0,1,-1),(-1,0,1))]
    seeds=[(650,350,500),(650,500,350)]
    cloud=[a,b,*pool,*seeds]
    queries=[(q,10,a,b,pool) for q in (3,4)]
    rows=invoke(binary,queries)
    out={'sites':len(cloud),'pool_size':len(pool),'maximum_possible_matching_credits':len(pool)//2}
    for query,row in zip(queries,rows):
        validate_row(query,row)
        q=query[0]; minimum=len(cloud); valid=0
        for extra in combinations(cloud[2:],q-2):
            support=[a,b,*extra]
            if max(dist(x,y) for x,y in combinations(support,2))>dist(a,b): continue
            value=ball(support)
            if value is None: continue
            c,r=value; valid+=1
            minimum=min(minimum,sum(dist(c,z)<r for z in cloud))
        require(valid>0 and row['single']==0 and row['bound']>=9,'vacuous triangle strengthening')
        require(row['bound']<=minimum,'false depth in fixture')
        out[f'q{q}']={'single':row['single'],'greedy_matching':row['matching'],
                      'bound_with_triangles':row['bound'],'positive_owned_supports':valid,
                      'minimum_true_depth':minimum,'groups':row['groups']}
    rng=random.Random(7722); variants=[]
    for _ in range(40):
        ids=list(range(len(pool))); rng.shuffle(ids)
        p=[pool[i] for i in ids]
        for q in (3,4):
            variants.append((q,10,tuple(200000+50*x for x in a),
                             tuple(200000+50*x for x in b),
                             [tuple(200000+50*x for x in z) for z in p]))
    vrows=invoke(binary,variants)
    for query,row in zip(variants,vrows): validate_row(query,row)
    out['permutation_upper_range_queries']=len(variants)
    out['permutation_bounds']={str(v):sum(r['bound']==v for r in vrows) for v in sorted({r['bound'] for r in vrows})}
    return out


def clouds():
    rng=random.Random(2026092201)
    for trial in range(24):
        family='uniform_small' if trial<8 else 'uniform_u18' if trial<16 else 'slab' if trial<20 else 'parallel_rows'
        p=[]
        while len(p)<14:
            if family=='uniform_small': z=tuple(rng.randrange(65) for _ in range(3))
            elif family=='uniform_u18': z=tuple(rng.randrange(M+1) for _ in range(3))
            elif family=='slab': z=(rng.randrange(500),rng.randrange(500),rng.randrange(2))
            else: z=(1000 if len(p)%2==0 else 60000+1000*(trial-20),(100+7*trial)*(len(p)//2),0)
            if z not in p: p.append(z)
        yield family,p


def exhaustive(binary):
    queries=[]; minima=[]; families=[]; counts={'clouds':0,'positive_owned_balls':0,'oracle_power_tests':0}
    for family,p in clouds():
        counts['clouds']+=1
        for q in (3,4):
            least={}
            for ids in combinations(range(len(p)),q):
                value=ball([p[i] for i in ids])
                if value is None: continue
                c,r=value
                owner=min(combinations(ids,2),key=lambda e:(-dist(p[e[0]],p[e[1]]),e))
                depth=sum(dist(c,z)<r for z in p)
                counts['positive_owned_balls']+=1; counts['oracle_power_tests']+=len(p)
                least[owner]=min(least.get(owner,len(p)),depth)
            for a,b in combinations(range(len(p)),2):
                pool=[p[i] for i in range(len(p)) if i not in (a,b)]
                for k in (5,10):
                    queries.append((q,k,p[a],p[b],pool)); minima.append(least.get((a,b))); families.append(family)
    rows=invoke(binary,queries)
    unguarded=invoke(binary,queries,['--unguarded'])
    counts['queries']=len(queries); counts['nonvacuous_queries']=0; counts['false_depth_bounds']=0
    stats={}
    for query,row,raw,minimum,family in zip(queries,rows,unguarded,minima,families):
        validate_row(query,row)
        q,k,*_=query; key=f'{family}/q{q}/K{k}'
        s=stats.setdefault(key,{'queries':0,'nonvacuous':0,'individual_rejections':0,'pair_rejections':0,
                               'triangle_rejections':0,'extra_vs_individual':0,'extra_nonvacuous':0,'pair_tests':0,'unguarded_pair_tests':0,'guard_skips':0})
        s['queries']+=1; s['pair_tests']+=row['pair_tests']
        s['unguarded_pair_tests']+=raw['pair_tests']; s['guard_skips']+=row['guard_skip']
        t=k-q+2
        old=row['single']>=t; pair=row['matching']>=t; new=row['bound']>=t
        require(new==(raw['bound']>=t),'guard changed rejection')
        s['individual_rejections']+=old; s['pair_rejections']+=pair; s['triangle_rejections']+=new
        s['extra_vs_individual']+=new and not old
        if minimum is not None:
            counts['nonvacuous_queries']+=1; s['nonvacuous']+=1
            require(row['bound']<=minimum,f'false bound: {key}, {row}, true={minimum}')
            s['extra_nonvacuous']+=new and not old
    return {**counts,'by_family':stats}


def mutations(binary):
    a,b,x=(0,100,100),(200,100,100),(100,220,100)
    ordinary=[(100,165,100),(100,5,100),(101,6,100)]
    cases=[('overlap',(3,3,a,b,ordinary),[a,b,x]),
           ('pair_two',(3,3,a,b,ordinary[:2]),[a,b,x]),
           ('q4_alpha3',(4,3,(20,20,20),(20,40,40),[(12,30,30),(12,31,30)]),
            [(20,20,20),(20,40,40),(40,20,40),(40,40,20)]),
           ('nonstrict',(4,3,(10,10,10),(10,16,16),[(8,12,12),(8,14,14)]),
            [(10,10,10),(10,16,16),(16,10,16),(16,16,10)]),
           ('no_h',(3,2,(10,10,10),(20,10,10),[(30,10,10),(40,10,10)]),
            [(10,10,10),(20,10,10),(15,16,10)])]
    out={}
    for mutant,query,support in cases:
        value=ball(support); require(value is not None,'mutation lacks positive support')
        c,r=value; true=sum(dist(c,z)<r for z in query[-1])
        good=invoke(binary,[query],['--unguarded'])[0]; bad=invoke(binary,[query],['--mutant',mutant])[0]
        validate_row(query,good)
        require(good['bound']<=true<bad['bound'],f'noncausal mutant {mutant}: {good}, {bad}, true {true}')
        out[mutant]={'true_pool_depth':true,'correct_bound':good['bound'],'mutant_bound':bad['bound']}
    return out


def boxes(binary):
    ac=list(product((0,2),(199,201),(199,201)))
    bc=list(product((198,200),(199,201),(199,201)))
    groups=[[(100,200+r,200),(100,200-r,200)] for r in range(61,70)]
    aa=list(product(range(3),range(199,202),range(199,202)))
    bb=list(product(range(198,201),range(199,202),range(199,202)))
    checks=0
    for q in (3,4):
        for g in groups:
            require(all(cert(a,b,g,q) for a in ac for b in bc),'corner certificate absent')
            for a in aa:
                for b in bb:
                    require(cert(a,b,g,q),'corner certificate not transferred')
                    checks+=1
    g=[(100,265,200),(100,135,200)]
    require(cert((0,200,200),(200,200,200),g,4),'representative fixture')
    require(not cert((99,200,200),(101,200,200),g,4),'representative mutant not killed')
    # A different successful witness/group at each corner does NOT imply
    # a certificate throughout the box. At the middle, both pool sites are
    # exact contacts of a valid positive owned q3/q4 ball.
    b=(300,200,200); middle=(200,200,200)
    pool=[(250,150,200),(250,250,200)]
    switching={}
    for q in (3,4):
        aa2=[(200,100,200),(200,300,200),middle]
        queries=[(q,q-1,a,b,pool) for a in aa2]
        rows=invoke(binary,queries)
        support=[middle,b,(250,200,260)] if q==3 else [middle,b,(250,230,270),(250,170,270)]
        value=ball(support); require(value is not None,'switching fixture lacks positive ball')
        require(max(dist(x,y) for x,y in combinations(support,2))==dist(middle,b),'nonowned switching fixture')
        c,r=value
        require(all(dist(c,z)==r for z in pool),'switching contacts')
        require(rows[0]['bound']>=1 and rows[1]['bound']>=1 and rows[2]['bound']==0,'switching fixture failed')
        switching[str(q)]={'corner_bounds':[x['bound'] for x in rows[:2]],'middle_bound':rows[2]['bound'],'true_middle_pool_depth':0}
    return {'certified_groups_per_lane':9,'corner_tests':2*9*64,'interior_endpoint_checks':checks,
            'one_representative_is_insufficient':True,'changing_groups_at_corners_is_unsound':switching}


def extremes(binary):
    pool=list(product((0,1,M-1,M),repeat=3)); rng=random.Random(8822)
    queries=[]
    for _ in range(40):
        a,b=rng.sample(pool,2)
        for q in (3,4):
            for k in (5,10): queries.append((q,k,a,b,pool))
    for query,row in zip(queries,invoke(binary,queries)): validate_row(query,row)
    return len(queries)


def refusals(binary):
    bad=['2 5 0 0 0 1 1 1 0\n','4 2 0 0 0 1 1 1 0\n','3 11 0 0 0 1 1 1 0\n',
         '3 5 0 0 0 0 0 0 0\n','3 5 -1 0 0 1 1 1 0\n','3 5 0 0 0 262144 1 1 0\n',
         '3 5 0 0 0 1 1 1 2 2 2 2 2 2 2\n','3 5 0 0 0 1 1 1 65\n','3 5 0 0 0\n']
    for text in bad:
        p=subprocess.run([str(binary)],input=text,text=True,capture_output=True,timeout=10)
        require(p.returncode==2 and p.stderr and not p.stdout,'invalid input not refused')
    return len(bad)


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--binary',type=Path,required=True)
    args=parser.parse_args(); binary=args.binary.resolve()
    result={'scope':'Independent C++ prototype + exhaustive Fraction/Gram oracle; no HGP build or LiDAR benchmark',
            'triangle_fixture':special(binary),'exhaustive':exhaustive(binary),
            'mutations':mutations(binary),'rectangles':boxes(binary),'refusals':refusals(binary),'extreme_queries':extremes(binary)}
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__=='__main__': main()
