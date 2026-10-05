#!/usr/bin/env python3
"""Exact high-K primitive guards, not a native or full SupportsOracle run."""
from collections import Counter
from itertools import combinations, product
from math import comb, gcd
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parent
CHECKS = 0

def need(ok, why):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise RuntimeError(why)

def dot(a, b):
    return sum(x*y for x, y in zip(a, b))

def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))

def det(a, b, c):
    return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0])

def positive_support(ps):
    # All points have equal nonzero norm, centre = 0. A minimal support
    # is affine-independent with 0 in relative interior.
    if len(ps) == 2:
        return all(x+y == 0 for x, y in zip(*ps))
    if len(ps) == 3:
        a, b, c = ps
        return det(a, b, c) == 0 and all(dot(sub(y, x), sub(z, x)) > 0 for x, y, z in ((a,b,c),(b,a,c),(c,a,b)))
    a, b, c, d = ps
    denominator = det(sub(b,a), sub(c,a), sub(d,a))
    numerators = (det(b,c,d), -det(a,c,d), det(a,b,d), -det(a,b,c))
    return denominator != 0 and all(n*denominator > 0 for n in numerators)

def support_masks(points):
    return tuple(sum(1 << i for i in ids) for a in (2,3,4)
                 for ids in combinations(range(len(points)), a)
                 if positive_support(tuple(points[i] for i in ids)))

def contains_support(mask, supports):
    return any(mask & q == q for q in supports)

def intersection_line(a, b):
    v = (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
    g = gcd(gcd(abs(v[0]), abs(v[1])), abs(v[2]))
    if g == 0:
        raise RuntimeError('distinct antipodal directions required')
    v = tuple(x//g for x in v)
    return v if next(x for x in v if x) > 0 else tuple(-x for x in v)

def choose(n, k):
    return comb(n, k) if 0 <= k <= n else 0

def counts(p, m, k, closure):
    t = k-p
    return {'kparties_reliees':choose(p+m,k), 'compressed_parts':choose(m,t),
            'strict_traces':choose(m,t)-closure.get(t,0),
            'cofaces':sum(choose(p,k+1-j)*closure.get(j,0) for j in range(2,m+1)),
            'gabriel_cofaces':closure.get(t+1,0)}

def main():
    before = json.loads((ROOT/'BEFORE.json').read_text())
    for source in before['sources']:
        need(hashlib.sha256((ROOT/source['owned']).read_bytes()).hexdigest() == source['sha256'], 'source pin '+source['owned'])
    need(sum(s.get('equals_previous_closed',False) for s in before['sources']) == 12, 'all twelve S6 files unchanged')
    cpp = (ROOT/'s6/src/supports/counts.cpp').read_text()
    need('counts.strict_traces = counts.compressed_parts - closure.parts(static_cast<u32>(t));' in cpp, 'source strict-trace formula')
    need('u64{supports_detail::binomial(p, k + 1 - j)} * closure.parts' in cpp, 'source distinct-coface formula')

    points = tuple((x,y,z) for x in range(-2,3) for y in range(-2,3) for z in range(-2,3) if x*x+y*y+z*z == 5)
    supports = support_masks(points)
    by_arity = dict(sorted(Counter(q.bit_count() for q in supports).items()))
    need(len(points) == 24 and by_arity == {2:12,3:24,4:792}, 'certified Sphere5 Q family')
    pairs = tuple((i,points.index(tuple(-x for x in p))) for i,p in enumerate(points) if p < tuple(-x for x in p))
    need(len(pairs) == 12 and {i for pair in pairs for i in pair} == set(range(24)), 'antipodal partition')
    separable = []
    for choices in product((0,1), repeat=12):
        mask = sum(1 << pairs[j][c] for j,c in enumerate(choices))
        need(mask.bit_count() == 12, 'one site per opposite pair')
        if not contains_support(mask, supports):
            separable.append(mask)
    # Independent count: add twelve distinct central planes. A new plane
    # meets earlier ones along L distinct lines, yielding 2L new chambers.
    normals = tuple(points[i] for i,j in pairs)
    increments = (1,)+tuple(len({intersection_line(normals[i],normals[j]) for j in range(i)}) for i in range(1,12))
    chambers = 2*sum(increments)
    need(len(separable) == chambers == 116, 'support closure and independent plane arrangement agree')
    closure = {12:comb(24,12)-len(separable)}
    closure.update({j:comb(24,j) for j in range(13,25)})
    high = counts(0,24,12,closure)
    need(high == {'kparties_reliees':2704156,'compressed_parts':2704156,'strict_traces':116,'cofaces':2496144,'gabriel_cofaces':2496144}, 'exact K12 oracle')
    incidences = {a:comb(24-a,13-a) for a in by_arity}
    total_incidence = sum(by_arity[a]*incidences[a] for a in by_arity)
    need(total_incidence == 149954688 and total_incidence > high['cofaces'], 'incidences are not distinct cofaces')

    # Small native-fixture recommendation at K10: eight strict interiors
    # plus four square corners. No oracle enumeration above 2**4 is needed.
    corners = ((-10,-10,0),(-10,10,0),(10,-10,0),(10,10,0))
    interiors = ((-1,-1,0),(-1,0,0),(-1,1,0),(0,-1,0),(0,0,0),(0,1,0),(1,-1,0),(1,0,0))
    square_q = support_masks(corners)
    need(Counter(q.bit_count() for q in square_q) == {2:2}, 'square only two minimal diagonal supports')
    need(all(dot(p,p) < 200 for p in interiors) and all(dot(p,p) == 200 for p in corners), 'exact complete I8 U4 census')
    square_n = {j:sum(contains_support(sum(1<<i for i in ids),square_q) for ids in combinations(range(4),j)) for j in range(5)}
    small = counts(8,4,10,square_n)
    need(small == {'kparties_reliees':66,'compressed_parts':6,'strict_traces':4,'cofaces':12,'gabriel_cofaces':4}, 'small exact K10 counts')
    need(2*comb(10,9) == 20 > small['cofaces'], 'small K10 distinctness guard')
    native = tuple(tuple(x+10 for x in p) for p in corners+interiors)
    need(len(native) == len(set(native)) == 12 and all(0 <= x < (1<<18) for p in native for x in p), 'recommended K10 native coordinates admissible')
    print(json.dumps({'status':'PASS','checks':CHECKS,'native_executed':False,
      'source_pin':before['published_pin'],'s6_unchanged_files':12,
      'sphere5':{'sites':24,'Q_by_arity':by_arity,'antipodal_choices_tested':4096,
        'separable12':len(separable),'N12':closure[12],'N13_to_N24':{j:closure[j] for j in range(13,25)},
        'independent_arrangement_chambers':chambers,'arrangement_increments':increments,
        'K12_counts':high,'support_cofaces_by_arity_K12':incidences,'incidences_K12':total_incidence},
      'recommended_small_K10':{'points':native,'center':[10,10,10],'squared_radius':200,'p':8,'m':4,'qmin':2,'N':square_n,'counts':small,'incidences':20},
      'scope':['Exact scalar/geometry oracle; no C++/GPU execution or FULL reconstruction.',
        'Existing Sphere5 proof independently checked all Q presentations with Gram; this follow-up adds only high-K closure and counts.',
        'Sphere5 uses a canonical q2 sphere; this does not replace native q3/q4 factory and overflow gates.',
        'N_j above twelve follows the pigeonhole diameter proof; no enumeration of 2**24 subsets.',
        'S3 low-size fixture coverage is K<=5; a separate source gate targets LiDAR K10. No failure or new native qualification is inferred.']},sort_keys=True,indent=2))

if __name__ == '__main__':
    main()
