# Second controle : coquilles plus larges (m = 10 a 13) contre la definition brute, puis cout du quotient polynomial
# sur les coquilles completes de 24 et 30 points cospheriques et de 12, 16, 24 points cocycliques (sans juge brut).
import itertools, random, sys, time
import quotient_check as Q

def big_shells():
    s5 = sorted(set(p for a in (2, -2) for b in (1, -1) for p in itertools.permutations((a, b, 0))))
    s9 = sorted(set([p for a in (3, -3) for p in itertools.permutations((a, 0, 0))] +
                    [p for a in (2, -2) for b in (2, -2) for c in (1, -1) for p in itertools.permutations((a, b, c))]))
    c25 = [(x, y, 0) for x in range(-5, 6) for y in range(-5, 6) if x*x + y*y == 25]
    c65 = [(x, y, 0) for x in range(-9, 10) for y in range(-9, 10) if x*x + y*y == 65]
    c325 = [(x, y, 0) for x in range(-19, 20) for y in range(-19, 20) if x*x + y*y == 325]
    return s5, s9, c25, c65, c325

def main():
    s5, s9, c25, c65, c325 = big_shells()
    rng = random.Random(11)
    checks = fails = 0
    for name, pool, sizes, reps in (('s5', s5, (10, 11, 12), 3), ('s9', s9, (10, 12), 2), ('c25', c25, (10, 12), 2),
                                    ('c65', c65, (12, 13), 2)):
        for sz in sizes:
            for _ in range(reps):
                U = rng.sample(pool, sz)
                S = Q.maximal_separable(U)
                for t in range(1, sz + 1):
                    b = Q.brute_pieces(U, t)
                    d = Q.design_pieces(U, t, S)
                    checks += 1
                    if b != d:
                        fails += 1
                        print('ECART', name, sz, t)
    print('quotient_large_checks', checks, 'fails', fails)
    # cout du quotient polynomial seul (comptes de composantes par t), coquilles completes
    for name, U in (('sphere_24', s5), ('sphere_30', s9), ('cercle_12', c25), ('cercle_16', c65), ('cercle_24', c325)):
        t0 = time.process_time()
        S = Q.maximal_separable(U)
        m = len(U)
        res = []
        for t in range(1, m + 1):
            alive = [M for M in S if bin(M).count('1') >= t]
            if not alive:
                res.append('N')
                continue
            par = list(range(len(alive)))
            def find(x):
                while par[x] != x:
                    par[x] = par[par[x]]
                    x = par[x]
                return x
            for a in range(len(alive)):
                for b in range(a + 1, len(alive)):
                    if bin(alive[a] & alive[b]).count('1') >= t:
                        ra, rb = find(a), find(b)
                        if ra != rb:
                            par[ra] = rb
            res.append(str(len(set(find(a) for a in range(len(alive))))))
        dt = time.process_time() - t0
        print(name, 'm', m, 'ensembles', len(S), 'morceaux_par_t', ','.join(res), 'cpu_s_python', round(dt, 3))
    return 1 if fails else 0

if __name__ == '__main__':
    sys.exit(main())
