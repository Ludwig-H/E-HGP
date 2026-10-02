"""Budgets de puissance et temoins exacts : entiers/Fraction autonomes."""
from fractions import Fraction as F
from itertools import permutations
import json


def require(condition, note):
    if not condition:
        raise RuntimeError(note)


rows = []
for bits in (18, 21, 24):
    m = 1 << bits
    bounds = [72 * m ** 6] + [48 * m ** 6] * 3
    cumulative = []
    for ordering in permutations(bounds):
        partial = 0
        for term in ordering:
            partial += term
            cumulative.append(partial)
    require(max(cumulative) == 216 * m ** 6, 'majorant de tous les ordres de somme')
    native_by_bound = max(cumulative) < 1 << 127
    require(native_by_bound == (bits == 18), 'preuve universelle i128 seulement u18')
    require(216 * m ** 6 < 1 << (6 * bits + 8), 'Budget::side inclut les intermediaires')

    # Triangle aigu presque droit : a=(0,0,0), b=(s,0,0), c=(1,s,0).
    # Centre derive des deux bisecteurs, independamment des expressions C++.
    s = m - 1
    a, b, c, point = (0, 0, 0), (s, 0, 0), (1, s, 0), (s, s, s)
    center = (F(s, 2), F(s * s - s + 1, 2 * s), F(0))
    radius = sum(x * x for x in center)
    require(all(sum((F(x) - y) ** 2 for x, y in zip(vertex, center)) == radius
                for vertex in (a, b, c)), 'sphere par les trois sites')
    require(s > 1 and s * s > s - 1, 'triangle strictement aigu')
    denominator = 2 * s ** 4
    numerator = (s ** 5, s ** 5 - s ** 4 + s ** 3, 0)
    require(tuple(F(n, denominator) for n in numerator) == center, 'centre N/D')
    first = denominator * sum(v * v for v in point)
    terms = [-2 * n * v for n, v in zip(numerator, point)]
    exact = denominator * (sum((F(x) - y) ** 2 for x, y in zip(point, center)) - radius)
    require(exact.denominator == 1 and first + sum(terms) == exact, 'puissance exacte sans rayon explicite')
    require(exact == 2 * s ** 6 + 2 * s ** 5 - 2 * s ** 4, 'formule developpee du temoin')
    if bits == 21:
        require(first >= 1 << 127 and 0 < exact < 1 << 127,
                'u21 : resultat representable mais premier produit deborde i128')
    if bits == 24:
        require(exact >= 1 << 127, 'u24 : resultat lui-meme trop large')
    rows.append({'bits': bits, 'side_budget': 6 * bits + 8,
                 'universal_bound_fits_i128': native_by_bound,
                 'max_partial_bound_bits': max(cumulative).bit_length(),
                 'acute_witness': {'coordinates': (a, b, c, point),
                                   'center': [str(x) for x in center],
                                   'denominator': str(denominator),
                                   'first_product': str(first), 'first_product_bits': first.bit_length(),
                                   'exact_power': str(exact), 'exact_power_bits': exact.numerator.bit_length()}})

print(json.dumps({'status': 'PASS', 'scope': 'autonomous proof and exact witnesses; no native execution',
                  'profiles': rows}, sort_keys=True, indent=2))
