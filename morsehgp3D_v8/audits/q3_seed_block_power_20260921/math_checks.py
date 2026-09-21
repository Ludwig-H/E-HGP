#!/usr/bin/env python3
"""Exact arithmetic gate for a proposed q3 seed-block centre envelope.

No product code, performance benchmark or implementation qualification.
Cartesian centres are independently solved from three linear equations.
The tested boxes enclose conditional centres of acute triangles owned by ab;
they are not claimed to enclose circumcentres of every point in the seed box.
"""
from fractions import Fraction as F
from itertools import product
from pathlib import Path
import hashlib
import json
import random

COMPARE_WORK={"calls":0,"divisions":0}


def require(test, message):
    if not test:
        raise RuntimeError(message)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def norm(a):
    return dot(a, a)


def cartesian(a, b, x):
    normal = cross(sub(b, a), sub(x, a))
    if normal == (0, 0, 0):
        return None
    rows = [[F(2*t) for t in sub(b, a)]+[F(norm(b)-norm(a))],
            [F(2*t) for t in sub(x, a)]+[F(norm(x)-norm(a))],
            [F(t) for t in normal]+[F(dot(normal, a))]]
    for col in range(3):
        pivot = next(i for i in range(col, 3) if rows[i][col])
        rows[col], rows[pivot] = rows[pivot], rows[col]
        denominator = rows[col][col]
        rows[col] = [value/denominator for value in rows[col]]
        for i in range(3):
            if i != col:
                coefficient = rows[i][col]
                rows[i] = [u-coefficient*v for u, v in zip(rows[i], rows[col])]
    center = tuple(row[-1] for row in rows)
    return center, norm(sub(center, a))


def valid(a, b, x):
    acute = all(dot(sub(u, v), sub(w, v)) > 0 for u, v, w in ((a,x,b),(b,a,x),(a,b,x)))
    return acute and norm(sub(x,a)) <= norm(sub(b,a)) and norm(sub(x,b)) <= norm(sub(b,a))


def forms(a, b, x, z):
    d = sub(b, a)
    w = tuple(2*x[i]-a[i]-b[i] for i in range(3))
    v = tuple(2*z[i]-a[i]-b[i] for i in range(3))
    D, t = norm(d), dot(d, w)
    J = D*norm(w)-t*t
    P = tuple(D*w[i]-t*d[i] for i in range(3))
    qx, qz = norm(w)-D, norm(v)-D
    return D, t, J, P, qx, J*qz-qx*dot(P,v)


def interval_sum(parts):
    parts=tuple(parts)
    return tuple(sum(part[i] for part in parts) for i in (0, 1))


def multiply(a, b):
    values = [x*y for x in a for y in b]
    return min(values), max(values)


def square(interval):
    low, high = interval
    return (0 if low <= 0 <= high else min(low*low, high*high), max(low*low, high*high))


def linear(coefficients, intervals):
    return interval_sum(multiply((c,c), span) for c, span in zip(coefficients, intervals))


def box(points):
    points=tuple(points)
    return tuple((min(p[i] for p in points), max(p[i] for p in points)) for i in range(3))


def quotient_less(a,b):
    """Compare nonnegative fractions without cross-products or wider integers."""
    require(a>=0 and b>=0,"nonnegative ratio comparison")
    COMPARE_WORK["calls"]+=1
    an,ad,bn,bd=a.numerator,a.denominator,b.numerator,b.denominator
    reverse=False
    while True:
        aq,ar=divmod(an,ad); bq,br=divmod(bn,bd)
        COMPARE_WORK["divisions"]+=2
        if aq!=bq:
            result=(aq>bq) if reverse else (aq<bq)
            break
        if ar==0 or br==0:
            result=False if ar==br==0 else ((br==0) if reverse else (ar==0))
            break
        an,ad,bn,bd=ad,ar,bd,br
        reverse=not reverse
    require(result==(a<b),"quotient comparator versus Fraction oracle")
    return result


def seed_bounds(a, b, X):
    d, D = sub(b,a), norm(sub(b,a))
    require(D > 0, "distinct edge endpoints")
    w = tuple((2*lo-a[i]-b[i], 2*hi-a[i]-b[i]) for i,(lo,hi) in enumerate(X))
    ws = interval_sum(square(span) for span in w)
    qx = ws[0]-D, ws[1]-D
    t = linear(d,w)
    P = tuple(linear(tuple((D if i==j else 0)-d[i]*d[j] for j in range(3)),w) for i in range(3))
    components = [linear((0,-d[2],d[1]),w), linear((d[2],0,-d[0]),w), linear((-d[1],d[0],0),w)]
    J = interval_sum(square(span) for span in components)
    return D, qx, t, P, J


def centers(a, b, X, refined):
    D, qx, t, P, J = seed_bounds(a,b,X)
    # Necessary conditions for integer, acute, maximum-edge seeds only.
    qlo, qhi = max(1,qx[0]), min(2*D,qx[1])
    tlow, thigh = max(-D+1,t[0]), min(D-1,t[1])
    if qlo > qhi or tlow > thigh:
        return None
    low, high = F(0), F(2,3)
    used = refined and J[0] > 0
    if used:
        ts = square((tlow,thigh))
        rlow, rhigh = D*D-ts[1], D*D-ts[0]
        require(rlow > 0, "strict acuteness interval")
        # Include the naive qx/J interval, then preserve J=D*qx+R.
        for candidate in (F(D*qlo,J[1]),F(D*qlo,D*qlo+rhigh)):
            if quotient_less(low,candidate): low=candidate
        for candidate in (F(D*qhi,J[0]),F(D*qhi,D*qhi+rlow)):
            if quotient_less(candidate,high): high=candidate
    if quotient_less(high,low):
        return None
    bounds = []
    for span in P:
        lower=3*(high if span[0]<0 else low)*span[0]
        upper=3*(high if span[1]>0 else low)*span[1]
        bounds.append((lower.numerator//lower.denominator, -(-upper.numerator//upper.denominator)))
    return tuple(bounds), used, (low,high)


def power_bounds(a, b, C, Z):
    # C encloses B=12D(c-m). The exact separable bound over C x integer Z
    # is min/max of two endpoint parabolas per coordinate. No B^2 formed.
    D = norm(sub(b,a))
    lows, highs = [], []
    for axis, ((zlo,zhi),(blo,bhi)) in enumerate(zip(Z,C)):
        s = a[axis]+b[axis]
        minima, maxima = [], []
        for beta in (blo,bhi):
            vertex = F(s,2)+F(beta,12*D)
            floor = vertex.numerator//vertex.denominator
            candidates = (max(zlo,min(zhi,floor)),max(zlo,min(zhi,floor+1)))
            def value(z):
                v = 2*z-s
                return 3*D*v*v-beta*v
            minima.append(min(map(value,candidates)))
            maxima.append(max(value(zlo),value(zhi)))
        lows.append(min(minima)); highs.append(max(maxima))
    return sum(lows)-3*D*D, sum(highs)-3*D*D


def cubic_bounds(a, b, X, Z):
    D, qx, _, P, J = seed_bounds(a,b,X)
    v = tuple((2*lo-a[i]-b[i],2*hi-a[i]-b[i]) for i,(lo,hi) in enumerate(Z))
    vv = interval_sum(square(span) for span in v)
    qz = vv[0]-D, vv[1]-D
    first = multiply(J,qz)
    second = multiply(qx,interval_sum(multiply(p,vv) for p,vv in zip(P,v)))
    return first[0]-second[1], first[1]-second[0]


def lattice(X):
    return list(product(*(range(lo,hi+1) for lo,hi in X)))


def fixtures():
    a, b = (20,20,20),(40,20,20)
    seeds = [(30,y,20) for y in range(32,38)]
    zs = [box([p]) for p in (a,b,(30,31,20),seeds[0],seeds[-1],(30,40,20))]
    zs.append(((29,31),(30,31),(19,21)))
    result = [("axis",a,b,seeds,zs),
              ("fallback",a,b,[(30,y,20) for y in range(20,38)],zs),
              ("volume",a,b,lattice(((27,32),(31,34),(19,22))),zs)]
    def rotate(p):
        return tuple(500+dot(row,p) for row in ((2,-2,1),(1,2,2),(-2,-1,2)))
    result.append(("rotated",rotate(a),rotate(b),list(map(rotate,seeds)),
                   [box(map(rotate,lattice(Z))) for Z in zs]))
    result.append(("equilateral",(30,30,30),(36,36,30),[(36,30,36)],
                   [box([p]) for p in ((30,30,30),(36,36,30),(36,30,36),(32,34,28),(34,32,32))]))
    M=65535
    result.append(("u16",(0,0,0),(M,M,0),[(M,0,M)],
                   [box([p]) for p in ((0,0,0),(M,M,0),(M,0,M),(M,M,M),(43690,21845,21845))]))
    rng=random.Random(330621)
    for seed in range(8):
        cloud=[]
        while len(cloud)<12:
            p=tuple(rng.randrange(5,46) for _ in range(3))
            if p not in cloud: cloud.append(p)
        result.append(("random_"+str(seed),cloud[0],cloud[1],cloud[2:],
                       [box([p]) for p in cloud[:4]]))
    return result


def main():
    counts={"cases":0,"noncollinear":0,"valid_seeds":0,"formula_checks":0,
            "centre_checks":0,"power_checks":0,"refined_boxes":0,"fallback_boxes":0,
            "strict_admissions":0,"nonnegative_exclusions":0,"contact_checks":0}
    for name,a,b,seeds,Zs in fixtures():
        X=box(seeds); counts["cases"]+=1
        for mode in (False,True):
            prepared=centers(a,b,X,mode)
            if prepared:
                counts["refined_boxes" if prepared[1] else "fallback_boxes"]+=1
            for x in seeds:
                reference=cartesian(a,b,x)
                if reference is None: continue
                counts["noncollinear"]+=1
                center,radius=reference
                D,t,J,P,qx,_=forms(a,b,x,a)
                require(J>0,"noncollinear J")
                proposed=tuple(F(a[i]+b[i],2)+F(qx*P[i],4*J) for i in range(3))
                require(proposed==center,"Cartesian centre disagreement")
                eligible=valid(a,b,x)
                require(eligible==(qx>0 and abs(t)<D and qx+2*abs(t)<=2*D),"acuteness/ownership equivalence")
                if eligible:
                    counts["valid_seeds"]+=1
                    require(prepared is not None,"valid seed lost by preparation")
                    C,_,limits=prepared
                    lam=F(D*qx,J)
                    require(0<lam<=F(2,3) and limits[0]<=lam<=limits[1],"lambda interval")
                    for i in range(3):
                        require(C[i][0]<=12*D*(center[i]-F(a[i]+b[i],2))<=C[i][1],"centre envelope")
                    counts["centre_checks"]+=1
                for Z in Zs:
                    cubic=cubic_bounds(a,b,X,Z)
                    bounded=power_bounds(a,b,prepared[0],Z) if eligible else None
                    for z in lattice(Z):
                        require(all(0<=v<=65535 for p in (a,b,x,z) for v in p),"u16 fixture")
                        power=norm(sub(z,center))-radius
                        numerator=forms(a,b,x,z)[-1]
                        require(numerator==4*J*power,"power numerator identity")
                        require(cubic[0]<=numerator<=cubic[1],"cubic enclosure")
                        counts["formula_checks"]+=1
                        if power==0:
                            require(not numerator<0,"contact counted strict")
                            counts["contact_checks"]+=1
                        if bounded is not None:
                            require(bounded[0]<=12*D*power<=bounded[1],"shared power enclosure")
                            counts["power_checks"]+=1
                    if bounded is not None:
                        counts["strict_admissions"]+=bounded[1]<0
                        counts["nonnegative_exclusions"]+=bounded[0]>=0
    a,b=(20,20,20),(40,20,20)
    seeds=[(30,y,20) for y in range(32,38)]; z=(30,31,20); X=box(seeds); Z=box([z])
    naive=cubic_bounds(a,b,X,Z); universal=power_bounds(a,b,centers(a,b,X,False)[0],Z)
    refined=power_bounds(a,b,centers(a,b,X,True)[0],Z)
    require(naive[1]==1670400 and naive[1]>=0 and universal[1]>=0 and refined[1]==-92800,"strict improvement fixture")
    eq=forms((30,30,30),(36,36,30),(36,30,36),(30,30,30))
    require(F(eq[0]*eq[4],eq[2])==F(2,3),"equilateral equality")
    # Removing acuteness or ownership makes the universal lambda envelope false.
    bad_acute=forms((0,0,0),(10,0,0),(5,2,0),(0,0,0))
    bad_owner=forms((0,0,0),(4,0,0),(2,10,0),(0,0,0))
    require(F(bad_acute[0]*bad_acute[4],bad_acute[2])<0,"missing acute counterexample")
    require(F(bad_owner[0]*bad_owner[4],bad_owner[2])>F(2,3),"missing owner counterexample")
    # Prefix credit is tied to its cursor. A fresh census cannot receive it.
    population=[z,a,b,seeds[0],seeds[-1]]
    center,radius=cartesian(a,b,seeds[0])
    powers=[norm(sub(p,center))-radius for p in population]
    prefix=sum(v<0 for v in powers[:1]); tail=sum(v<0 for v in powers[1:]); fresh=sum(v<0 for v in powers)
    require(prefix==1 and tail==0 and fresh==1 and prefix+tail==fresh,"prefix census fixture")
    require(fresh<2 and prefix+fresh>=2 and sum(v==0 for v in powers)==3,"double-credit/contact counterexample")
    for key in ("valid_seeds","refined_boxes","fallback_boxes","strict_admissions","nonnegative_exclusions","contact_checks"):
        require(counts[key]>0,"vacuous "+key)
    M=65535
    require(1296*M**6<2**107 and 648*M**7<2**122,"i128 preparation bound")
    ratios=[F(0),F(2,3),F((1<<68)-1,(1<<69)-3),F((1<<68)-2,(1<<69)-5)]
    for left,right in product(ratios,repeat=2):
        quotient_less(left,right)
    print(json.dumps({"status":"PASS","source_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                      "counts":counts,"improvement":{"direct_F":naive,"universal_12Dpower":universal,"refined_12Dpower":refined},
                      "ratio_comparison_work":COMPARE_WORK,
                      "prefix":{"fresh":fresh,"resumed":prefix+tail,"wrong_restarted":prefix+fresh,"shell":3},
                      "limits":"Mathematical proposal only; no product, LiDAR cost or parallel qualification."},sort_keys=True))


if __name__=="__main__":
    main()
