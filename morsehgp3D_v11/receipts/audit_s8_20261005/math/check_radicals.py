#!/usr/bin/env python3
"""Bounded S8 mathematical countercheck. Python model, no C++ or native probe execution."""
import hashlib
import json
import math
import pathlib
import random
import sys
from fractions import Fraction as Q

sys.dont_write_bytecode = True
HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE / 'snapshot/morsehgp3D_v11'
sys.path.insert(0, str(ROOT / 'tests/num'))
import radical_port as port

PRIMES = (2, 3, 5)
SIGNATURE_PRIMES = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
                    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173)
CAPACITY = 17408


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sig(n):
    out = []
    for p in SIGNATURE_PRIMES:
        unit, valuation = n, 0
        while unit % p == 0:
            unit //= p; valuation += 1
        out.append((valuation % 2, unit % 8 if p == 2 else pow(unit % p, (p-1)//2, p)))
    return tuple(out)


def integer_classes(terms):
    classes = []
    for coef, rad in terms:
        rad = Q(rad)
        if not rad or not coef:
            continue
        n, c = rad.numerator * rad.denominator, Q(coef, rad.denominator)
        signature = sig(n)
        for cls in classes:
            if cls[2] != signature:
                continue
            product = n * cls[0]
            require(product.bit_length() <= CAPACITY, 'unexpected product capacity in bounded check')
            s = math.isqrt(product)
            if s*s == product:
                cls[1] += c * Q(s, cls[0]); break
        else:
            classes.append([n, c, signature])
    return [(n, c) for n, c, _s in classes if c]


def model(terms):
    classes = integer_classes(terms)
    if not classes:
        return (0, 0, 0)
    if len(classes) == 1:
        return (port.sign(classes[0][1]), 1, 0)
    if len(classes) == 2:
        (n, a), (m, b) = classes
        answer = port.sign(a) if port.sign(a) == port.sign(b) else port.sign(a)*port.sign(a*a*n-b*b*m)
        return answer, 2, 0
    for bits in (96, 192, 384, 768, 1536, 3072, 6144):
        low = high = Q(0)
        for n, c in classes:
            if n.bit_length()+2*bits > CAPACITY:
                return 'capacity_refusal', len(classes), bits
            s = math.isqrt(n << (2*bits))
            low += c*(s if c > 0 else s+1)
            high += c*(s+1 if c > 0 else s)
        if low > 0 or high < 0:
            return (1 if low > 0 else -1), len(classes), bits
    return 'precision_refusal', len(classes), 6144


def product(a, b):
    """Exact multiplication in the basis sqrt(product of a subset of 2,3,5)."""
    out = {}
    for i, c in a.items():
        for j, d in b.items():
            scalar = math.prod(p for bit, p in enumerate(PRIMES) if (i & j) >> bit & 1)
            key = i ^ j
            out[key] = out.get(key, Q(0)) + c*d*scalar
    return {m:c for m,c in out.items() if c}


def field_sign(value, depth=3):
    """Independent exact recursive squaring in Q(sqrt2,sqrt3,sqrt5), without isqrt intervals/classes."""
    value = {m:c for m,c in value.items() if c}
    if not value:
        return 0
    if depth == 0:
        return port.sign(value.get(0, Q(0)))
    bit, p = 1 << (depth-1), PRIMES[depth-1]
    a = {m:c for m,c in value.items() if not m & bit}
    b = {m ^ bit:c for m,c in value.items() if m & bit}
    sa, sb = field_sign(a, depth-1), field_sign(b, depth-1)
    if not sa: return sb
    if not sb or sa == sb: return sa
    difference = product(a, a)
    for m,c in product(b, b).items():
        difference[m] = difference.get(m, Q(0))-p*c
    return sa*field_sign(difference, depth-1)


require(not port.source_mismatches(str(ROOT/'bench/points_radius.py')), 'ported Python source differs by AST')
rng = random.Random(20261005)
cases = []
for at in range(80):
    terms, basis = [], {}
    for j in range(1 + at % 8):
        mask = rng.randrange(8)
        squarefree = math.prod(p for i,p in enumerate(PRIMES) if mask >> i & 1)
        coefficient = Q(rng.randrange(-7,8), rng.choice((1,2,3,7,31)))
        factor = Q(rng.randrange(1,11), rng.randrange(1,11))
        rad = squarefree * factor * factor
        terms.append((coefficient, rad))
        basis[mask] = basis.get(mask,Q(0)) + coefficient*factor
        if at % 4 == 0:
            terms.append((-coefficient/3, 9*rad))
            basis[mask] -= coefficient*factor
    answer, classes, bits = model(terms)
    require(answer == field_sign(basis) == port.sign_of_radicals(terms), 'independent multiquadratic sign differs')
    # RootTable model: non-reduced levels, integer certificates and outward signed brackets.
    signed_levels = []
    for c, rad in terms:
        if c:
            signed_levels.append((1 if c > 0 else -1, c*c*rad))
    low = high = 0
    for sign, level in signed_levels:
        n, d = level.numerator*49, level.denominator*49
        r = math.isqrt((n << 128)//d)
        require(r*r*d <= n << 128 < (r+1)*(r+1)*d, 'RootTable integer certificate')
        low += r if sign > 0 else -r-1
        high += r+1 if sign > 0 else -r
    require(high-low == len(signed_levels), 'RootTable width differs from term count')
    decisive = 1 if low > 0 else -1 if high < 0 else None
    require(decisive is None or decisive == answer, 'RootTable false sign')
    cases.append(dict(sign=answer, classes=classes, bits=bits, bracket_decides=decisive is not None))

# A deliberate signature collision proves that an equal fingerprint must still test a perfect square.
modulus = 8 * math.prod(SIGNATURE_PRIMES[1:])
collision = modulus + 1
while math.isqrt(collision)**2 == collision:
    collision += modulus
require(sig(collision) == sig(1) and len(integer_classes([(1, Q(collision)),(-1,Q(1))])) == 2,
        'signature collision wrongly identifies square classes')
require(model([(1,Q(collision)),(-1,Q(1))])[0] == 1, 'collision sign')

equivalent = 0
for at in range(80):
    s = rng.choice((1,2,3,5,6,10,15,30))
    a,b = rng.getrandbits(120)+1, rng.getrandbits(110)+1
    require(sig(s*a*a) == sig(s*b*b), 'signature rejects equivalent square classes')
    equivalent += 1

limits = []
for bits in (18,21,24):
    numerator, denominator = 3*((1 << bits)-1)**2*173, 173
    r = math.isqrt((numerator << 128)//denominator)
    require(r.bit_length() <= bits+65 and 16*(r+1) < 1 << 94, 'RootTable root/i128 domain bound')
    limits.append(dict(coord_bits=bits, root_bits=r.bit_length(), root_capacity=bits+65))

def second(n):
    return [(1,Q(n*n)),(-2,Q(n*n+1)),(1,Q(n*n+2))]
quasi = model(second(1 << 40))
require(quasi == (-1,3,192), 'second-order quasi equality')
budget = model(second((1 << 2050)+1))
require(budget == ('precision_refusal',3,6144), 'precision exhausted must refuse')
try:
    port.sign_of_radicals(second((1 << 2050)+1))
except port.Refusal:
    pass
else:
    raise RuntimeError('Python port lost its explicit refusal')

result = dict(pin='53c027fe848b0d890f164eb87ebf347338c58d55', native_executed=False,
              scope='bounded Python translation + independent multiquadratic sign + integer certificates',
              field_sign_checks=len(cases), equality_checks=sum(c['sign']==0 for c in cases),
              root_brackets_decide=sum(c['bracket_decides'] for c in cases), equivalent_signatures=equivalent,
              signature_collision=dict(n=str(collision), bits=collision.bit_length(), classes=2, sign=1),
              root_limits=limits, quasi=quasi, explicit_precision_refusal=budget,
              decisions_sha256=hashlib.sha256(json.dumps(cases,sort_keys=True,separators=(',',':')).encode()).hexdigest())
print(json.dumps(result,sort_keys=True,separators=(',',':')))
