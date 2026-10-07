"""Audit L06 : oracle de l'IDENTITE de composante de l'entree cover (la porte du depot ne juge que le niveau alpha).

Pour de petits nuages et chaque ordre k <= K : x entre a alpha_k(x)^2 = min des beta(F), F k-partie contenant x ; sa
composante a la coupe (a, fermee) est celle, dans Gamma_k(a), d'une k-partie minimale F. S'il existe plusieurs
k-parties minimales dans des composantes differentes de Gamma_k(alpha^2) (ex aequo), l'attache est ambigue a ce
niveau : on compte ces cas et on accepte toute composante candidate. On verifie :
  (1) niveau publie = alpha^2 exact ;
  (2) a chaque niveau critique (coupe fermee), la composante de la tour de chaque point entre est l'une de ses
      composantes candidates de l'oracle, et deux points non ambigus sont groupes ensemble ssi l'oracle les groupe.
Usage : python3 oracle_cover.py BUILD_DIR [nuages par famille] [graine] [n]
"""
import os
import random
import subprocess
import sys
import tempfile
from fractions import Fraction
from itertools import combinations

COPY = '/tmp/v11-audit/l06_code_tour/instr'
sys.path.insert(0, os.path.join(COPY, 'reference'))
import hgp10_ref as R  # noqa: E402


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
            if len(t) == 8:  # entree cover : x y z noeud r<rang> num den
                cur['points'].append(((int(t[1]), int(t[2]), int(t[3])), int(t[4]), Fraction(int(t[6]), int(t[7]))))
            else:            # K = 1 : entree core
                cur['points'].append(((int(t[1]), int(t[2]), int(t[3])), int(t[4]), Fraction(int(t[5]))))
    return orders


def top(nodes, v, a):
    while nodes[v][0] >= 0 and nodes[nodes[v][0]][1] <= a:
        v = nodes[v][0]
    return v


def cloud(kind, n, rnd):
    pts = set()
    while len(pts) < n:
        if kind == 'generique':
            pts.add(tuple(rnd.randint(0, 1000) for _ in range(3)))
        elif kind == 'grille':
            pts.add(tuple(rnd.randint(0, 3) for _ in range(3)))
        elif kind == 'coplanaire':
            pts.add((rnd.randint(0, 6), rnd.randint(0, 6), 0))
        elif kind == 'petite_grille':
            pts.add(tuple(rnd.randint(0, 12) for _ in range(3)))
        else:
            pts.add(tuple(rnd.choice((0, 2, 4)) + rnd.randint(0, 1) for _ in range(3)))
    P = sorted(pts)
    rnd.shuffle(P)
    return P


def check(exe, P, K, tmp, stats):
    src = os.path.join(tmp, 'in.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    dump = os.path.join(tmp, 'tower.txt')
    r = subprocess.run([exe, src, '--k=%d' % K, '--threads=2', '--entry=cover', '--dump=' + dump],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return 'code %d %s' % (r.returncode, r.stdout.strip()[:200])
    orders = parse(dump)
    idx = {p: i for i, p in enumerate(P)}
    n = len(P)
    for k in range(2, min(K, n) + 1):
        o = orders[k]
        nodes = o['nodes']
        beta = {F: R.meb(P, F)[0] for F in combinations(range(n), k)}
        cof = {G: R.meb(P, G)[0] for G in combinations(range(n), k + 1)} if k < n else {}
        alpha = [min(b for F, b in beta.items() if x in F) for x in range(n)]
        cand = [[F for F, b in beta.items() if x in F and b == alpha[x]] for x in range(n)]
        got = {}
        for pt, v, lv in o['points']:
            x = idx[pt]
            got[x] = v
            stats['levels'] += 1
            if lv != alpha[x]:
                return 'k=%d x=%d niveau %s contre alpha^2 %s' % (k, x, lv, alpha[x])
        levels = sorted(set(beta.values()) | set(cof.values()))
        parent = {}

        def find(F):
            while parent[F] != F:
                parent[F] = parent[parent[F]]
                F = parent[F]
            return F
        vs = sorted(beta.items(), key=lambda t: t[1])
        es = sorted(cof.items(), key=lambda t: t[1])
        iv = ie = 0
        for a in levels:
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
            # coupe fermee a
            groups = {}
            comps = {}
            for x in range(n):
                if alpha[x] > a:
                    continue
                c = {find(F) for F in cand[x]}
                comps[x] = c
                if alpha[x] == a and len(c) > 1:
                    stats['ambigus'].add((tuple(P), k, x))
                groups.setdefault(top(nodes, got[x], a), []).append(x)
            seen = {}
            for g, xs in groups.items():
                common = set.intersection(*[comps[x] for x in xs])
                stats['cuts'] += 1
                if not common:
                    return 'k=%d a=%s : groupe de la tour %s sans composante commune dans l\'oracle' % (k, a, xs)
                for x in xs:
                    if len(comps[x]) == 1:
                        c = next(iter(comps[x]))
                        if c in seen and seen[c] != g:
                            return 'k=%d a=%s : composante de l\'oracle scindee par la tour (points %s)' % (k, a, xs)
                        seen[c] = g
    return None


def main():
    exe = os.path.join(sys.argv[1], 'mhgp10_tower')
    per = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 20261002
    n = int(sys.argv[4]) if len(sys.argv) > 4 else 10
    rnd = random.Random(seed)
    stats = dict(levels=0, cuts=0, ambigus=set())
    fails = checks = 0
    with tempfile.TemporaryDirectory() as tmp:
        for kind in ('generique', 'grille', 'coplanaire', 'grappes', 'petite_grille'):
            for _ in range(per):
                P = cloud(kind, n, rnd)
                err = check(exe, P, 5, tmp, stats)
                checks += 1
                print('%s n=%d : %s' % (kind, len(P), err or 'conforme'), flush=True)
                if err:
                    fails += 1
                    print('  NUAGE', P)
    print('oracle_cover checks %d fails %d niveaux %d groupes %d attaches_ambigues %d' %
          (checks, fails, stats['levels'], stats['cuts'], len(stats['ambigus'])))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
