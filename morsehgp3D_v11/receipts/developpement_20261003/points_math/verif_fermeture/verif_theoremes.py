#!/usr/bin/env python3
"""Verification adverse des enonces T1-T4, minmax, identite (k,k+1)=(k+1,k+1), m<=k, et contre-controles, avec
le seul oracle independant vf_oracle (aucun code du rapport ni de la reference).

    PYTHONDONTWRITEBYTECODE=1 python3 verif_theoremes.py --clouds 300 --seed 4242 --seconds 300

Generateurs propres (graines differentes du rapport) : 'ties' (grille, ex aequo), 'generic' (uniforme), 'blobs'
(deux amas + vallee). Pour chaque (nuage, k <= 3 ou 4, m de 1 a k+2) :
  T1  : m <= k+1, kp = max(k,m) >= 2 : fermeture == liaison simple de w_kp (MEB des kp-parties seules), diag A_kp ;
  T2  : w_kp <= mr^2 <= 4 w_kp hors diagonale, A_kp <= D_kp <= 4 A_kp, et u <= SL(mr^2) <= 4 u coefficient par
        coefficient (diagonale comprise) ;
  T3  : w <= 4 u hors diagonale ; chaque bloc au niveau a dans UNE couverture qualifiee au niveau 4a ; on releve
        max w/u (atteinte de 4 ?) ;
  T4  : pour core, cover, first_m, H_1, H_m, EC_m : w_cible <= u_F <= 4 max(e_i, e_j, w_cible) ;
  MM  : u == minmax(w) hors diagonale ; e_i = min_j w(i,j) si m >= 2 ;
  ID  : (k,k+1) == (k+1,k+1) (matrice complete) ; (k,m) == (k,1) pour m <= k ; on compte les cas ou
        (k,k+2) != (k+1,k+2).
"""
import argparse
from fractions import Fraction
import json
import random
import sys
import time

sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo  # noqa: E402


def gen_ties(rng):
    n = rng.randint(4, 9)
    side = rng.choice([3, 4, 5, 8])
    sc = rng.choice([1, 3, 100])
    s = set()
    while len(s) < n:
        s.add((rng.randrange(side) * sc, rng.randrange(side) * sc, rng.choice([0, rng.randrange(side) * sc])))
    s = sorted(s)
    rng.shuffle(s)
    return s


def gen_generic(rng):
    n = rng.randint(5, 9)
    s = set()
    while len(s) < n:
        s.add(tuple(rng.randrange(2000) for _ in range(3)))
    s = sorted(s)
    rng.shuffle(s)
    return s


def gen_blobs(rng):
    na, nb, nv = rng.choice([3, 4]), rng.choice([3, 4]), rng.choice([1, 2])
    t = rng.randint(50, 250)
    big = rng.randint(700, 1600)
    s = set()
    pts = []

    def add(p):
        if p not in s:
            s.add(p)
            pts.append(p)
            return True
        return False
    c = 0
    while c < na:
        c += add((rng.randrange(t), rng.randrange(t), rng.randrange(t)))
    c = 0
    while c < nb:
        c += add((big + rng.randrange(t), rng.randrange(t), rng.randrange(t)))
    c = 0
    while c < nv:
        c += add((rng.randrange(t, big), rng.randrange(-t, 2 * t), rng.randrange(-t, 2 * t)))
    return pts


class Tally(object):
    def __init__(self):
        self.c, self.f, self.ex = {}, {}, []
        self.diag = {}

    def check(self, name, ok, info=None):
        self.c[name] = self.c.get(name, 0) + 1
        if not ok:
            self.f[name] = self.f.get(name, 0) + 1
            if len(self.ex) < 15:
                self.ex.append((name, info))

    def mx(self, name, val):
        if name not in self.diag or val > self.diag[name][0]:
            self.diag[name] = (val, None)


def analyse(pts, T):
    n = len(pts)
    cl = vo.Cloud(pts)
    kmax = min(4 if n <= 7 else 3, n - 1)
    fulls = {}

    def full(k):
        if k not in fulls:
            fulls[k] = vo.Full(cl, k)
        return fulls[k]
    clos = {}

    def closure(k, m):
        if (k, m) not in clos:
            clos[(k, m)] = full(k).closure(m)
        return clos[(k, m)]
    for k in range(1, kmax + 1):
        F = full(k)
        for m in range(1, min(k + 2, n) + 1):
            info = dict(pts=pts, k=k, m=m)
            u, w = closure(k, m)
            off = [(i, j) for i in range(n) for j in range(n) if i != j]
            # MM
            mm = vo.minmax(w, n)
            ok = all(u[i][j] == mm[i][j] for i, j in off) and all(u[i][i] == w[i][i] for i in range(n))
            if m >= 2:
                ok = ok and all(u[i][i] == min(w[i][j] for j in range(n) if j != i) for i in range(n))
            T.check('MM_sous_dominante', ok, info)
            # ID
            if m <= k:
                T.check('ID_m_le_k', u == closure(k, 1)[0], info)
            if m == k + 1 and k + 1 <= n - 1:
                T.check('ID_k_kplus1', u == closure(k + 1, k + 1)[0], info)
            if m == k + 2 and k + 1 <= n - 1:
                T.check('CONTRE_k_kplus2_egal', u == closure(k + 1, k + 2)[0], info)
            # T1, T2
            kp = max(k, m)
            if m <= k + 1 and kp >= 2:
                wk = vo.w_direct(cl, kp)
                hl = vo.minmax(wk, n)
                A = [min(wk[i][j] for j in range(n) if j != i) for i in range(n)]
                ok = all(u[i][j] == hl[i][j] for i, j in off) and all(u[i][i] == A[i] for i in range(n))
                T.check('T1_forme_directe', ok, info)
                if m == k + 1 and k >= 2:
                    wk0 = vo.w_direct(cl, k)
                    hl0 = vo.minmax(wk0, n)
                    T.check('CONTRE_T1_ordre_k', all(u[i][j] == hl0[i][j] for i, j in off), info)
                Dk = [cl.Dk(i, kp) for i in range(n)]
                mr = [[max(Dk[i], Dk[j], cl.d2(i, j)) if i != j else Dk[i] for j in range(n)] for i in range(n)]
                ok = all(wk[i][j] <= mr[i][j] <= 4 * wk[i][j] for i, j in off)
                ok = ok and all(A[i] <= Dk[i] <= 4 * A[i] for i in range(n))
                T.check('T2_encadrement_paires', ok, info)
                umr = vo.minmax(mr, n)
                ok = all(u[i][j] <= umr[i][j] <= 4 * u[i][j] for i in range(n) for j in range(n))
                T.check('T2_encadrement_liaison', ok, info)
                T.check('CONTRE_T2_facteur1', all(umr[i][j] <= u[i][j] for i in range(n) for j in range(n)), info)
                r = max(umr[i][j] / u[i][j] for i in range(n) for j in range(n) if u[i][j] > 0)
                T.mx('T2_max_umr_sur_u', float(r))
            # T3
            ok = all(w[i][j] <= 4 * u[i][j] for i, j in off)
            T.check('T3_w_le_4u', ok, info)
            ratios = [w[i][j] / u[i][j] for i, j in off if u[i][j] > 0]
            if ratios:
                T.mx('T3_max_w_sur_u', float(max(ratios)))
            g = True
            for a, snap in F.snaps:
                four = F.snap_at(4 * a)[1]
                covs = [cov for _v, cov in four if vo.popcount(cov) >= m]
                for b in vo.blocks(u, a):
                    bm = sum(1 << i for i in b)
                    if not any(bm & ~cv == 0 for cv in covs):
                        g = False
            T.check('T3_bloc_dans_une_couverture_4a', g, info)
            g = True
            for a, snap in F.snaps:
                covs = [cov for _v, cov in snap]
                for b in vo.blocks(u, a):
                    bm = sum(1 << i for i in b)
                    if not any(bm & ~cv == 0 for cv in covs):
                        g = False
            T.check('INFO_fermeture_fidele_faible', g, info)
            # T4
            w1 = closure(k, 1)[1]
            rules = dict(core=(F.hang_core(), w1), cover=(F.hang_first(1), w1), first=(F.hang_first(m), w),
                         H1=(F.hang_margin(1), w1), Hm=(F.hang_margin(m), w), EC=(F.hang_ec(m), w))
            for name, (ent, tw) in rules.items():
                uf = F.ultra(ent)
                lo = all(uf[i][j] >= tw[i][j] for i, j in off)
                hi = all(uf[i][j] <= 4 * max(uf[i][i], uf[j][j], tw[i][j]) for i, j in off)
                T.check('T4_bas_' + name, lo, info)
                T.check('T4_haut_' + name, hi, info)
                rr = max(uf[i][j] / max(uf[i][i], uf[j][j], tw[i][j]) for i, j in off)
                T.mx('T4_max_ratio_' + name, float(rr))
                # le rapport aux entrees de la FERMETURE (et non a celles de la regle) : T4 est-il vrai avec e_CL ?
                hi2 = all(uf[i][j] <= 4 * max(u[i][i], u[j][j], tw[i][j]) for i, j in off) if name in (
                    'first', 'Hm', 'EC') else True
                T.check('INFO_T4_avec_entrees_fermeture_' + name, hi2, info)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--clouds', type=int, default=300)
    ap.add_argument('--seed', type=int, default=4242)
    ap.add_argument('--seconds', type=float, default=300)
    ap.add_argument('--out', default='resultats/theoremes.json')
    a = ap.parse_args()
    rng = random.Random(a.seed)
    T = Tally()
    t0 = time.time()
    done = dict(ties=0, generic=0, blobs=0)
    for c in range(a.clouds):
        if time.time() - t0 > a.seconds:
            break
        kind = ('ties', 'generic', 'blobs')[c % 3]
        pts = dict(ties=gen_ties, generic=gen_generic, blobs=gen_blobs)[kind](rng)
        analyse(pts, T)
        done[kind] += 1
    el = time.time() - t0
    print('nuages %s en %.1f s (graine %d)' % (done, el, a.seed))
    for name in sorted(T.c):
        print('%-45s controles %6d  echecs %5d' % (name, T.c[name], T.f.get(name, 0)))
    for name in sorted(T.diag):
        print('diagnostic %-35s max %.4f' % (name, T.diag[name][0]))
    for name, info in T.ex[:8]:
        print('exemple', name, info)
    with open(a.out, 'w') as fh:
        json.dump(dict(clouds=done, seconds=round(el, 1), checks=T.c, failures=T.f,
                       diag={k: v[0] for k, v in T.diag.items()},
                       examples=[(n_, str(i)) for n_, i in T.ex]), fh, indent=1, sort_keys=True)


if __name__ == '__main__':
    main()
