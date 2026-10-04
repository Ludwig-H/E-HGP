"""Independent scalar Fraction checks, source guards and mutation witnesses; no native execution."""
from fractions import Fraction as F
from itertools import permutations, combinations
from pathlib import Path
import hashlib
import importlib.util
import json
import sys

BASE = Path(__file__).resolve().parent
SRC = BASE / 'source' / 'morsehgp3D_v11'
checks = 0


def require(ok, label):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(label)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def form(a, b, c):
    u, v = sub(b, a), sub(c, a)
    w = cross(u, v)
    g = dot(w, w)
    if not g:
        return None
    uu, vv = dot(u, u), dot(v, v)
    t = tuple(uu*y-vv*x for x, y in zip(u, v))
    n, d = cross(t, w), 2*g
    raw = (uu*vv*dot(sub(c, b), sub(c, b)), 4*g)
    # Independent 2x2 Gram solve, avoiding cross products for the centre.
    uv = dot(u, v)
    determinant = uu*vv-uv*uv
    alpha = F(vv*(uu-uv), 2*determinant)
    beta = F(uu*(vv-uv), 2*determinant)
    center = tuple(F(x)+alpha*y+beta*z for x, y, z in zip(a, u, v))
    require(tuple(F(x)+F(y, d) for x, y in zip(a, n)) == center, 'centre Gram')
    radius = dot(sub(center, a), sub(center, a))
    require(F(*raw) == radius, 'raw degree-six radius')
    require(raw[1] == 2*d, 'deferred denominator 2D unchanged')
    return n, d, raw, center, radius


def power(a, n, d, x):
    u = sub(x, a)
    return d*dot(u, u)-2*dot(n, u)


source_sphere = (SRC / 'src/num/sphere.cpp').read_text()
source_leaf = (SRC / 'src/catalogue/leaf.cpp').read_text()
source_pred = (SRC / 'src/num/predicates.cpp').read_text()
source_header = (SRC / 'src/num/geometry.hpp').read_text()
start = source_sphere.index('Q3Candidate::through(')
stop = source_sphere.index('Result<Sphere> Q3Candidate::materialize()', start)
require('checked_level' not in source_sphere[start:stop], 'factory has no Level')
require('to_wide(2 * denominator_)' in source_sphere[stop:], 'source raw denominator')
require('detail::dot(u, u)} * detail::dot(v, v)' in source_sphere[stop:], 'source raw numerator')
require('bool q3_power_i128_certified() const noexcept' in source_header, 'power certificate separate')
require('arity_(sphere.presentation_arity())' in source_pred[source_pred.index('explicit CenterView(const Q3Candidate&'):], 'tag preserved')
census = source_leaf[source_leaf.index('Outcome census_and_emit('):source_leaf.index('// logical :')]
require(census.index('if (support != generated)') < census.index('if (p + qmin') < census.index('emission_level(sphere'), 'S*/admission before materialize')
require('if (!num::strictly_acute(a, b, c)) return std::optional<num::Q3Candidate>{};' in source_leaf, 'acute guard only q3')
require('MHGP11_TRY(extend(leaf, q, i + 1, next, next_logical));' in source_leaf, 'q4 recursion kept')

profiles = []
for bits in (18, 21, 24):
    m = (1 << bits)-1
    s = (1 << (bits-1))-1
    f = ((0,0,0), (2,2,0), (2,0,2), (0,2,2))
    g = tuple(tuple(t*s+1 for t in p) for p in f)
    cases = (f[:3], g[:3], (g[3],g[2],g[1]),
             ((0,0,0),(4,0,0),(2,3,0)),
             ((0,0,0),(m,1,0),(m-1,1,0)),
             ((1,2,3),(m,7,11),(5,m,m-3)))
    queries = ((0,0,0),(m,m,m),(m//2,m//2,m//2),(0,m,m),(m,0,3))
    for triangle in cases:
        raw_reference = None
        for a,b,c in permutations(triangle):
            n,d,raw,center,radius = form(a,b,c)
            require(d > 0 and d.bit_length() <= 4*bits+5, 'D budget')
            require(all(abs(t).bit_length() <= 5*bits+5 for t in n), 'N budget')
            require(raw[0].bit_length() <= 6*bits+5, 'raw numerator degree6 budget')
            require(raw[1].bit_length() <= 4*bits+6, 'raw denominator degree4 budget')
            if raw_reference is None:
                raw_reference = raw
            require(raw_reference == raw, 'six permutation raw-level invariance')
            for x in (a,b,c)+queries:
                value = power(a,n,d,x)
                require(F(value,d) == dot(sub(x,center),sub(x,center))-radius, 'signed power exact')
            require(all(power(a,n,d,x)==0 for x in (a,b,c)), 'all support contacts')
            require(dot(cross(sub(b,a),sub(c,a)),sub(center,a))==0, 'centre remains in support plane')
    a,b,c = g[:3]
    n,d,raw,center,radius = form(a,b,c)
    opposite = power(a,n,d,g[3])
    require(opposite == 256*s**6, 'native mutant causal power witness')
    require(opposite.bit_length() == {18:110,21:128,24:146}[bits], 'profile actual witness width')
    certified = d < 1 << (123-2*bits) and all(abs(t)<1 << (124-bits) for t in n)
    require(certified == (bits == 18), 'global certificate guard')
    if bits > 18:
        require(opposite > (1 << 127)-1, 'false tag4 exceeds signed i128')
    require((3*raw[0],3*raw[1]) != raw and F(3*raw[0],3*raw[1])==F(*raw), 'raw-level mutant value unchanged encoding changed')
    profiles.append({'bits':bits,'opposite_power_bits':opposite.bit_length(),'power_certificate':certified})

# Causal guard for the obtuse-admission mutant on an ACTUAL pre-existing oracle fixture.
path = SRC / 'tests/catalogue/fixtures.py'
spec = importlib.util.spec_from_file_location('pinned_catalogue_fixtures', path)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
found = None
for fixture in module.fixtures(18):
    if len(fixture.points) > 32:
        continue
    lower = tuple(min(p[j] for p in fixture.points) for j in range(3))
    upper = tuple(max(p[j] for p in fixture.points)+1 for j in range(3))
    for a,b,c in combinations(fixture.points,3):
        if all(dot(sub(y,x),sub(z,x))>0 for x,y,z in ((a,b,c),(b,a,c),(c,a,b))):
            continue
        candidate = form(a,b,c)
        if candidate is None:
            continue
        n,d,raw,center,radius = candidate
        if not all(lo<=x<hi for lo,x,hi in zip(lower,center,upper)):
            continue
        sides = [power(a,n,d,x) for x in fixture.points]
        p = sum(t<0 for t in sides)
        shell = sum(t==0 for t in sides)
        k = max(2,p+2)
        if shell == 3 and k<=12:
            found={'fixture':fixture.name,'k':k,'p':p,'m':shell,'triangle':[list(a),list(b),list(c)],
                   'raw_level':list(raw),'center':[str(t) for t in center]}
            break
    if found:
        break
require(found is not None, 'oracle has reached non-strict mutant fixture')
# All sites of this unsplit root lie in its closure: no strict uniform dominator can
# exist for x (test centre=x); J2 also has this exact centre as a witness. In m==q
# the mutant skips canonicalisation and emits a non-positive q3, unlike the oracle.
for filename in ('num.json','catalogue.json'):
    mutants = json.loads((SRC/'tests/mutants'/filename).read_text())['mutants']
    for mutant in mutants:
        if mutant['id'] not in ('candidat_q3_tague_q4_u21','candidat_q3_niveau_brut_change','presentation_q3_taguee_q4_u21','q3_candidat_obtus_admis'):
            continue
        text = (SRC/mutant['fichier']).read_text()
        require(text.count(mutant['cherche'])==1, 'unique mutation source '+mutant['id'])
print(json.dumps({'status':'PASS','checks':checks,'native_execution':False,'profiles':profiles,
                  'obtuse_mutant_actual_fixture':found,
                  'scope':'independent Gram/Fraction formula and source guards; no C++ execution or native mutant kill'},sort_keys=True,indent=2))
