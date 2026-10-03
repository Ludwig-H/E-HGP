#!/usr/bin/env python3
"""Verifie, sur nuages aleatoires bornes (n <= 9, k <= 4), les garanties de la fermeture qualifiee et les
identites structurelles demontrees dans RAPPORT.md (section 2).

    python3 verif_garanties.py --clouds 240 --seed 20261003 --out resultats/garanties.json

Controles par (nuage, k, m), m de 1 a k + 2 :
  G1 laminarite : partition recalculee coupe par coupe (sans memoire) raffine la suivante, egale a la cumulee ;
  G2 minmax : hors diagonale u = sous-dominante de w ; entrees = w[i][i] ; u <= w ;
  G3 m <= k ne change rien ; G4 identite (k, k+1) = (k+1, k+1) (blocs, entrees, hauteurs) ;
  G5 forme directe : pour m <= k + 1 et kp = max(k, m) >= 2, fermeture = liaison simple de w_kp (aucune connexite
     de FULL n'intervient) ;
  G6 encadrement HDBSCAN : w_kp <= mr_kp^2 <= 4 w_kp sur toute paire, A_kp <= D_kp <= 4 A_kp ;
  G7 dualite : toute pendaison fidele (core, cover, first, H_1, H_m, fermeture exclusive) verifie u >= w ;
  G8 fermeture exclusive fidele : chaque bloc a chaque coupe est inclus dans une couverture vivante ;
  G9 equivariance : permutation des identifiants et isometrie entiere (rotation, reflexion, translation).
Tout ecart est compte et les premiers sont conserves dans le JSON.
"""
import argparse
from fractions import Fraction
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402


DIAG = {}


def offdiag_equal(a, b, n):
    return all(a[i][j] == b[i][j] for i in range(n) for j in range(n) if i != j)


def full_equal(a, b, n):
    return all(a[i][j] == b[i][j] for i in range(n) for j in range(n))


class Tally(object):
    def __init__(self):
        self.count = {}
        self.fail = {}
        self.examples = []

    def check(self, name, ok, info=None):
        self.count[name] = self.count.get(name, 0) + 1
        if not ok:
            self.fail[name] = self.fail.get(name, 0) + 1
            if len(self.examples) < 20 and not name.startswith('M'):
                self.examples.append(dict(check=name, info=info))


def analyse_cloud(pts, tally, rng, equiv):
    n = len(pts)
    defn = lf.Definition(pts)
    kmax = min(4, n - 1)
    cache = {}

    def clo(k, m):
        if (k, m) not in cache:
            cache[(k, m)] = lf.closure(defn.order(k), n, m)
        return cache[(k, m)]
    for k in range(1, kmax + 1):
        res = defn.order(k)
        for m in range(1, min(k + 2, n) + 1):
            info = dict(points=pts, k=k, m=m)
            c = clo(k, m)
            u, w = c['u'], c['w']
            # G1
            parts = [lf.partition_of_cut(cut, n, m) for cut in res.cuts]
            lam = all(lf.refines(parts[x], parts[x + 1]) for x in range(len(parts) - 1))
            same = all(parts[x] == lf.blocks_from_u(u, cut.level) for x, cut in enumerate(res.cuts))
            tally.check('G1_laminarite', lam and same, info)
            # G2
            mm = lf.minmax_closure(w, n)
            ent_ok = all(u[i][i] == w[i][i] for i in range(n))
            if m >= 2:
                ent_ok = ent_ok and all(u[i][i] == x for i, x in enumerate(lf.offdiag_min(w, n)))
            dom = all(u[i][j] <= w[i][j] for i in range(n) for j in range(n))
            tally.check('G2_minmax', offdiag_equal(u, mm, n) and ent_ok and dom, info)
            # G3
            if m <= k:
                tally.check('G3_m_le_k', full_equal(u, clo(k, 1)['u'], n), info)
            # G4
            if m == k + 1 and k + 1 <= n:
                tally.check('G4_identite_k_kplus1', full_equal(u, clo(k + 1, k + 1)['u'], n), info)
            # G5, G6
            kp = max(k, m)
            if m <= k + 1 and kp >= 2:
                uh, wkp = lf.hl_direct(defn, n, kp)
                tally.check('G5_forme_directe_HL', full_equal(u, uh, n), info)
                mr, dk = lf.mr2(defn, n, kp)
                sand = all(wkp[i][j] <= mr[i][j] <= 4 * wkp[i][j] for i in range(n) for j in range(n) if i != j)
                sand = sand and all(wkp[i][i] <= dk[i] <= 4 * wkp[i][i] for i in range(n))
                tally.check('G6_encadrement_HDBSCAN', sand, info)
            # G10 : w <= 4 u (rayon : co-couverture au plus 2 fois apres la reunion de la fermeture)
            tally.check('G10_w_le_4u', all(w[i][j] <= 4 * u[i][j] for i in range(n) for j in range(n) if i != j),
                        info)
            # G11 : chaque bloc de la fermeture au niveau a est inclus dans UNE couverture FULL au niveau 4a
            g11 = True
            for cut in res.cuts:
                four = 4 * cut.level
                later = None
                for c2 in res.cuts:
                    if c2.level <= four:
                        later = c2
                    else:
                        break
                covers = [cov for (_v, cov, _c) in later.closed]
                for b in lf.pr.blocks_at(u, cut.level):
                    bm = 0
                    for i in b:
                        bm |= 1 << i
                    if not any(bm & ~cov == 0 for cov in covers):
                        g11 = False
            tally.check('G11_bloc_r_dans_couverture_2r', g11, info)
            refm, treem = lf.faithful_rules(res, n, m)
            for rule in ('margin', 'margin1', 'first', 'cover', 'core'):
                ur = lf.ultrametric_of(refm[rule], treem)
                tw = w if rule in ('first', 'margin') else clo(k, 1)['w']
                ratio = max(ur[i][j] / max(ur[i][i], ur[j][j], tw[i][j])
                            for i in range(n) for j in range(n) if i != j)
                DIAG[rule] = max(DIAG.get(rule, 0), ratio)
                if rule == 'first':
                    tally.check('M7_fidele_facteur_3_first',
                                all(ur[i][j] <= 3 * max(ur[i][i], ur[j][j], tw[i][j])
                                    for i in range(n) for j in range(n) if i != j))
            tally.check('M6_facteur_1.9', all(100 * w[i][j] <= 361 * u[i][j] for i in range(n) for j in range(n)
                                              if i != j))
            # G7, G8
            ref, tree = lf.faithful_rules(res, n, m)
            w1 = clo(k, 1)['w']
            for rule in ('core', 'cover', 'margin1', 'first', 'margin'):
                ur = lf.ultrametric_of(ref[rule], tree)
                target = w if rule in ('first', 'margin') else w1
                tally.check('G7_fidele_ge_w_' + rule,
                            all(ur[i][j] >= target[i][j] for i in range(n) for j in range(n) if i != j), info)
            for rule in ('core', 'cover', 'margin1', 'first', 'margin'):
                ur = lf.ultrametric_of(ref[rule], tree)
                target = w if rule in ('first', 'margin') else w1
                tally.check('G12_fidele_le_4max_' + rule,
                            all(ur[i][j] <= 4 * max(ur[i][i], ur[j][j], target[i][j])
                                for i in range(n) for j in range(n) if i != j), info)
            ec = lf.ec_hanging(res, n, m)
            uec = lf.ultrametric_of(ec, tree)
            tally.check('G12_fidele_le_4max_ec', all(uec[i][j] <= 4 * max(uec[i][i], uec[j][j], w[i][j])
                                                    for i in range(n) for j in range(n) if i != j), info)
            tally.check('G7_fidele_ge_w_ec',
                        all(uec[i][j] >= w[i][j] for i in range(n) for j in range(n) if i != j), info)
            faithful = True
            for cut in res.cuts:
                covers = [cov for (_v, cov, _c) in cut.closed]
                for b in lf.pr.blocks_at(uec, cut.level):
                    bm = 0
                    for i in b:
                        bm |= 1 << i
                    if not any(bm & ~cov == 0 for cov in covers):
                        faithful = False
            tally.check('G8_ec_bloc_dans_une_couverture', faithful, info)
    # Temoins negatifs (mutants) : chacun doit echouer au moins une fois sur la campagne.
    for k in range(1, kmax + 1):
        res = defn.order(k)
        for m in range(1, min(k + 2, n) + 1):
            c = clo(k, m)
            u, w = c['u'], c['w']
            if m == k + 1 and k >= 2:
                uh, _ = lf.hl_direct(defn, n, max(k, m - 1))
                tally.check('M1_mauvais_ordre_HL(kp=k)', full_equal(u, uh, n))
            tally.check('M2_fermeture_ge_w', all(u[i][j] >= w[i][j] for i in range(n) for j in range(n) if i != j))
            kp = max(k, m)
            if m <= k + 1 and kp >= 2:
                mr, _ = lf.mr2(defn, n, kp)
                wkp = lf.w_direct(defn, n, kp)
                tally.check('M3_encadrement_facteur_1',
                            all(mr[i][j] <= wkp[i][j] for i in range(n) for j in range(n) if i != j))
            faithful = True
            for cut in res.cuts:
                covers = [cov for (_v, cov, _c) in cut.closed]
                for b in lf.pr.blocks_at(u, cut.level):
                    bm = 0
                    for i in b:
                        bm |= 1 << i
                    if not any(bm & ~cov == 0 for cov in covers):
                        faithful = False
            tally.check('M4_fermeture_bloc_dans_une_couverture', faithful)
            if m == k + 2 and k + 1 <= n - 1:
                tally.check('M5_identite_k_kplus2', full_equal(u, clo(k + 1, k + 2)['u'], n))
    if equiv:
        perm = list(range(n))
        rng.shuffle(perm)
        pp = [pts[perm[j]] for j in range(n)]
        iso = [(-y + 5000, x + 7, -z + 3000) for (x, y, z) in pts]
        dp, di = lf.Definition(pp), lf.Definition(iso)
        for k in range(1, kmax + 1):
            for m in sorted(set([1, k + 1, k + 2])):
                if m > n:
                    continue
                u = clo(k, m)['u']
                up = lf.closure(dp.order(k), n, m)['u']
                ui = lf.closure(di.order(k), n, m)['u']
                okp = all(up[a][b] == u[perm[a]][perm[b]] for a in range(n) for b in range(n))
                oki = full_equal(ui, u, n)
                tally.check('G9_equivariance', okp and oki, dict(points=pts, k=k, m=m))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clouds', type=int, default=240)
    parser.add_argument('--seed', type=int, default=20261003)
    parser.add_argument('--seconds', type=float, default=240.0)
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    rng = random.Random(args.seed)
    tally = Tally()
    started = time.monotonic()
    done = dict(gate=0, generic=0, two_blobs=0)
    for c in range(args.clouds):
        if time.monotonic() - started > args.seconds:
            break
        kind = ('gate', 'generic', 'two_blobs')[c % 3]
        if kind == 'gate':
            pts = lf.cloud_gate(rng)
        elif kind == 'generic':
            pts = lf.cloud_generic(rng)
        else:
            pts = lf.cloud_two_blobs(rng)[0]
        analyse_cloud(pts, tally, rng, equiv=(c % 4 < 2))
        done[kind] += 1
    out = dict(clouds=done, seed=args.seed, seconds=round(time.monotonic() - started, 1),
               checks=tally.count, failures=tally.fail, examples=tally.examples,
               diag_ratio_faithful={r: str(v) for r, v in DIAG.items()})
    with open(args.out, 'w') as fh:
        json.dump(out, fh, indent=1, sort_keys=True, default=str)
    print('nuages %s en %.1f s' % (done, time.monotonic() - started))
    bad = 0
    for name in sorted(tally.count):
        fails = tally.fail.get(name, 0)
        if name.startswith('M'):
            verdict = 'mutant tue' if fails else 'MUTANT VIVANT'
            bad += 0 if fails else 1
        else:
            verdict = 'conforme' if not fails else 'ECART'
            bad += 1 if fails else 0
        print('%-40s controles %6d  echecs %5d  %s' % (name, tally.count[name], fails, verdict))
    for rule in sorted(DIAG):
        print('diagnostic : max u(i,j) / max(e_i, e_j, w(i,j)) pour %-8s = %s (%.4f) ; borne prouvee 4'
              % (rule, DIAG[rule], float(DIAG[rule])))
    print('verdict_garanties %s' % ('conforme' if not bad else 'ECART'))
    return 1 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main())
