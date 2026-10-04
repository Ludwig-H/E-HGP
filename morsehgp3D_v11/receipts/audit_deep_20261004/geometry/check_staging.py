#!/usr/bin/env python3
"""Source-bound exact scalar review. No product imports, native, fits or large input."""
from fractions import Fraction as F
from itertools import combinations, permutations, product
from pathlib import Path
import hashlib
import json

BASE = Path(__file__).resolve().parent
CHECKS = 0


def need(value, why):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(why)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def norm(a):
    return dot(a, a)


def q3(points):
    a, b, c = points
    u, v = sub(b, a), sub(c, a)
    w = cross(u, v)
    g = norm(w)
    if not g:
        return None
    t = tuple(norm(u)*v[j]-norm(v)*u[j] for j in range(3))
    n, d = cross(t, w), 2*g
    center = tuple(F(a[j])+F(n[j], d) for j in range(3))
    # Independent Gram solution for the same center.
    uu, uv, vv = norm(u), dot(u,v), norm(v)
    det = uu*vv-uv*uv
    weights = (F(vv*(uu-uv), 2*det), F(uu*(vv-uv), 2*det))
    gram = tuple(F(a[j])+weights[0]*u[j]+weights[1]*v[j] for j in range(3))
    need(gram == center, 'Cramer and Gram centers disagree')
    need(F(norm(u)*norm(v)*norm(sub(c,b)), 4*g) == norm(sub(center,a)), 'reduced level exact')
    return center, n, d


def corners(box):
    return product(*[(lo,hi) for lo,hi in box])


def dominates(witness, site, box):
    # Independent corner characterization, not the production preprocessed Terms expression.
    return all(norm(sub(witness,c)) < norm(sub(site,c)) for c in corners(box))


def line_meets(points, box, center):
    normal = cross(sub(points[1],points[0]), sub(points[2],points[0]))
    lower, upper = None, None
    for j, (lo,hi) in enumerate(box):
        if not normal[j]:
            if not lo <= center[j] <= hi:
                return False
            continue
        ends = sorted((F(lo-center[j], normal[j]), F(hi-center[j], normal[j])))
        lower = ends[0] if lower is None else max(lower,ends[0])
        upper = ends[1] if upper is None else min(upper,ends[1])
    return lower is None or lower <= upper


def morton(point):
    return sum(((point[j]>>b)&1)<<(3*b+j) for b in range(5) for j in range(3))


def leaves(points, kmax, leaf_size, scale=1):
    scaled = [tuple(scale*x for x in p) for p in points]
    root = tuple((min(p[j] for p in scaled), max(p[j] for p in scaled)+1) for j in range(3))
    result, node_count = [], 0

    def visit(parent, box, depth):
        nonlocal node_count
        node_count += 1
        midpoint_twice = tuple(lo+hi for lo,hi in box)
        witnesses = sorted(parent, key=lambda i:(norm(tuple(2*scaled[i][j]-midpoint_twice[j] for j in range(3))),i))[:3*kmax]
        kept = [i for i in parent if sum(dominates(scaled[w],scaled[i],box) for w in witnesses) < kmax]
        if not kept:
            return
        adjusted = tuple((max(box[j][0],min(scaled[i][j] for i in kept)),min(box[j][1],max(scaled[i][j] for i in kept)+1)) for j in range(3))
        if any(lo>=hi for lo,hi in adjusted):
            return
        axis = max(range(3), key=lambda j:adjusted[j][1]-adjusted[j][0])
        lo, hi = adjusted[axis]
        if len(kept) > leaf_size and hi-lo > 1:
            middle = lo+(hi-lo)//2
            left, right = list(adjusted), list(adjusted)
            left[axis],right[axis] = (lo,middle),(middle,hi)
            visit(kept,tuple(left),depth+1)
            visit(kept,tuple(right),depth+1)
        else:
            result.append((kept,tuple((F(lo,scale),F(hi,scale)) for lo,hi in adjusted),depth))
    visit(list(range(len(points))),root,0)
    return result,node_count


def eligible_triple(points, sites, box, kmax, triple):
    p = tuple(points[i] for i in triple)
    ball = q3(p)
    if ball is None:
        return None
    center,n,d = ball
    if not all(dot(sub(p[(i+1)%3],p[i]),sub(p[(i+2)%3],p[i]))>0 for i in range(3)):
        return None
    dom = {j for i in triple for j in sites if dominates(points[j],points[i],box)}
    if len(dom)>kmax-2 or not line_meets(p,box,center):
        return None
    # Every shorter prefix then passes its weaker G3 threshold; a center-line
    # witness also witnesses every pair bisector. This includes live-row pruning.
    for size in (1,2,3):
        prefix_dom={j for i in triple[:size] for j in sites if dominates(points[j],points[i],box)}
        need(len(prefix_dom)<=kmax+1-size,'earlier prefix is not cut by G3')
    for i,j in combinations(triple,2):
        need(not dominates(points[i],points[j],box) and not dominates(points[j],points[i],box),
             'pair graph must retain this triple')
    return center,n,d


def find_owner_reject():
    # Fixed seed/count box enumeration; eight sites, max 160 finite attempts.
    state = 317
    for attempt in range(160):
        chosen = []
        while len(chosen)<8:
            coords = []
            for unused in range(3):
                state = (1664525*state+1013904223)&0xffffffff
                coords.append((state>>12)%7)
            p=tuple(coords)
            if p not in chosen:
                chosen.append(p)
        points = sorted(chosen,key=morton)
        leaf_nodes,nodes = leaves(points,3,6)
        for sites,box,depth in leaf_nodes:
            for triple in combinations(sites,3):
                answer=eligible_triple(points,sites,box,3,triple)
                if answer is None:
                    continue
                center,n,d=answer
                if any(not lo<=center[j]<hi for j,(lo,hi) in enumerate(box)):
                    need(depth>0,'owner fixture actually subdivided')
                    need(line_meets(tuple(points[i] for i in triple),box,center),'closed center line kept')
                    p=tuple(points[i] for i in triple)
                    raw_num=norm(sub(p[1],p[0]))*norm(sub(p[2],p[0]))*norm(sub(p[2],p[1]))
                    raw_den=4*norm(cross(sub(p[1],p[0]),sub(p[2],p[0])))
                    need(raw_num==3000 and raw_den==464,'fixed reachable witness raw Level encoding')
                    witness=(F(5,2),F(11,4),F(2))
                    need(all(lo<=witness[j]<=hi for j,(lo,hi) in enumerate(box)),
                         'center line witness is in the closed box, including z=lo')
                    need(all(norm(sub(witness,p[i]))==norm(sub(witness,p[0])) for i in (1,2)),
                         'center line witness equidistant from the triple')
                    return {'attempt':attempt,'points':points,'sites':sites,'triple':triple,'box':box,'center':center,'N':n,'D':d,'nodes':nodes,'depth':depth,'kmax':3,'leaf_size':6,
                            'closed_center_line_witness':witness,'level_raw':[raw_num,raw_den],
                            'level_value':F(raw_num,raw_den),
                            'scope':'bounded exact replay of sequential DFS preparation and pre-owner predicates, not product execution'}
    raise RuntimeError('bounded owner witness search exhausted')


def main():
    before=json.loads((BASE/'SOURCE_BEFORE.json').read_text())
    v10=json.loads((BASE/'SOURCE_V10_BEFORE.json').read_text())
    for manifest in (before,v10):
        for path,info in manifest['files'].items():
            need(hashlib.sha256((BASE/info['copy']).read_bytes()).hexdigest()==info['sha256'],'snapshot changed '+path)
    src=BASE/'source/morsehgp3D_v11'
    sphere=(src/'src/num/sphere.cpp').read_text()
    leaf=(src/'src/catalogue/leaf.cpp').read_text()
    old=(BASE/'v10_source/morsehgp3D_v10/src/catalogue/generator.cpp').read_text()
    need('const auto numerator = multiply(to_wide(i128{uu} * vv), to_wide(detail::dot(bc, bc)));' in sphere,'eager q3 numerator changed')
    need('if (sphere.value() && center_in_box(*sphere.value(), leaf.box))' in leaf,'owner gate changed')
    need(old.index('const bool admit =') < old.index('r.level = emitted_level'),'v10 level really after admission')
    L=2**18-1
    need(6*L**4<2**75 and 12*L**5<2**100,'global certificate for shared u18 cube even under profile24')
    # Max projected triangle determinant is L^2 on corners of the common square.
    extrema=0
    for a,b,c in product(tuple(product((0,L),repeat=2)),repeat=3):
        determinant=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
        extrema=max(extrema,abs(determinant))
    need(extrema==L*L,'projected determinant corner bound')
    corners3=tuple(product((0,L),repeat=3))
    checked=0
    for triple in combinations(corners3,3):
        for ordered in permutations(triple):
            ball=q3(ordered)
            if ball is None:
                continue
            center,n,d=ball
            for bits in (21,24):
                need(d<2**(123-2*bits) and all(abs(x)<2**(124-bits) for x in n),'global q3 power i128 certificate')
            checked+=1
    witness=find_owner_reject()
    # Source lower/upper are sound but need not be extrema of the coupled power polynomial.
    D,N=2,10
    lo,hi=1,9
    separate_upper=D*max(lo*lo,hi*hi)-2*N*lo
    endpoint_upper=max(D*lo*lo-2*N*lo,D*hi*hi-2*N*hi)
    need(separate_upper==142 and endpoint_upper==-18,'strict-interior box missed by current separable upper bound')
    need(all(D*x*x-2*N*x<0 for x in range(lo,hi+1)),'actual interval strictly interior')
    rows=[]
    for bits in (18,21,24):
        num_bits,den_bits=8*bits+12,6*bits+8
        denominator_words=2 if den_bits<=127 else (den_bits+63)//64
        rows.append({'bits':bits,'level_budget':[num_bits,den_bits],'level_storage_kind':['Wide<%d>'%((num_bits+63)//64),'i128' if den_bits<=127 else 'Wide<%d>'%denominator_words],'exact_level_comparison_words':(num_bits+63)//64+denominator_words,'center_line_test_storage':'i64' if 3*bits+5<=63 else 'i128','q3_power_shared_u18_cube':'i128'})
    print(json.dumps({'status':'PASS','checks':CHECKS,'pin':before['pin'],'v10_pin':v10['pin'],'shared_cube_q3_permutations':checked,'profile_rows':rows,'owner_rejected_q3':witness,'power_bound_example':{'separate_upper':separate_upper,'coupled_endpoint_upper':endpoint_upper,'scope':'valid public Box and q2 sphere, not measured index traffic'},'native_runs':0,'fits':0,'gcp_actions':0,'scope':'exact scalar source-bound reachable DFS witness; no native execution or timing'},sort_keys=True,indent=1,default=str))


if __name__=='__main__':
    main()
