"""Controles de la synthese (lecture seule du worktree, oracle de la definition) :
(F) factorisation : pour toute pendaison fidele, chaque bloc a la coupe fermee L est inclus dans l'amas discret de son
    noeud ; donc les blocs d'au moins mcs points de la projection sans mcs coincident avec ceux de la lecture
    'absorption' de la proposition de l'utilisateur (proprietaire calcule sur la tour condensee) ;
(Q) premiere couverture qualifiee a l'ordre k avec m = k+1 egale a la premiere couverture d'ordre k+1 (A_{k+1}).
Usage : python3 -B factorisation_check.py GRAINE NUAGES SECONDES"""
import os, sys, random, time, json
BENCH = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench'
sys.path.insert(0, BENCH)
sys.dont_write_bytecode = True
import points_reference as pr
from hgp11_ref import Definition

def popcount(x):
    return bin(x).count('1')

def main(seed, clouds, seconds):
    rng = random.Random(seed)
    t0 = time.time()
    stats = dict(nuages=0, couples=0, blocs=0, blocs_hors_amas=0, egalites_F=0, ecarts_F=0,
                 sites_Q=0, ecarts_Q=0)
    exemples = []
    for c in range(clouds):
        if time.time() - t0 > seconds:
            break
        n = rng.randint(4, 8)
        side = rng.choice([3, 4, 6, 10, 30])
        pts = set()
        while len(pts) < n:
            pts.add((rng.randint(0, side), rng.randint(0, side), rng.randint(0, side // 3 or 1)))
        pts = sorted(pts)
        stats['nuages'] += 1
        D = Definition(pts)
        for k in (2, 3):
            if k + 1 > n:
                continue
            res = D.order(k)
            res_up = D.order(k + 1)
            # coverage of each node at each closed cut
            cov = {}
            for cut in res.cuts:
                for v, coverage, _core in cut.closed:
                    cov[(cut.level, v)] = coverage
            levels = sorted(set(cut.level for cut in res.cuts))
            for m in (1, k + 1):
                ref, tree = pr.reference_rules(res, n, m)
                rules = ('core', 'cover', 'margin1') if m == 1 else ('first', 'margin')
                for rule in rules:
                    entries = ref[rule]
                    stats['couples'] += 1
                    u = pr.reference_ultrametric(entries, tree)
                    for L in levels:
                        blocks = pr.blocks_at(u, L)
                        hblocks = []
                        for B in blocks:
                            owners = set(tree.alive_ancestor(entries[i][1], L) for i in B)
                            if len(owners) != 1:
                                raise AssertionError('bloc a plusieurs noeuds')
                            v = owners.pop()
                            mask = cov.get((L, v), 0)
                            stats['blocs'] += 1
                            inside = all(mask >> i & 1 for i in B)
                            if not inside:
                                stats['blocs_hors_amas'] += 1
                                if len(exemples) < 5:
                                    exemples.append(dict(points=pts, k=k, m=m, rule=rule, level=str(L), bloc=sorted(B)))
                            hblocks.append((B, popcount(mask)))
                        for mcs in range(2, n + 1):
                            big = set(B for B, _ in hblocks if len(B) >= mcs)
                            absorb = set(B for B, size in hblocks if size >= mcs and len(B) >= mcs)
                            if big == absorb:
                                stats['egalites_F'] += 1
                            else:
                                stats['ecarts_F'] += 1
            # (Q) first qualified coverage at order k, m = k+1 vs first coverage at order k+1
            firstq = [None] * n
            for cut in res.cuts:
                for v, coverage, _core in cut.closed:
                    if popcount(coverage) >= k + 1:
                        for i in range(n):
                            if coverage >> i & 1 and (firstq[i] is None or cut.level < firstq[i]):
                                firstq[i] = cut.level
            firstup = [None] * n
            for cut in res_up.cuts:
                for v, coverage, _core in cut.closed:
                    for i in range(n):
                        if coverage >> i & 1 and (firstup[i] is None or cut.level < firstup[i]):
                            firstup[i] = cut.level
            for i in range(n):
                stats['sites_Q'] += 1
                if firstq[i] != firstup[i]:
                    stats['ecarts_Q'] += 1
                    if len(exemples) < 8:
                        exemples.append(dict(points=pts, k=k, site=i, qualifie=str(firstq[i]), ordre_sup=str(firstup[i])))
    stats['secondes'] = round(time.time() - t0, 1)
    print(json.dumps(dict(graine=seed, stats=stats, exemples=exemples), indent=1))

if __name__ == '__main__':
    main(int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3]))
