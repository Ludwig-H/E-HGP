#!/usr/bin/env python3
"""Independent exact local-sweep model, with explicit guards under python -O.
No product imports. Abstract affine fixtures and real u16 geometry are labelled.
"""
from fractions import Fraction as F
from itertools import product
import json
import random

M = 65535
I128 = 1 << 127


def require(condition, why):
    if not condition:
        raise RuntimeError(why)


def sign(x):
    return (x > 0)-(x < 0)


def dot(a, b):
    return sum(x*y for x, y in zip(a,b))


def det(rows):
    x,y,z = rows
    return (x[0]*(y[1]*z[2]-y[2]*z[1])-
            x[1]*(y[0]*z[2]-y[2]*z[0])+
            x[2]*(y[0]*z[1]-y[1]*z[0]))


def value(form, point):
    return form[0]+form[1]*point[0]+form[2]*point[1]


def geometry_forms(a, b, points):
    v = tuple(y-x for x,y in zip(a,b))
    D = dot(v,v)
    if not D:
        raise ValueError("repeated endpoints")
    k = max(range(3),key=lambda d:abs(v[d]))
    i,j = [d for d in range(3) if d != k]
    h,sgn = abs(v[k]),sign(v[k])
    A,B = [0]*3,[0]*3
    A[i],A[k],B[j],B[k] = h,-sgn*v[i],h,-sgn*v[j]
    out=[]
    for z in points:
        w=tuple(2*z[d]-a[d]-b[d] for d in range(3))
        out.append((dot(w,w)-D,-2*dot(w,A),-2*dot(w,B)))
    return out


def oriented(seed, *forms, swap_only_seed=False):
    require(seed[1] or seed[2], "seed does not define a line")
    if seed[2]:
        return (seed,)+forms
    swap=lambda t:(t[0],t[2],t[1])
    return (swap(seed),)+(forms if swap_only_seed else tuple(map(swap,forms)))


def restricted(seed, form):
    c,a,b=seed
    p=form[0]*b-form[2]*c
    s=form[1]*b-form[2]*a
    return p,s


def compare(seed,z,w,mutant=None):
    x,z,w=oriented(seed,z,w,swap_only_seed=mutant=="swap_seed_only")
    pz,sz=restricted(x,z)
    pw,sw=restricted(x,w)
    require(sz and sw, "parallel form passed to root comparator")
    delta=det((x,z,w))
    if mutant=="split_equal_roots" and delta==0:
        return 1
    return -(1 if mutant=="ignore_b_sign" else sign(x[2]))*sign(delta)*(1 if mutant=="ignore_denominator_signs" else sign(sz)*sign(sw))


def root_point(seed,other):
    c,a,b=seed
    cz,az,bz=other
    denominator=a*bz-az*b
    if not denominator:
        return None
    nx=b*cz-bz*c
    ny=az*c-a*cz
    if denominator<0:
        nx,ny,denominator=-nx,-ny,-denominator
    return (F(nx,denominator),F(ny,denominator)),(nx,ny,denominator)


def in_cell(point,cell):
    return all(lo<=x<=hi for x,(lo,hi) in zip(point,cell))


def classify(forms,cell,base=0,received=None,mutant=False):
    inside=[]
    active=[]
    candidates=range(len(forms)) if received is None else received
    for index in candidates:
        values=[value(forms[index],point) for point in product(*cell)]
        minimum,maximum=min(values),max(values)
        if maximum<0:
            inside.append(index)
        elif minimum>0 or (mutant and minimum==0):
            pass
        else:
            active.append(index)
    return base+len(inside),tuple(active),tuple(inside)


def sweep(forms,seed,cell,base,active,threshold=None):
    """Sweep ALL active roots from -infinity; emit only roots in the cell.

    base+active depth outside the cell is an algebraic extension, not a
    claim about global depth. No root is discarded before its count update.
    threshold enables the deliberately unsafe saturation mutant only.
    """
    x,*oriented_forms=oriented(seed,*forms)
    constants=[]
    groups={}
    count=base
    for index in active:
        p,s=restricted(x,oriented_forms[index])
        if s==0:
            if p==0:
                constants.append(index)
            elif p*x[2]<0:
                count+=1
        else:
            r=F(-p,s)
            entry=sign(s)*sign(x[2])<0
            groups.setdefault(r,[]).append((index,entry))
            count+=not entry
    if threshold is not None:
        count=min(count,threshold)
    out=[]
    for r,group in sorted(groups.items()):
        count-=sum(not entry for _,entry in group)
        point=root_point(seed,forms[group[0][0]])[0]
        if in_cell(point,cell):
            out.append((point,count,tuple(sorted(constants+[index for index,_ in group]))))
        count+=sum(entry for _,entry in group)
        if threshold is not None:
            count=min(count,threshold)
    return out


def direct(forms,point):
    return sum(value(form,point)<0 for form in forms),tuple(i for i,form in enumerate(forms) if value(form,point)==0)


def check_sweep(forms,seed,cell,counts):
    base,active,_=classify(forms,cell)
    rows=sweep(forms,seed,cell,base,active)
    expected_points=set()
    for form in forms:
        answer=root_point(seed,form)
        if answer is not None and in_cell(answer[0],cell):
            expected_points.add(answer[0])
    require({point for point,_,_ in rows}==expected_points,"local sweep lost or duplicated a root")
    for point,depth,shell in rows:
        require((depth,shell)==direct(forms,point),"local depth/shell differs from direct global forms")
        counts["local_root_checks"]+=1
    # Prefix inheritance: keep all forms touching zero, including min=0.
    midpoint=tuple((lo+hi)/2 for lo,hi in cell)
    for child in product(*[[(lo,mid),(mid,hi)] for (lo,hi),mid in zip(cell,midpoint)]):
        c,a,_=classify(forms,child,base,active)
        independently,aa,_=classify(forms,child)
        require(c==independently and a==aa,"parent/child exact partition changed")
        for point in product(*child):
            require(c+sum(value(forms[i],point)<0 for i in a)==direct(forms,point)[0],"inherited count lost exactness")
        counts["child_partition_checks"]+=1


def main():
    counts=dict(comparator_checks=0,comparator_zero_equalities=0,local_root_checks=0,child_partition_checks=0,
                real_u16_comparator_cases=0,max_reduced_det_bits=0,max_naive_cross_bits=0,root_location_checks=0)
    killed={name:False for name in ("ignore_b_sign","ignore_denominator_signs","swap_seed_only","split_equal_roots")}
    rng=random.Random(20260921)
    triples=[]
    for _ in range(1200):
        a,b,x,z,w=[tuple(rng.randrange(M+1) for _ in range(3)) for _ in range(5)]
        if a==b:
            continue
        fx,fz,fw=geometry_forms(a,b,(x,z,w))
        if fx[1] or fx[2]:
            triples.append((fx,fz,fw,True))
    # Extremal coordinate cube: all ordered endpoint pairs and all six seeds.
    corners=list(product((0,M),repeat=3))
    for ia,a in enumerate(corners):
        for ib,b in enumerate(corners):
            if ia==ib:
                continue
            remaining=[p for ip,p in enumerate(corners) if ip not in (ia,ib)]
            for ix,x in enumerate(remaining):
                z,w=remaining[(ix+1)%6],remaining[(ix+2)%6]
                fx,fz,fw=geometry_forms(a,b,(x,z,w))
                if fx[1] or fx[2]:
                    triples.append((fx,fz,fw,True))
    tiny=((0,1,0),(0,1,1),(-1,-2,1))
    triples.extend([(tiny[0],tiny[1],tiny[2],False),((0,0,-1),(1,1,0),(-1,1,0),False),
                    ((0,0,1),(-1,1,0),(1,-1,0),False)])
    triples.append((tuple(v*M*M for v in (1,8,8)),tuple(v*M*M for v in (15,-8,8)),tuple(v*M*M for v in (-14,8,-8)),False))
    for _ in range(200):
        x,z,w=[tuple(rng.randrange(-100,101) for _ in range(3)) for _ in range(3)]
        if x[1] or x[2]:
            triples.append((x,z,w,False))
    for fx,fz,fw,real in triples:
        if real:
            require(all(abs(form[0])<=15*M*M and max(abs(form[1]),abs(form[2]))<=8*M*M
                        for form in (fx,fz,fw)),"real coefficient bounds")
        x,z,w=oriented(fx,fz,fw)
        pz,sz=restricted(x,z);pw,sw=restricted(x,w)
        if not sz or not sw:
            continue
        expected=sign(F(-pz,sz)-F(-pw,sw))
        require(compare(fx,fz,fw)==expected,"reduced root comparator failed")
        require(x[2]*det((x,z,w))==pz*sw-sz*pw,"determinant cancellation identity")
        delta=det((x,z,w))
        require(abs(delta)<=5760*M**6<I128,"reduced determinant i128 bound")
        counts["max_reduced_det_bits"]=max(counts["max_reduced_det_bits"],abs(delta).bit_length())
        counts["max_naive_cross_bits"]=max(counts["max_naive_cross_bits"],abs(pz*sw).bit_length(),abs(pw*sz).bit_length())
        counts["comparator_checks"]+=1;counts["real_u16_comparator_cases"]+=real
        counts["comparator_zero_equalities"]+=expected==0
        for name in killed:
            try:
                killed[name]|=compare(fx,fz,fw,name)!=expected
            except RuntimeError:
                # Swapping only the seed can even turn a real event into a fake parallel form.
                require(name=="swap_seed_only","unexpected mutant failure")
                killed[name]=True
        point,(nx,ny,denominator)=root_point(fx,fz)
        require(value(fx,point)==value(fz,point)==0,"root coordinate determinant")
        for q in (1,1024):
            for boundary in (-2*q,0,2*q):
                require(sign(q*nx-boundary*denominator)==sign(point[0]-F(boundary,q)),"exact root cell location")
                require(abs(q*nx)+abs(boundary*denominator)<I128,"cell location i128 bound")
                counts["root_location_checks"]+=1
    require(all(killed.values()),"root-comparator mutants survived: "+str([name for name in killed if not killed[name]]))
    require(counts["max_naive_cross_bits"]>127,"wide naive product fixture not exercised")

    # Real positive owned regular tetra: the event is on the lower-left cell corner.
    a,b,x,y=(10,10,10),(12,12,10),(10,12,8),(12,10,8)
    forms=geometry_forms(a,b,(a,b,x,y))
    require(forms[2:]==[(16,-16,16),(16,16,16)],"real min-zero fixture changed")
    cell=((F(0),F(1,4)),(F(-1),F(-3,4)))
    check_sweep(forms,forms[2],cell,counts)
    base,active,_=classify(forms,cell,mutant=True)
    require(not sweep(forms,forms[2],cell,base,active),"min>=0 mutant did not lose the true q4 root")
    require(direct(forms,(F(0),F(-1)))==(0,(0,1,2,3)),"positive fixture strict shell")

    # Mixed entry/exit at one root, plus identically-zero restricted forms.
    mixed=[(0,0,0),(0,0,1),(0,0,2),(-1,1,0),(1,-1,0),(-1,0,0)]
    check_sweep(mixed,mixed[1],((F(0),F(2)),(F(-1),F(1))),counts)
    # Saturation before a decreasing sweep changes a depth 2 into 1 at the first root.
    decreasing=[(-1,1,0),(-2,1,0),(-3,1,0),(0,0,1)]
    cell=((F(0),F(4)),(F(-1),F(1)))
    base,active,_=classify(decreasing,cell)
    right=sweep(decreasing,decreasing[-1],cell,base,active)
    wrong=sweep(decreasing,decreasing[-1],cell,base,active,threshold=2)
    require(right[0][1]==2 and wrong[0][1]==1,"saturating declining-count mutant survived")
    check_sweep(decreasing,decreasing[-1],cell,counts)
    # A root outside C must still affect the initialization if swept from -infinity.
    outside=[(0,0,1),(2,1,3),(-1,1,0)]
    check_sweep(outside,outside[0],((F(0),F(2)),(F(-4),F(4))),counts)

    # Closed cells overlap at their boundary; one exact owner must emit the event.
    border=[(0,0,1),(0,1,0),(0,0,0)]
    outputs=[]
    cells=(((F(-1),F(0)),(F(-1),F(1))),((F(0),F(1)),(F(-1),F(1))))
    for idx,cell in enumerate(cells):
        base,active,_=classify(border,cell)
        outputs.extend((idx,row) for row in sweep(border,border[0],cell,base,active))
    owned=[row for idx,row in outputs if idx==(1 if row[0][0]>=0 else 0)]
    require(len(outputs)==2 and len(owned)==1,"boundary ownership mutant survived")

    # A minimum of child counts is a lower bound, not an exact parent depth.
    forms=[(-1,1,0)]
    left=direct(forms,(F(0),F(0)))[0];right=direct(forms,(F(2),F(0)))[0]
    require(min(left,right)==0 and left==1,"child-minimum exactness counterexample")
    for _ in range(220):
        forms=[tuple(rng.randrange(-8,9) for _ in range(3)) for _ in range(10)]
        seed=tuple(rng.randrange(-4,5) for _ in range(3))
        if not (seed[1] or seed[2]):
            continue
        forms.append(seed)
        check_sweep(forms,seed,((F(-2),F(2)),(F(-2),F(2))),counts)
    print(json.dumps({"schema":"mhgp8_audit_q4_local_sweep_math_v1","status":"PASS",
                      "scope":"independent affine/integer model; real min-zero tetra fixture; no product qualification",
                      **counts,"comparator_mutants":sorted(killed),"other_mutants":["drop_min_zero","no_boundary_owner","saturate_declining_count"],
                      "child_minimum_not_exact":True},sort_keys=True))


if __name__=="__main__":
    main()
