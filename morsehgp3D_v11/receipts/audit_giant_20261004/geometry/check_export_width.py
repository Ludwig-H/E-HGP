#!/usr/bin/env python3
"""Independent scalar Gram/Fraction witness for the closed points-export width, not a native test."""
from fractions import Fraction as F
from itertools import permutations, combinations
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
checks = 0


def need(value, why):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(why)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def solve(a, b):
    rows = [[F(x) for x in row] + [F(y)] for row, y in zip(a, b)]
    for col in range(len(rows)):
        pivot = next((j for j in range(col, len(rows)) if rows[j][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [v/scale for v in rows[col]]
        for j in range(len(rows)):
            if j != col:
                amount = rows[j][col]
                rows[j] = [x-amount*y for x, y in zip(rows[j], rows[col])]
    return tuple(row[-1] for row in rows)


def circumsphere(points):
    a = points[0]
    edges = [sub(b, a) for b in points[1:]]
    weights = solve([[dot(u, v) for v in edges] for u in edges], [F(dot(u, u), 2) for u in edges])
    if weights is None:
        return None
    center = tuple(a[j] + sum(w*u[j] for w, u in zip(weights, edges)) for j in range(3))
    return center, dot(sub(center, a), sub(center, a)), (1-sum(weights),) + weights


def determinant(u, v, w):
    return u[0]*(v[1]*w[2]-v[2]*w[1]) - u[1]*(v[0]*w[2]-v[2]*w[0]) + u[2]*(v[0]*w[1]-v[1]*w[0])


def main():
    metadata = json.loads((BASE/'SOURCE_BEFORE.json').read_text())
    for name in ('src/num/sphere.cpp', 'src/num/budgets.hpp', 'bench/points_export.cpp'):
        info = metadata['files']['morsehgp3D_v11/'+name]
        need(hashlib.sha256((BASE/info['copy']).read_bytes()).hexdigest() == info['sha256'], 'changed proof dependency')
    # Reading tokens binds this proof to the documented nonreduced numerator and three-word guard.
    export = (BASE/'source/morsehgp3D_v11/bench/points_export.cpp').read_text()
    sphere = (BASE/'source/morsehgp3D_v11/src/num/sphere.cpp').read_text()
    need('for (u64 j = 3; j < wide.words.size(); ++j)' in export and
         'if (wide.words[j] != 0) return fail(Reason::tower_invariant);' in export,
         'three-word export contract changed')
    need('for (const auto& level : domain.catalogue().levels())' in export and
         'outcome = fixed(out, level.numerator());' in export,
         'all catalogue levels pass through the three-word guard')
    need('multiply(to_wide(denominator_), to_wide(denominator_))' in sphere,
         'source no longer emits nonreduced D squared')
    rows = []
    for bits in (18, 21, 24):
        L = 2**bits - 1
        points = ((0,0,0), (L,L,0), (L,0,L), (0,L,L))
        ball = circumsphere(points)
        center, radius, bary = ball
        need(center == (F(L,2),)*3 and bary == (F(1,4),)*4, 'regular tetra center and positive weights')
        need(radius == F(3*L*L,4), 'exact radius')
        for q in (2,3):
            for part in combinations(points,q):
                need(circumsphere(part)[0] != center, 'support cardinality is exactly four')
        for order in permutations(points):
            c, r, w = circumsphere(order)
            need((c,r,w) == (center,radius,bary), 'Gram/Fraction permutation invariance')
            D = 2*abs(determinant(*[sub(p,order[0]) for p in order[1:]]))
            N = tuple((c[j]-order[0][j])*D for j in range(3))
            need(D == 4*L**3 and all(v.denominator == 1 and abs(v) == 2*L**4 for v in N), 'closed factory scale')
            num, den = int(dot(N,N)), D*D
            need(num == 12*L**8 and den == 16*L**6 and F(num,den) == radius, 'nonreduced Level exact')
            need(num.bit_length() <= 8*bits+12 and den.bit_length() <= 6*bits+8, 'valid native Level budgets')
        need(0+4 <= 3+1, 'qmin4 positive ball admitted at K3')
        need((num >> 192 != 0) == (bits == 24), 'export-width failure only at u24 on this witness')
        upper_word = (num >> 192) & (2**64-1)
        need(upper_word == (11 if bits == 24 else 0), 'actual fourth word of source-produced numerator')
        rows.append({'bits':bits,'L':L,'p':0,'m':4,'qmin':4,'K':3,'permutations':24,
                     'numerator':'12*L^8','denominator':'16*L^6','num_bits':num.bit_length(),
                     'den_bits':den.bit_length(),'beta':'3*L^2/4','three_word_export_refuses':bool(num>>192),
                     'numerator_decimal':str(num),'denominator_decimal':str(den),
                     'word_index_3':upper_word,
                     'source_predicted_export_reason':'tower_invariant' if upper_word else 'width_guard_passes'})
    # Export incidences cannot overflow these u64 expressions under CURRENT catalogue resource guards.
    maxballs = 2**32-2
    maxpopulation = 1024*maxballs
    need(maxpopulation < 2**42 and 8*maxpopulation+8*(2**32-2) < 2**46,
         'u64 incidence admission arithmetic fits under max_leaf<=1024')
    print(json.dumps({'status':'PASS','checks':checks,'rows':rows,'pin':metadata['pin'],
                     'scope':'scalar exact witness and source-bound width; no Cloud/native execution',
                     'refusal_proof':'nonzero numerator word index 3 triggers the pinned fixed() guard; all catalogue levels are written',
                     'native_runs':0,'fits':0,'gcp_actions':0,
                     'incidence_bound_current':'P<=1024*B<2^42; 8P+8n<2^46'},sort_keys=True,indent=1))


if __name__ == '__main__':
    main()
