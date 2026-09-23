#!/usr/bin/env python3
"""Oracle combinatoire autonome du préfixe d'intrus, sans géométrie ni moteur."""

from itertools import combinations


def first_outside(interior, selected):
    return next((u for u in interior if u not in selected), None)


def check():
    pairs = 0
    negative = 0
    for n in range(2, 8):
        universe = tuple(range(n))
        facets = [s for k in range(1, min(4, n) + 1)
                  for s in combinations(universe, k)]
        for m in range(1, n + 1):
            for interior in combinations(universe, m):
                for first in facets:
                    z = first_outside(interior, first)
                    if z is None:
                        if not set(interior).issubset(first):
                            raise AssertionError(("negative", n, interior, first))
                        negative += 1  # Le cache ne stocke aucune réponse −1.
                        continue
                    prefix = tuple(u for u in interior if u <= z)
                    reconstructed = tuple(u for u in first if u < z and u in interior) + (z,)
                    if reconstructed != prefix:
                        raise AssertionError(("initial", n, interior, first))
                    for second in facets:
                        cached = first_outside(prefix, second)
                        exact = first_outside(interior, second)
                        if cached is not None:
                            if cached != exact:
                                raise AssertionError(("hit", n, interior, first, second))
                        elif exact is not None:
                            extension = tuple(u for u in second if z < u < exact and u in interior) + (exact,)
                            full = tuple(u for u in interior if u <= exact)
                            if exact <= z or prefix + extension != full:
                                raise AssertionError(("extend", n, interior, first, second))
                        pairs += 1
    if pairs != 1_336_782:
        raise AssertionError(("pair_count", pairs))
    print(f"PASS pairs={pairs} negative={negative}")


if __name__ == "__main__":
    check()
