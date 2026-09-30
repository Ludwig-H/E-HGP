"""Exact event oracle on the translated historical tree, with rational sqrt enclosures."""
from fractions import Fraction as F
import json
from math import isfinite,isqrt
from pathlib import Path
import sys
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parent


def require(ok,message):
    if not ok: raise ValueError(message)


def expression(beta):
    return {} if beta is None else {F(beta):F(1)}


def times(n,a):
    return {beta:n*c for beta,c in a.items() if n*c}


def plus(a,b):
    out=dict(a)
    for beta,c in b.items():
        out[beta]=out.get(beta,F(0))+c
        if not out[beta]: del out[beta]
    return out


def minus(a,b): return plus(a,times(-1,b))


def bounds(a,z,bits=160):
    lo=hi=F(0);Q=1<<bits
    for beta,c in a.items():
        if z==2:
            left=right=1/beta
        else:
            # floor(Q sqrt(1/beta)), computed with integers only.
            s=isqrt((beta.denominator*Q*Q)//beta.numerator)
            left=F(s,Q)
            right=left if beta.numerator*s*s==beta.denominator*Q*Q else F(s+1,Q)
        if c>=0:lo+=c*left;hi+=c*right
        else:lo+=c*right;hi+=c*left
    return lo,hi


def greater(a,b,z):
    difference=minus(a,b)
    for bits in (160,320,640):
        lo,hi=bounds(difference,z,bits)
        if lo>0:return True
        if hi<0 or lo==hi==0:return False
    raise ValueError('unresolved exact EOM comparison')


def near(number,a,z):
    if type(number) not in (int,float) or not isfinite(number):return False
    value=F(number);lo,hi=bounds(a,z)
    tolerance=F(1,10**12)
    return lo-tolerance<=value<=hi+tolerance


def truth(mcs,z,single):
    raw=json.loads((ROOT/'historical/internal_k3.json').read_text())
    levels=[F(int(n),int(d)) for n,d in raw['levels']]
    o=raw['orders'][0];nodes=o['nodes']
    birth=[levels[v[0]] for v in nodes]
    children=[v[3] for v in nodes]
    entry=[levels[r] for r in o['cover_lv']]
    target=o['cover_node'];n=len(target)
    descendants=[]
    for v in range(len(nodes)):
        own={x for x,node in enumerate(target) if node==v}
        for c in children[v]:own|=descendants[c]
        descendants.append(own)
    roots=[v for v,row in enumerate(nodes) if row[1]==-1]
    require(len(roots)==1,'root')
    clusters=[dict(parent=None,birth={},stability={},mass=n)]
    alive={roots[0]:(0,set(range(n)))}
    point_cluster=[None]*n;point_lambda=[None]*n;collapses=[]
    def drop(points,c,value):
        for x in points:
            require(point_cluster[x] is None,'double payment')
            point_cluster[x]=c;point_lambda[x]=value
            clusters[c]['stability']=plus(clusters[c]['stability'],minus(value,clusters[c]['birth']))
    for beta in sorted(set(entry)|set(birth),reverse=True):
        value=expression(beta)
        for v in sorted(tuple(alive)):
            c,points=alive[v]
            cohort={x for x in points if entry[x]==beta}
            drop(cohort,c,value);points-=cohort
            if not points:del alive[v]
            elif len(points)<mcs:
                collapses.append(dict(beta=str(beta),remaining=len(points),point_ids=[raw['sites'][x][0] for x in sorted(points)]))
                drop(points,c,value);del alive[v]
        for v in reversed(range(len(nodes))):
            if birth[v]!=beta or v not in alive:continue
            c,points=alive.pop(v)
            parts=[(child,points&descendants[child]) for child in children[v]]
            big=[(child,part) for child,part in parts if len(part)>=mcs]
            if len(big)>=2:
                for child,part in parts:
                    if len(part)<mcs:drop(part,c,value);continue
                    clusters[c]['stability']=plus(clusters[c]['stability'],times(len(part),minus(value,clusters[c]['birth'])))
                    new=len(clusters);clusters.append(dict(parent=c,birth=value,stability={},mass=len(part)))
                    alive[child]=(new,set(part))
            elif len(big)==1:
                child,keep=big[0];drop(points-keep,c,value);alive[child]=(c,set(keep))
            else:drop(points,c,value)
    require(not alive and all(c is not None for c in point_cluster),'unfinished sweep')
    kids=[[] for _ in clusters]
    for c,row in enumerate(clusters):
        if row['parent'] is not None:kids[row['parent']].append(c)
    best={};chosen=set()
    for c in reversed(range(len(clusters))):
        sub={}
        for k in kids[c]:sub=plus(sub,best[k])
        if kids[c] and ((c==0 and not single) or greater(sub,clusters[c]['stability'],z)):
            best[c]=sub
        else:best[c]=clusters[c]['stability'];chosen.add(c)
    if not single:chosen.discard(0)
    selected=[]
    for c in sorted(chosen):
        parent=clusters[c]['parent']
        while parent is not None and parent not in chosen:parent=clusters[parent]['parent']
        if parent is None:selected.append(c)
    ids={c:i for i,c in enumerate(selected)};labels=[]
    for c in point_cluster:
        while c is not None and c not in ids:c=clusters[c]['parent']
        labels.append(-1 if c is None else ids[c])
    return dict(clusters=clusters,point_cluster=point_cluster,point_lambda=point_lambda,
                selected=selected,labels=labels,collapses=collapses)


def judge(path):
    rows=[json.loads(line) for line in path.read_text().splitlines()]
    require(len(rows)==12,'row floor')
    seen=set();different=0;out=[]
    for row in rows:
        key=(row['mcs'],row['z'],row['allow_single'])
        require(type(key[0]) is int and key[0] in (1,2,6) and type(key[1]) is int and
                key[1] in (1,2) and type(key[2]) is bool and key not in seen,'case identity')
        seen.add(key);mcs,z,single=key
        require(row['api_valid'] is True and row['K']==3 and row['case']=='historical_cover' and
                row['point_ids']==[3,4,5,1,0,2],'materialized identity')
        exact=truth(*key)
        require(len(row['clusters'])==len(exact['clusters']) and len(row['point_lambda'])==6,'output coverage')
        has_difference=False
        for c,(actual,expected) in enumerate(zip(row['clusters'],exact['clusters'])):
            require(actual['id']==c and actual['parent']==expected['parent'] and actual['mass']==expected['mass']
                    and near(actual['birth'],expected['birth'],z),'unexpected cluster topology')
            has_difference|=not near(actual['stability'],expected['stability'],z)
        has_difference|=any(not near(a,b,z) for a,b in zip(row['point_lambda'],exact['point_lambda']))
        require(has_difference==(mcs==6),'unexpected concordance/disagreement')
        require(row['selected']==exact['selected'] and row['labels']==exact['labels'] and
                row['point_cluster']==exact['point_cluster'],'unexpected selected/labels/origin difference')
        if mcs==6:
            expected_old=plus(expression(25),times(5,expression(F(169,9))))
            require(len(row['clusters'])==1 and near(row['clusters'][0]['stability'],expected_old,z),
                    'not expected historical-body score')
            require(exact['collapses']==[dict(beta='25',remaining=5,point_ids=[3,4,5,1,2])],
                    'wrong dynamic collapse')
        different+=has_difference
        summary=[]
        for cluster in exact['clusters']:
            lo,hi=bounds(cluster['stability'],z)
            summary.append(dict(expression={str(beta):str(c) for beta,c in cluster['stability'].items()},
                                certified_lo=str(lo),certified_hi=str(hi)))
        out.append(dict(mcs=mcs,z=z,allow_single=single,differs=has_difference,
                        native_stabilities=[c['stability'] for c in row['clusters']],
                        exact_stabilities=summary,collapses=exact['collapses'],
                        native_selected=row['selected'],exact_selected=exact['selected']))
    require(seen=={(m,z,s) for m in (1,2,6) for z in (1,2) for s in (False,True)}
            and different==4,'nonvacuity')
    return dict(status='HISTORICAL_COVER_SCORE_DIFFERENCE_CONFIRMED',rows=12,
                exact_positive_controls=8,differing_rows=4,EOM_flips=0,comparisons=out,
                scope='new head execution from historical 6-point native cover; no new generator',native_generator_invocations=0)


if __name__=='__main__':
    require(len(sys.argv)==2,'usage check.py native.stdout')
    print(json.dumps(judge(Path(sys.argv[1])),sort_keys=True,indent=2))
