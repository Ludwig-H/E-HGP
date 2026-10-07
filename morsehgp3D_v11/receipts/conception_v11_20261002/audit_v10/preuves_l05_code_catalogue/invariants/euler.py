"""Identite d'Euler par ordre sur le catalogue a K + 2 (coquilles regulieres seulement) :
chi_k = [k = 1] n + somme sur q = 2..4, j = 1..q de (-1)^(q - j) C(q - 1, j - 1) N(q, p = k - j) = 1 pour k <= K
(la j-ieme statistique d'ordre de q distances a un lien inferieur equivalent a un bouquet de C(q-1, j-1) spheres).
Lit la sortie JSON de mhgp10_catalogue --k=K+2. Entrees sans doublon ; les coquilles etendues faussent le comptage naif."""
import json, sys
from math import comb
r = json.loads(open(sys.argv[1]).readline())
K2 = r['K']; n = r['sites']
N = {2: r['by_q_p']['q2'], 3: r['by_q_p']['q3'], 4: r['by_q_p']['q4']}
print('n=%d K+2=%d boules=%d etendues=%d' % (n, K2, r['balls'], r['extended']))
for k in range(1, K2 - 1):
    chi = n if k == 1 else 0
    for q in (2, 3, 4):
        for j in range(1, q + 1):
            p = k - j
            if p >= 0:
                chi += (-1) ** (q - j) * comb(q - 1, j - 1) * N[q][p]
    print('  ordre %d : chi = %d' % (k, chi))
