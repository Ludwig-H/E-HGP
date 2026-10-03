#!/usr/bin/env python3
"""Auto-controle de indep_full.py : MEB connues, fixture {0,2,4}, puis recoupe aleatoire contre l'oracle v11
(hgp11_ref + points_reference, utilises ici seulement comme temoin de MON code)."""
import sys, random, time, json
sys.dont_write_bytecode = True
from fractions import Fraction as Fr
import indep_full as I

# MEB connues
assert_ = I.exiger
assert_(I.meb2([(0,0,0),(2,0,0)]) == 1, 'meb paire')
assert_(I.meb2([(0,0,0),(2,0,0),(1,1,0)]) == 1, 'meb triangle droit')
assert_(I.meb2([(0,0,0),(2,0,0),(1,5,0)]) == Fr(169,25), 'meb triangle aigu')  # centre (1, 2.4) r=2.6
assert_(I.meb2([(1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1)]) == 3, 'meb tetraedre regulier')
# {0,2,4} a K = 2 : median couvert a 1 par deux lentilles, fusion a 4
T = I.Full([(0,0,0),(2,0,0),(4,0,0)], 2)
print('levels', T.h, 'children', T.children, 'cov median', T.cov[1])
e1 = I.rule_margin_sq(T, 1)
print('H1 {0,2,4}', e1)

sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference')
import points_reference as PR
from hgp11_ref import Definition

def oracle_u(P, K, m, rule):
    res = Definition(list(P)).order(K)
    ref, tree = PR.reference_rules(res, len(P), m)
    return PR.reference_ultrametric(ref[rule], tree), res

rng = random.Random(20261003)
t0 = time.time()
stats = {'nuages': 0, 'comparaisons': 0, 'desaccords': 0, 'niveaux_noeuds_diff': 0}
ex = []
for it in range(int(sys.argv[1]) if len(sys.argv) > 1 else 60):
    n = rng.randint(4, 7)
    K = rng.choice([2, 2, 3])
    if K >= n: K = 2
    span = rng.choice([4, 6, 10, 30])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randint(0, span), rng.randint(0, span), rng.randint(0, span) if rng.random() < 0.6 else 0))
    P = sorted(pts)
    T = I.Full(P, K)
    for m, rule in ((1, 'margin1'), (K + 1, 'margin'), (1, 'cover'), (K + 1, 'first'), (1, 'core')):
        uo, res = oracle_u(P, K, m, rule)
        if rule == 'margin1' or rule == 'margin':
            mine = I.ultra_sq(T, I.rule_margin_sq(T, m))
        elif rule == 'core':
            mine = I.ultra_sq(T, I.rule_core(T))
        else:
            mine = I.ultra_sq(T, I.rule_first(T, m))
        for i in range(n):
            for j in range(n):
                stats['comparaisons'] += 1
                if uo[i][j] != mine[i][j]:
                    stats['desaccords'] += 1
                    if len(ex) < 5:
                        ex.append({'P': P, 'K': K, 'rule': rule, 'i': i, 'j': j, 'oracle': str(uo[i][j]), 'mine': str(mine[i][j])})
    lv_o = sorted(nd.level for nd in res.nodes)
    if lv_o != sorted(T.h):
        stats['niveaux_noeuds_diff'] += 1
        if len(ex) < 8:
            ex.append({'P': P, 'K': K, 'nodes_oracle': [str(x) for x in lv_o], 'nodes_mine': [str(x) for x in sorted(T.h)]})
    stats['nuages'] += 1
stats['secondes'] = round(time.time() - t0, 1)
print(json.dumps(stats))
for e in ex: print(json.dumps(e))
json.dump({'stats': stats, 'exemples': ex}, open('recus/test_indep.json', 'w'), indent=1)
