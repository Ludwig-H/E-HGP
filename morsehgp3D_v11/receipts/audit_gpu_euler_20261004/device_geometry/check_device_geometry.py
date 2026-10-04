#!/usr/bin/env python3
"""Independent bounded Fraction/Gram geometry; no compilation or device execution."""
from fractions import Fraction as F
from itertools import combinations, permutations, product
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parent
checks = 0
bounds = {'i64': 0, 'i128': 0}


def check(ok, why):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(why)


def fit(value, bits):
    check(-(1 << (bits - 1)) <= value < 1 << (bits - 1), 'signed fit %d' % bits)
    key = 'i64' if bits == 64 else 'i128'
    bounds[key] = max(bounds[key], abs(value).bit_length())
    return value


def diff(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def solve(matrix, rhs):
    m = [list(map(F, row)) + [F(y)] for row, y in zip(matrix, rhs)]
    for j in range(len(m)):
        pivot = next((i for i in range(j, len(m)) if m[i][j]), None)
        if pivot is None:
            return None
        m[j], m[pivot] = m[pivot], m[j]
        scale = m[j][j]
        m[j] = [x/scale for x in m[j]]
        for i in range(len(m)):
            if i != j:
                scale = m[i][j]
                m[i] = [x-scale*y for x, y in zip(m[i], m[j])]
    return tuple(row[-1] for row in m)


def gram(ps):
    a, *others = ps
    v = [diff(p, a) for p in others]
    weights = solve([[dot(x, y) for y in v] for x in v], [F(dot(x, x), 2) for x in v])
    if weights is None:
        return None
    center = tuple(F(a[j])+sum(w*x[j] for w, x in zip(weights, v)) for j in range(3))
    return center, dot(diff(center, a), diff(center, a)), (1-sum(weights), *weights)


def raw(ps):
    a = ps[0]
    if len(ps) == 2:
        n, d = diff(ps[1], a), 2
    elif len(ps) == 3:
        u, v = diff(ps[1], a), diff(ps[2], a)
        w = cross(u, v)
        g = sum(fit(x*x, 128) for x in w)
        fit(g, 128)
        if not g:
            return None
        uu, vv = fit(dot(u, u), 64), fit(dot(v, v), 64)
        t = tuple(fit(fit(uu*v[j], 128)-fit(vv*u[j], 128), 128) for j in range(3))
        n, d = cross(t, w), 2*g
    else:
        u, v, s = [diff(p, a) for p in ps[1:]]
        vs, su, uv = cross(v, s), cross(s, u), cross(u, v)
        det = fit(dot(u, vs), 128)
        if not det:
            return None
        norms = [fit(dot(x, x), 64) for x in (u, v, s)]
        n = []
        for j in range(3):
            value = 0
            for norm, vec in zip(norms, (vs, su, uv)):
                value = fit(value + fit(norm*vec[j], 128), 128)
            n.append(value)
        d = 2*det
        if d < 0:
            n, d = [-x for x in n], -d
        n = tuple(n)
    for x in n:
        fit(x, 128)
    fit(d, 128)
    return a, n, d


def coeff_center(s):
    a, n, d = s
    return tuple(F(a[j])+F(n[j], d) for j in range(3))


def cert(s, bits, orientation=False):
    _a, n, d = s
    dl = 1 << ((124-3*bits) if orientation else (123-2*bits))
    nl = 1 << ((124-2*bits) if orientation else (124-bits))
    return 0 < d < dl and all(-nl < x < nl for x in n)


def side(s, z, arity, bits):
    if arity == 3 and 6*bits+8 > 127 and not cert(s, bits):
        return None
    a, n, d = s
    v = diff(z, a)
    norm = fit(dot(v, v), 64)
    total = fit(d*norm, 128)
    for j in range(3):
        total = fit(total+fit(n[j]*(-2*v[j]), 128), 128)
    return (total > 0)-(total < 0)


def weights_native(ps):
    if any(max(p[j] for p in ps)-min(p[j] for p in ps) > 1 << 20 for j in range(3)):
        return None
    a = ps[0]
    u, v, s = [diff(p, a) for p in ps[1:]]
    vs, su, uv = cross(v, s), cross(s, u), cross(u, v)
    det = fit(dot(u, vs), 128)
    if not det:
        return False
    n = [sum(dot(w, w)*q[j] for w, q in zip((u, v, s), (vs, su, uv))) for j in range(3)]
    h = fit(2*fit(det*det, 128), 128)
    def scalar(vec):
        total = 0
        for x, y in zip(n, vec):
            total = fit(total+fit(x*y, 128), 128)
        return total
    face = tuple(fit(vs[j]+su[j]+uv[j], 64) for j in range(3))
    w0 = fit(h-scalar(face), 128)
    if w0 <= 0:
        return False
    w1 = scalar(vs)
    if w1 <= 0:
        return False
    w2 = scalar(su)
    return w2 > 0 and fit(fit(fit(h-w0, 128)-w1, 128)-w2, 128) > 0


def sat(ps, lo, hi):
    a, b, c = ps
    fu, gu = diff(a, b), diff(a, c)
    fc, gc = fit(dot(a, a)-dot(b, b), 64), fit(dot(a, a)-dot(c, c), 64)
    cr = [[0]*3 for _ in range(3)]
    rank_two = False
    for i in range(3):
        for j in range(i+1, 3):
            value = fit(gu[i]*fu[j]-fu[i]*gu[j], 64)
            cr[i][j] = cr[j][i] = abs(value)
            rank_two |= bool(value)
    if not rank_two:
        return 0
    p0, p1 = fc, gc
    for k in range(3):
        p0 = fit(p0-fit((lo[k]+hi[k])*fu[k], 64), 64)
        p1 = fit(p1-fit((lo[k]+hi[k])*gu[k], 64), 64)
    for k in range(3):
        if not fu[k] and not gu[k]:
            continue
        left = abs(fit(fit(gu[k]*p0, 128)-fit(fu[k]*p1, 128), 128))
        right = sum(fit((hi[j]-lo[j])*cr[k][j], 128) for j in range(3))
        fit(right, 128)
        if left > right:
            return 1
    return 2


def line_oracle(ps, lo, hi):
    ball = gram(ps)
    if ball is None:
        return 0
    center = ball[0]
    normal = cross(diff(ps[1], ps[0]), diff(ps[2], ps[0]))
    low, high = None, None
    for j in range(3):
        if not normal[j]:
            if not lo[j] <= center[j] <= hi[j]:
                return 1
            continue
        ends = sorted((F(lo[j]-center[j], normal[j]), F(hi[j]-center[j], normal[j])))
        low = ends[0] if low is None else max(low, ends[0])
        high = ends[1] if high is None else min(high, ends[1])
    return 2 if low is None or low <= high else 1


before = json.loads((ROOT/'BEFORE.json').read_text())
for path, binding in before['sources'].items():
    check(hashlib.sha256((ROOT/'source'/path).read_bytes()).hexdigest() == binding['sha256'], 'frozen source hash')
leaf = (ROOT/'source/src/catalogue/leaf_device.hpp').read_text()
pred = (ROOT/'source/src/catalogue/leaf_device_predicates.hpp').read_text()
for exact in ('s.d = 2 * g; s.arity = 3;', 's.d = d; s.arity = 4;',
              'if (unresolved) return;', 'return leaf.unresolved ? kUnresolved : kOk;',
              'if (!center_orientation(a, b, cc, s, plane)) { unresolved = true; return false; }',
              'if (high - low > (u32(1) << 20)) { unresolved = true; return; }',
              'if (support[i] != generated[i]) return;',
              'else if (!side(s, P[i], relation)) {\n        unresolved = true;\n        return;'):
    check(exact in leaf, 'source guard ' + exact)
check('if (!native_power_ok(c)) return false;' in pred, 'side guard before multiplication')
check('if (lower < 0 || upper >= 0) return false;' in pred, 'half-open owner')
check('if (left > right) return kDisjoint;' in pred, 'SAT closure contact')
check('if (hit) ++c.region_line_cache_hits; else ++c.region_line_evaluations;' in leaf and
      'if (center_line_meets(P[a], P[b], P[last], in.lo, in.hi) != kIntersects)' in leaf,
      'seen hit recomputes physical predicate')

fixtures = [((0,0,0),(4,0,0)), ((0,0,0),(2,2,0),(2,0,2)),
            ((0,0,0),(4,0,0),(0,4,0)), ((0,0,0),(6,0,0),(1,1,0)),
            ((0,0,0),(2,2,0),(2,0,2),(0,2,2)),
            ((1,2,6),(8,4,8),(2,1,3),(7,8,5))]
balls, powers, native_weights, unresolved_weights, sat_cases, orientations = 0,0,0,0,0,0
for bits in (18,21,24):
    maximum = (1 << bits)-1
    for base in fixtures:
        for scale in (1, max(1, maximum//16)):
            for ordered in permutations(base):
                ps = tuple(tuple(x*scale for x in p) for p in ordered)
                s, oracle = raw(ps), gram(ps)
                check((s is None) == (oracle is None), 'Gram degeneracy')
                if s is None:
                    continue
                center, radius, bary = oracle
                check(coeff_center(s) == center, 'raw center equals independent Gram')
                check(all(dot(diff(p,center),diff(p,center)) == radius for p in ps), 'all support contacts')
                balls += 1
                queries = tuple(dict.fromkeys(ps+((0,0,0),(maximum,maximum,maximum),(maximum,0,maximum))))
                for z in queries:
                    got = side(s,z,len(ps),bits)
                    if got is not None:
                        val = dot(diff(z,center),diff(z,center))-radius
                        check(got == (val>0)-(val<0), 'native side matches Fraction distance')
                        powers += 1
                if len(ps)==4:
                    got = weights_native(ps)
                    if got is None:
                        unresolved_weights += 1
                    else:
                        check(got == all(w>0 for w in bary), 'weights raw det agrees Gram positive hull')
                        native_weights += 1
                if cert(s,bits,orientation=True):
                    a,n,d=s
                    for tri in combinations(queries,3):
                        normal=cross(diff(tri[1],tri[0]),diff(tri[2],tri[0]))
                        total=0
                        for j in range(3):
                            coordinate=fit(n[j]+fit(d*(a[j]-tri[0][j]),128),128)
                            total=fit(total+fit(coordinate*normal[j],128),128)
                        exact=dot(diff(center,tri[0]),normal)
                        check((total>0)-(total<0)==(exact>0)-(exact<0),'center orientation independent affine plane')
                        orientations += 1
                if len(ps)==3:
                    for lo,hi in (((0,0,0),(1<<bits,)*3),((0,0,0),(scale,)*3),
                                  ((scale,)*3,(2*scale,)*3)):
                        check(sat(ps,lo,hi)==line_oracle(ps,lo,hi),'SAT equals rational line-box clipping')
                        sat_cases += 1

# Sharp closure/owner contact and the q3 native refusal before an unsafe product.
contact_ps=((0,0,0),(2,0,0),(0,2,0))
check(sat(contact_ps,(1,1,0),(2,2,1))==line_oracle(contact_ps,(1,1,0),(2,2,1))==2,'lower corner SAT contact retained')
check(sat(contact_ps,(0,0,0),(1,1,1))==line_oracle(contact_ps,(0,0,0),(1,1,1))==2,'upper corner SAT contact retained')
center=gram(contact_ps)[0]
check(all(F(1 if j<2 else 0)<=center[j]<F(2 if j<2 else 1) for j in range(3)), 'owner lower included')
check(not all(0<=center[j]<1 for j in range(3)), 'owner upper excluded')

causal=[]
for bits in (18,21,24):
    l=(1<<bits)-1
    ps=((0,0,0),(l,l,0),(l,0,l)); z=(l,l,l)
    s=raw(ps); oracle=gram(ps)
    acute=all(dot(diff(ps[j],ps[i]),diff(ps[k],ps[i]))>0 for i,j,k in ((0,1,2),(1,0,2),(2,0,1)))
    check(acute and all(0<=v<(1<<bits) for v in oracle[0]),'q3 strict and owner root')
    check(sat(ps,(0,0,0),(1<<bits,)*3)==2,'q3 prefix J2 admitted')
    got=side(s,z,3,bits)
    check((got is None)==(bits!=18),'uncertified q3 refuses at u21/u24; u18 safe')
    val=dot(diff(z,oracle[0]),diff(z,oracle[0]))-oracle[1]
    check(val>0,'fourth site is exterior; not a contact or threshold-rejection')
    causal.append({'profile':bits,'q3_certificate':cert(s,bits),'side_result':got,
                   'uncertified_first_product_bits':(s[2]*dot(diff(z,s[0]),diff(z,s[0]))).bit_length()})

for bits in (21,24):
    for l in (1<<20,(1<<20)+1):
        ps=((0,0,0),(l,l,0),(l,0,l),(0,l,l))
        oracle=gram(ps)
        check(all(0<=x<(1<<bits) for p in ps for x in p),'cube guard fixture within profile')
        check(all(w>0 for w in oracle[2]),'q4 cube fixture strict')
        check((weights_native(ps) is None)==(l>(1<<20)), 'guard at equality and one past cube bound')

# Dense cache rank is injective; masks never shift beyond32 and counters fit u64.
ranks=[k*(k-1)*(k-2)//6+j*(j-1)//2+i for i,j,k in combinations(range(32),3)]
check(sorted(ranks)==list(range(4960)),'rank exact dense cache 32sites')
check(max(ranks)//32==154 and (4960+31)//32==155,'seen bit array bounds')
for bits in (18,21,24):
    m=1<<bits
    check(12*m*m < 1<<63,'dominance i64 all partial magnitudes')
    check(18*m**3 < 1<<127,'SAT i128 all partial magnitudes')
    check(24*m**5 < 1<<127 and 24*m**4 < 1<<127,'q3 center i128 all partial magnitudes')
    check(72*m**5 < 1<<127,'q4 side i128 all partial magnitudes')
    check(96*m**5 < 1<<127,'midpoint owner i128 all partial magnitudes')
check(117*(1<<20)**6 < 1<<127,'q4 weights cube all partial magnitudes')

# Canonical support on complete shells: independent Gram positive weights versus midpoint/acute+plane/orientation.
def canonical_gram(shell, center):
    for q in (2,3,4):
        for ids in combinations(range(len(shell)),q):
            ball=gram([shell[i] for i in ids])
            if ball is not None and ball[0]==center and all(w>0 for w in ball[2]):
                return ids
    return None


def canonical_device(shell, center):
    for ids in combinations(range(len(shell)),2):
        if all(2*center[j]==shell[ids[0]][j]+shell[ids[1]][j] for j in range(3)):
            return ids
    for ids in combinations(range(len(shell)),3):
        ps=[shell[i] for i in ids]
        if not all(dot(diff(ps[j],ps[i]),diff(ps[k],ps[i]))>0 for i,j,k in ((0,1,2),(1,0,2),(2,0,1))):
            continue
        if dot(diff(center,ps[0]),cross(diff(ps[1],ps[0]),diff(ps[2],ps[0])))==0:
            return ids
    for ids in combinations(range(len(shell)),4):
        ps=[shell[i] for i in ids]
        strict=True
        for opposite in range(4):
            face=[p for i,p in enumerate(ps) if i!=opposite]
            normal=cross(diff(face[1],face[0]),diff(face[2],face[0]))
            vertex=dot(diff(ps[opposite],face[0]),normal)
            value=dot(diff(center,face[0]),normal)
            if not vertex or (vertex>0)-(vertex<0)!=(value>0)-(value<0):
                strict=False
                break
        if strict:
            return ids
    return None


shells=[(tuple(sorted(product((0,2),repeat=3))), (F(1),)*3),
        (((10,5,0),(8,9,0),(2,9,0),(5,0,0)),(F(5),F(5),F(0))),
        (((0,0,0),(2,2,0),(2,0,2)),(F(4,3),F(2,3),F(2,3)))]
canonical_cases=0
for shell, center in shells:
    check(len({dot(diff(p,center),diff(p,center)) for p in shell})==1,'complete shell equal contact')
    for ordering in (shell,tuple(reversed(shell))):
        expected=canonical_gram(ordering,center)
        check(expected==canonical_device(ordering,center),'lexicographic minimal support matches independent Gram')
        check(expected is not None,'strict support exists')
        canonical_cases+=1
cube=shells[0][0]
check(len(canonical_gram(cube,(F(1),)*3))==2,'eight-site shell qmin2')
regular=((0,0,0),(2,2,0),(2,0,2),(0,2,2))
check(all(w>0 for w in gram(regular)[2]),'q4 generated strict but noncanonical on cube shell')
check(len(canonical_gram(cube,gram(regular)[0]))==2,'lower-arity S* rejects q4 generator without retagging coefficients')

# Find an admissible integer reflection/permutation with an obtuse first-three Morton prefix but strict q4.
def morton(p):
    return sum(((p[j]>>b)&1)<<(3*b+j) for b in range(5) for j in range(3))
base=((1,2,6),(8,4,8),(2,1,3),(7,8,5))
obtuse_prefix=None
for axes in permutations(range(3)):
    for reflect in product((0,1),repeat=3):
        cloud=tuple(sorted((tuple(10-p[axes[j]] if reflect[j] else p[axes[j]] for j in range(3)) for p in base),key=morton))
        angles=[dot(diff(cloud[j],cloud[i]),diff(cloud[k],cloud[i])) for i,j,k in ((0,1,2),(1,0,2),(2,0,1))]
        if min(angles)<0:
            check(all(w>0 for w in gram(cloud)[2]),'strict q4 survives obtuse q3 prefix')
            obtuse_prefix={'points':cloud,'angles':angles}
            break
    if obtuse_prefix:
        break
check(obtuse_prefix is not None,'Morton-ordered obtuse prefix fixture')
check('else if constexpr (q == 3) q3();' in leaf and
      'if constexpr (q < 4)' in leaf,'q3 rejection does not govern extension')

# Cache semantics: hits/evaluations count logical requests, though device evaluates each request physically.
requests=[(0,1,2),(0,1,3),(0,2,3),(1,2,3),(0,1,2),(0,1,3),(0,2,3)]
seen=set(); evaluations=hits=0
for i,j,k in requests:
    rank=k*(k-1)*(k-2)//6+j*(j-1)//2+i
    if rank in seen:
        hits+=1
    else:
        evaluations+=1
    seen.add(rank)
check(evaluations==4 and hits==3 and evaluations+hits==len(requests),'same cache counters, seven physical calls')

print(json.dumps({'status':'PASS','scope':'independent Fraction/Gram plus source guards; no host/device native execution',
                  'checks':checks,'balls':balls,'native_powers':powers,'center_orientations':orientations,
                  'native_q4_weights':native_weights,'cube_unresolved_cases':unresolved_weights,
                  'sat_cases':sat_cases,'canonical_cases':canonical_cases,'obtuse_prefix_fixture':obtuse_prefix,
                  'cache_request_model':{'logical_evaluations':evaluations,'logical_hits':hits,'physical_calls':len(requests)},
                  'measured_partial_magnitude_bits':bounds,'causal_q3_refusal':causal,
                  'snapshot_base':before['base_commit'],'device_header_sha256':before['sources']['src/catalogue/leaf_device.hpp']['sha256'],
                  'predicates_header_sha256':before['sources']['src/catalogue/leaf_device_predicates.hpp']['sha256']},indent=2,sort_keys=True))
