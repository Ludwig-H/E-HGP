"""Porte T2 de la tour : forets C++ et attaches C n X == oracle exhaustif Gamma_k (reference/hgp10_ref.py).

Pour chaque nuage (generiques, grilles cospheriques, coplanaires), chaque ordre k <= K et chaque niveau critique a
de Gamma_k, coupes fermee (<= a) et ouverte (< a) :
  - nombre de composantes de la foret == nombre de composantes de Gamma_k(a) ;
  - partition C n X des points entres == partition de l'oracle (composante de Gamma_k(a) du sommet kNN(x)).
Usage : python3 test_tower_oracle.py BUILD_DIR [nuages]   (0 conforme, 1 desaccord, 3 plancher)
"""
import os
import random
import subprocess
import sys
import tempfile
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
sys.path.insert(0, HERE)
import hgp10_ref as R  # noqa: E402
from test_catalogue_oracle import clouds  # noqa: E402


def parse(path):
    orders = {}
    cur = None
    for line in open(path):
        t = line.split()
        if t[0] == 'order':
            cur = dict(nodes=[], points=[])
            orders[int(t[1])] = cur
        elif t[0] == 'node':
            cur['nodes'].append((int(t[2]), Fraction(int(t[3]), int(t[4]))))
        else:
            cur['points'].append(((int(t[1]), int(t[2]), int(t[3])), int(t[4]), Fraction(int(t[5]))))
    return orders


def gamma_sweep(P, k):
    """Oracle Gamma_k en un balayage : par niveau critique a, (nb composantes, partition C n X) ouverte et fermee."""
    from itertools import combinations
    n = len(P)
    beta = {F: R.meb(P, F)[0] for F in combinations(range(n), k)}
    cof = {G: R.meb(P, G)[0] for G in combinations(range(n), k + 1)} if k < n else {}
    levels = sorted(set(beta.values()) | set(cof.values()))
    knn = [R.knn_vertex(P, x, k) for x in range(n)]
    entry = [R.entry_level(P, x, k) for x in range(n)]
    parent = {}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    vs = sorted(beta.items(), key=lambda t: t[1])
    es = sorted(cof.items(), key=lambda t: t[1])
    iv = ie = 0
    out = []

    def snap(a, closed):
        roots = {find(F) for F in parent}
        groups = {}
        for x in range(n):
            if entry[x] < a or (closed and entry[x] == a):
                groups.setdefault(find(knn[x]), set()).add(x)
        return len(roots), sorted((frozenset(g) for g in groups.values()), key=lambda t: sorted(t))
    for a in levels:
        op = snap(a, False)
        while iv < len(vs) and vs[iv][1] == a:
            parent[vs[iv][0]] = vs[iv][0]
            iv += 1
        while ie < len(es) and es[ie][1] == a:
            G = es[ie][0]
            fs = [tuple(y for y in G if y != u) for u in G]
            for f in fs[1:]:
                x, y = find(fs[0]), find(f)
                if x != y:
                    parent[max(x, y)] = min(x, y)
            ie += 1
        out.append((a, op, snap(a, True)))
    return out


def top(nodes, v, a, closed):
    ok = (lambda l: l <= a) if closed else (lambda l: l < a)
    while nodes[v][0] >= 0 and ok(nodes[nodes[v][0]][1]):
        v = nodes[v][0]
    return v


def check(exe, P, K, tmp):
    src = os.path.join(tmp, 'in.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    dump = os.path.join(tmp, 'tower.txt')
    r = subprocess.run([exe, src, '--k=%d' % K, '--threads=2', '--dump=' + dump], capture_output=True, text=True)
    if r.returncode != 0:
        return 'code %d %s' % (r.returncode, r.stdout.strip()), 0
    orders = parse(dump)
    idx = {p: i for i, p in enumerate(P)}
    cuts = 0
    for k in range(1, min(K, len(P)) + 1):
        o = orders[k]
        nodes = o['nodes']
        for a, op, cl in gamma_sweep(P, k):
            for is_closed, (want_n, want_part) in ((True, cl), (False, op)):
                ok = (lambda l: l <= a) if is_closed else (lambda l: l < a)
                alive = sum(1 for v, (par, lv) in enumerate(nodes)
                            if ok(lv) and (par < 0 or not ok(nodes[par][1])))
                if alive != want_n:
                    return 'k=%d a=%s ferme=%s composantes %d contre %d' % (k, a, is_closed, alive, want_n), cuts
                groups = {}
                for pt, v, e in o['points']:
                    if ok(e):
                        groups.setdefault(top(nodes, v, a, is_closed), set()).add(idx[pt])
                mine = sorted((frozenset(g) for g in groups.values()), key=lambda s: sorted(s))
                if mine != want_part:
                    return 'k=%d a=%s ferme=%s partition C n X' % (k, a, is_closed), cuts
                cuts += 1
    return None, cuts


def main():
    exe = os.path.join(sys.argv[1], 'mhgp10_tower')
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 24
    rnd = random.Random(20260929)
    checks = fails = cuts = 0
    with tempfile.TemporaryDirectory() as tmp:
        for P in clouds(count, rnd):
            P = P[:12]
            for K in (1, 3, 5):
                err, c = check(exe, P, K, tmp)
                checks += 1
                cuts += c
                if err:
                    fails += 1
                    if fails <= 5:
                        print('ECART K=%d n=%d : %s\n  %s' % (K, len(P), err, P))
    E5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
    with tempfile.TemporaryDirectory() as tmp:
        err, c = check(exe, E5, 4, tmp)
        checks += 1
        cuts += c
        if err:
            fails += 1
            print('ECART E5 : %s' % err)
    print('tower_oracle_checks %d fails %d cuts %d' % (checks, fails, cuts))
    if fails:
        return 1
    return 3 if cuts < 500 else 0


if __name__ == '__main__':
    sys.exit(main())
