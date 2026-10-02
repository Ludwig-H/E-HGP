"""Tiny analytic models; no product import, native call, benchmark, or large allocation."""
from fractions import Fraction
from itertools import permutations, product


def distance_squared(a,b):
    return sum((x-y)**2 for x,y in zip(a,b))


def certified_list(points,lo,hi,k):
    # Independent vertex test: differences of squared distances are affine.
    corners=list(product(*[(a,b) for a,b in zip(lo,hi)]))
    mid=tuple(Fraction(a+b,2) for a,b in zip(lo,hi))
    witnesses=sorted(range(len(points)),key=lambda i:(distance_squared(points[i],mid),i))[:min(len(points),3*k)]
    retained=[]
    for i,a in enumerate(points):
        dominant=[j for j in witnesses if all(distance_squared(a,c)>distance_squared(points[j],c) for c in corners)]
        if len(dominant)<k:retained.append(i)
    return {'sites':retained,'witnesses':witnesses,'allocated_capacity':len(points)}


def overlap_model():
    points=[(x,0,0) for x in range(5)]
    root=certified_list(points,(0,0,0),(5,1,1),1)
    left=certified_list(points,(0,0,0),(2,1,1),1)
    right=certified_list(points,(2,0,0),(5,1,1),1)
    return {'n':5,'kmax':1,'leaf_size':4,'root':root,'left':left,'right':right,'logical_children':len(left['sites'])+len(right['sites']),'children_capacity':left['allocated_capacity']+right['allocated_capacity'],'interpretation':'Closed dominance keeps SiteIdx 2 in both disjoint half-open center boxes. Static analytic fixture; not a native execution.'}


def prefix_checked(counts,limit=(1<<64)-1):
    out=[0]
    for count in counts:
        if count<0 or count>limit-out[-1]:raise OverflowError('capacity prefix')
        out.append(out[-1]+count)
    return out


def scatter_model():
    jobs=[[(Fraction(2,3),(0,1),[10,11]),(Fraction(1,3),(2,3),[12,13,14])],[(Fraction(1,3),(4,5),[20,21,22])],[],[(Fraction(1),(6,7),[30]),(Fraction(2,3),(8,9),[31,32,33,34])]]
    bp=prefix_checked([len(j) for j in jobs]);pp=prefix_checked([sum(len(v) for _,_,v in j) for j in jobs])
    expected=sorted([(lev,supp,val) for job in jobs for lev,supp,val in job],key=lambda r:r[:2])
    outputs=[];wrong=None
    for order in permutations(range(len(jobs))):
        records=[None]*bp[-1];population=[None]*pp[-1]
        for ordinal in order:
            local=0
            for row,(lev,supp,val) in enumerate(jobs[ordinal]):
                population[pp[ordinal]+local:pp[ordinal]+local+len(val)]=val
                records[bp[ordinal]+row]=(lev,supp,pp[ordinal]+local,local,len(val))
                local+=len(val)
        records.sort(key=lambda r:r[:2]);levels=sorted(set(r[0] for r in records));ranks={lev:i+1 for i,lev in enumerate(levels)}
        observed=[(lev,supp,population[begin:begin+length]) for lev,supp,begin,_,length in records]
        if observed!=expected:raise RuntimeError('rebased scatter fails')
        outputs.append([(str(lev),list(supp),ranks[lev],val) for lev,supp,val in observed])
        if wrong is None:wrong=[(str(lev),list(supp),population[local:local+length]) for lev,supp,_,local,length in records]
    if any(x!=outputs[0] for x in outputs):raise RuntimeError('schedule-dependent result')
    correct=[(lev,supp,val) for lev,supp,_,val in outputs[0]]
    if wrong==correct:raise RuntimeError('missing-base mutant survived')
    virtual_ok=prefix_checked([1,(1<<64)-2])[-1]==(1<<64)-1
    refused=False
    try:prefix_checked([(1<<64)-1,1])
    except OverflowError:refused=True
    return {'balls_prefix':bp,'population_prefix':pp,'completion_orders':len(outputs),'canonical':outputs[0],'naive_local_population_begin':wrong,'naive_rejected':wrong!=correct,'u64_boundary_exact':virtual_ok,'u64_overflow_refused':refused,'scope':'Future parallel scatter contract; sequential Collector currently has global zero base and is correct.'}


def models():return {'overlap':overlap_model(),'scatter':scatter_model()}
