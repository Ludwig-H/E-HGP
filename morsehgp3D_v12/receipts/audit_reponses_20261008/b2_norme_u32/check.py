#!/usr/bin/env python3
"""Fixtures publiques q2 ; oracle rationnel indépendant. Aucun appel au moteur."""
from fractions import Fraction as F
import json

U32 = (1 << 32) - 1
I64 = (1 << 63) - 1
I128 = (1 << 127) - 1


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sign(x):
    return (x > 0) - (x < 0)


def certificate(d, n, t):
    # Inégalités exactes du certificat ; recherche exhaustive des exposants,
    # sans reprendre la proposition par longueurs de bits du code C++.
    return 0 <= t <= 61 and 0 < d < (1 << (123 - 2*t)) and all(
        -(1 << (124-t)) < v < (1 << (124-t)) for v in n)


def fixture(s):
    b = 1 << (s-1)
    a, end = (0, 0, 0), (b, 0, 0)
    center = tuple(F(x+y, 2) for x, y in zip(a, end))
    radius2 = sum((F(x)-c)**2 for x, c in zip(a, center))
    n, d = end, 2  # Sphere::through(a,b) : N=b-a, D=2, sans réduction.
    actual_span = max(end).bit_length()
    need(actual_span == s and a != end, 'support non dégénéré et étendue')
    need(all(0 <= x <= U32 for p in (a, end) for x in p), 'support public u32')
    need(sum((F(x)-c)**2 for x, c in zip(end, center)) == radius2, 'q2 milieu')
    domain = max(t for t in range(62) if certificate(d, n, t))
    need(domain == 60 and domain >= s+2, 'voie certifiée au domaine gardé')
    need(s > 16 and all(abs(v) <= I128 for v in (*n, d)), 'coefficients natifs et palier non étroit')
    m = 1 << s
    q = min(2*m-1, U32)
    queries = {'anchor': a, 'other_support': end,
               'center': tuple(int(x) for x in center), 'far': (q, q, q)}
    signs, values = {}, {}
    for label, x in queries.items():
        need(all(0 <= z <= U32 for z in x), 'requête publique u32')
        need(all(-m < z < 2*m for z in x), 'requête strictement dans le pavé')
        norm = sum(z*z for z in x)
        terms = [d*norm] + [-2*n[j]*x[j] for j in range(3)]
        partial = 0
        for term in terms:
            partial += term
            need(abs(term) <= I128 and abs(partial) <= I128, 'produits/sommes i128')
        geometric = sum((F(z)-c)**2 for z, c in zip(x, center)) - radius2
        need(F(partial, d) == geometric, 'puissance locale vs géométrie Fraction')
        # Autre ancre publique du même segment : coefficients négatifs.
        v = tuple(x[j]-end[j] for j in range(3))
        reverse = d*sum(z*z for z in v) + 2*b*v[0]
        need(reverse == partial, 'invariance du repère par inversion du support')
        signs[label], values[label] = sign(geometric), partial
    need(signs == {'anchor': 0, 'other_support': 0, 'center': -1, 'far': 1}, 'signes exacts')
    norm = 3*q*q
    # Troncature signée 64 DÉFINIE en Python : témoin d'une largeur erronée,
    # pas simulation qualifiée d'un débordement signé C++ (qui serait UB).
    truncated = (norm+(1 << 63)) % (1 << 64) - (1 << 63)
    wrong = d*truncated - 2*b*q
    need((sign(wrong) != signs['far']) == (s >= 30), 'discrimination du modèle de troncature')
    norm_short = s+2 <= 30  # kCoordBits=32 : le premier terme du OU statique est faux.
    need(not norm_short or norm <= I64, 'branche courte sûre')
    # Boîte [0,far] entièrement dans le pavé : centre entier minimisant,
    # far maximisant, car q >= b/2 dans chaque axe ; lower<0 puis upper>0.
    need(q >= center[0] and values['center'] < 0 < values['far'], 'bornes de boîte')
    boundary = [2*m, 0, 0] if 2*m <= U32 else None
    return {'span': s, 'support': [list(a), list(end)], 'anchor': list(a),
            'center': [str(x) for x in center], 'radius_squared': str(radius2),
            'numerator': list(n), 'denominator': d, 'power_domain': domain,
            'guard_open': [-m, 2*m], 'short_norm': norm_short, 'lane': 'certified',
            'far': [q, q, q], 'one_square': q*q, 'norm': norm, 'norm_over_i64': norm > I64,
            'power_local': values['far'], 'signs': signs, 'box_signs': [-1, 1],
            'u32_guard_boundary': boundary, 'boundary_sign': 1 if boundary else None,
            'signed64_truncation_model_power': wrong}


def main():
    fixtures = [fixture(s) for s in (28, 29, 30, 32)]
    need(not fixtures[1]['short_norm'] and not fixtures[1]['norm_over_i64'], 's29 conservateur')
    need(fixtures[2]['one_square'] <= I64 < fixtures[2]['norm'], 's30 : somme seule')
    need(I64 < fixtures[3]['one_square'], 's32 : chaque carré')
    # Pour TOUS les points gardés de tout support d'étendue <=29 :
    # |v_j|<2^(s+1), donc ||v||²<3*2^60<2^63. Traduire ne change pas v.
    need(3*(1 << 60) < (1 << 63), 'borne universelle s<=29')
    result = {'native_executed': False, 'profile_required': 32, 'fixtures': fixtures,
              'target_spans': [30, 32], 'control_spans': [28, 29],
              's29_uniform_norm_upper_exclusive': 3*(1 << 60),
              'expected_ledger_one_guarded_query': {'native': 0, 'certified': 1, 'checked': 0, 'wide': 0,
                                                    'outside_sites': 0}}
    print(json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
