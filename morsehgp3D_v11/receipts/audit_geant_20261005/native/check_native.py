#!/usr/bin/env python3
"""Audit portable stdlib ; aucun moteur natif execute."""
from functools import lru_cache
from math import comb
from pathlib import Path
import json

PIN = '238734f1d03ab32e2a722bf036eb8fc5626dfd44'
checks = 0

def require(condition, message):
    global checks
    checks += 1
    if not condition:
        raise RuntimeError(message)

@lru_cache(maxsize=None)
def expected_nodes(n, leaf):
    if n <= leaf:
        return 1
    return 1 + expected_nodes(n // 2, leaf) + expected_nodes(n - n // 2, leaf)

def product_nodes(n, leaf):
    width = 1
    while n // width > leaf:
        width *= 2
    extra = n % width if n // width == leaf else 0
    return 2 * (width + extra) - 1

for leaf in range(1, 257):
    for n in range(1, 1025):
        require(product_nodes(n, leaf) == expected_nodes(n, leaf), f'nodes {n}/{leaf}')
    for n in (2**18 - 1, 2**21 - 1, 2**24 - 1, 2**31, 2**32 - 2):
        require(product_nodes(n, leaf) == expected_nodes(n, leaf), f'large nodes {n}/{leaf}')

certificates = []
for bits in (18, 21, 24):
    m = 2**bits
    d = 2**(123 - 2 * bits) - 1
    n = 2**(124 - bits) - 1
    power = d * 3 * (m - 1)**2 + 6 * n * (m - 1)
    require(power < 2**127, f'q3 power {bits}')
    d = 2**(124 - 3 * bits) - 1
    n = 2**(124 - 2 * bits) - 1
    orientation = 3 * (n + d * (m - 1)) * (m - 1)**2
    require(orientation < 2**127, f'orientation {bits}')
    q4 = 72 * m**5
    require(q4 < 2**127, f'q4 power {bits}')
    certificates.append(dict(bits=bits, q3_power_bound_bits=power.bit_length(),
                             orientation_bound_bits=orientation.bit_length(),
                             q4_power_bound_bits=q4.bit_length()))
require(117 * (2**20)**6 < 2**127, 'q4 barycentric local')

for top in range(36):
    for bottom in range(min(top, 13) + 1):
        require(comb(top, bottom) < 2**32, f'counts {top}/{bottom}')

# Domaine public Shape puis fonction support_cofaces telle que le WIP la code.
def native_support_cofaces(p, m, k, arity):
    if arity < 2 or arity > 4:
        return 0
    top, bottom = p + m - arity, k + 1 - arity
    return 0 if bottom < 0 or bottom > top else comb(top, bottom)

counterexamples = []
for k in range(1, 13):
    for p in range(12):
        for m in range(2, 25):
            for q in range(2, min(4, m) + 1):
                if p + q > k + 1:
                    continue
                for arity in range(2, 5):
                    if arity > m and native_support_cofaces(p, m, k, arity) != 0:
                        counterexamples.append(dict(p=p, m=m, qmin=q, k=k, arity=arity,
                                                    got=native_support_cofaces(p, m, k, arity), expected=0))
require(dict(p=2, m=2, qmin=2, k=3, arity=3, got=1, expected=0) in counterexamples,
        'public helper witness')

source_manifest = json.loads(Path(__file__).with_name('source_manifest.json').read_text())
output = dict(pin=PIN, checks=checks, native_run=False, gcp=False, certificates=certificates,
              support_cofaces_invalid_arity_cases=len(counterexamples),
              support_cofaces_invalid_arity_first=counterexamples[:8],
              source_manifest=source_manifest)
print(json.dumps(output, sort_keys=True, indent=2))
