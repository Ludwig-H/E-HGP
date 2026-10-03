import sys, random, json, time
sys.dont_write_bytecode = True
import indep_full as I, indep_full_j as J
rng = random.Random(7)
bad = 0; cmpn = 0; t0 = time.time()
for it in range(300):
    n = rng.randint(4, 8); K = rng.choice([2, 3, 3, 4]);
    if K >= n: K = 2
    span = rng.choice([3, 5, 8, 20]); pts = set()
    while len(pts) < n:
        pts.add((rng.randint(0, span), rng.randint(0, span), rng.randint(0, span) if rng.random() < 0.6 else 0))
    P = sorted(pts)
    A, B = I.Full(P, K), J.FullJ(P, K)
    for m in (1, K + 1):
        for kap in (1, 2):
            ua = I.ultra_sq(A, I.rule_margin_sq(A, m, kap)); ub = I.ultra_sq(B, I.rule_margin_sq(B, m, kap))
            cmpn += 1; bad += ua != ub
    ua = I.ultra_sq(A, I.rule_first(A, 1)); ub = I.ultra_sq(B, I.rule_first(B, 1)); cmpn += 1; bad += ua != ub
    if sorted(A.h) != sorted(B.h): bad += 1
print(json.dumps({'nuages': 300, 'comparaisons_ultrametriques': cmpn, 'desaccords': bad, 'secondes': round(time.time() - t0, 1)}))
