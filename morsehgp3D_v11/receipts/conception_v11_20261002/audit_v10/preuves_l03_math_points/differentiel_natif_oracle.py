#!/usr/bin/env python3
"""Audit L03 : differentiel entre le binaire v10 (afb081774) et l'oracle independant, pour les hierarchies de points
core et cover : suites des partitions (blocs d'au moins 2 points) a toutes les coupes fermees ou elles changent.
Oracle borne (n <= 11) : il etablit la verite sur de petits nuages ; aucune pente, aucune qualification d'echelle.
Les nuages ou un point a plusieurs composantes de premiere couverture au meme niveau sont comptes a part pour cover
(le produit y departage par indice : constat 04)."""
import sys, random, math, json
import oracle_l03 as O
import natif as N

def same(seq_o, seq_n):
    if len(seq_o) != len(seq_n):
        return False
    for (a, p), (b, q) in zip(seq_o, seq_n):
        if tuple(sorted(p)) != tuple(sorted(q)):
            return False
        fa = float(a)
        if abs(fa - b) > 1e-9 * max(1.0, abs(fa)):
            return False
    return True

def main():
    seed, trials, out = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
    rnd = random.Random(seed)
    stats = dict(nuages=0, cas_core=0, ecarts_core=0, cas_cover=0, ecarts_cover=0, cover_avec_egalites=0, cover_egalites_ecart=0, refus=0)
    ecarts = []
    for t in range(trials):
        n = rnd.randint(6, 11)
        box = rnd.choice([1000, 1000, 40, 6])          # generique, generique, peu degenere, tres degenere
        P = set()
        while len(P) < n:
            P.add((rnd.randrange(box), rnd.randrange(box), rnd.randrange(box)))
        P = sorted(P)
        stats['nuages'] += 1
        for K in (2, 3, 4):
            if K + 1 > n:
                continue
            T = O.Tour(P, K)
            for entry in ('core', 'cover'):
                res = N.run_cluster(P, K, entry, mcs=2)
                if res['code'] != 0:
                    stats['refus'] += 1
                    continue
                sn = N.suite(res['tree'], 2)
                if entry == 'core':
                    so = T.suite(O.core_projection(T), 2)
                    stats['cas_core'] += 1
                    if not same(so, sn):
                        stats['ecarts_core'] += 1
                        ecarts.append(['core', P, K])
                else:
                    ties = any(len(T.cover_tie_components(x)[1]) > 1 for x in range(T.n))
                    so = T.suite(O.cover_projection(T, 'min'), 2)
                    if ties:
                        stats['cover_avec_egalites'] += 1
                        if not same(so, sn):
                            stats['cover_egalites_ecart'] += 1
                    else:
                        stats['cas_cover'] += 1
                        if not same(so, sn):
                            stats['ecarts_cover'] += 1
                            ecarts.append(['cover', P, K])
        if (t + 1) % 25 == 0:
            print(t + 1, stats, flush=True)
    json.dump(dict(graine=seed, stats=stats, ecarts=ecarts[:20]), open(out, 'w'))
    print('FIN', stats)

if __name__ == '__main__':
    main()
