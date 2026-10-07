#!/usr/bin/env python3
"""Five public synthetic points per seed; integer determinant, no cloud file or native run."""
from itertools import permutations
import json
import math


def words(seed, count):
    mask = 2**64 - 1
    for i in range(1, count + 1):
        z = (seed + i * 0x9E3779B97F4A7C15) & mask
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & mask
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & mask
        yield z ^ (z >> 31)


def first_five(seed):
    # prepare_small.py: gaussian_int(2^20,1), 12 uniforms per coordinate; projection then integer rounding.
    stream = iter(words(seed, 5 * 3 * 12))
    out = []
    for _ in range(5):
        g = [16 * (sum(next(stream) >> 48 for _ in range(12)) - 6 * 2**16) for _ in range(3)]
        norm = math.sqrt(sum(float(v) * float(v) for v in g))
        out.append(tuple(math.floor(v * (10000.0 / norm) + 0.5) for v in g))
    if len(set(out)) != 5:
        raise RuntimeError('unexpected duplicate in first five generated points')
    return out


def determinant(matrix):
    n = len(matrix)
    total = 0
    for p in permutations(range(n)):
        sign = (-1)**sum(p[i] > p[j] for i in range(n) for j in range(i + 1, n))
        total += sign * math.prod(matrix[i][p[i]] for i in range(n))
    return total


def main():
    cases = []
    for n in (100, 3000, 10000):
        seed = 20261007 + 2000 + n
        points = first_five(seed)
        det = determinant([(*p, sum(v*v for v in p), 1) for p in points])
        if det == 0:
            raise RuntimeError('fixture unexpectedly cospherical')
        cases.append(dict(nominal_sites=n, generated_sites=5, seed=seed, points=points,
                          cosphericity_determinant=det, all_on_one_exact_sphere=False))
    print(json.dumps(dict(schema='ehgp.v12.audit_performance.degeneracy.v1',
                         document_pin='5c5fc710921ac03f6d4c2aca40f17e8442202a5c',
                         cases=cases, native_engine_executed=False, private_data_read=False),
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
