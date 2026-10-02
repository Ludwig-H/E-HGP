#!/usr/bin/env python3
"""Existence mure, forme (ii) : maturite GEOMETRIQUE. Reference exacte par K-parties exhaustives (petites scenes).

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=benchmark_only, public_status=not_claimed. GCP non utilise. Oracle borne (n <= 12 environ), hors de tout chemin
de mesure.

DEFINITION. tau dans [0, 1]. Le site x est MUR dans la composante C de L_K(r), au niveau r, ssi
    dist(x, C_r) <= tau r                     (tau = 1 : x est couvert par C ; tau = 0 : x est dans C).
Taille geometrique : s_tau(C, r) = nombre de sites murs dans C au niveau r. Elle croit le long d'une lignee (C_r
croit, tau r aussi). Elle est intrinseque (aucune projection) ; un site peut etre mur dans plusieurs composantes.

CE QUI EST CALCULABLE. C_r est la reunion des convexes  I(S, r) = intersection des boules B(s, r), s dans S,  sur
les K-parties S dont le convexe est dans C (lemme de descente de vote_condense : I(S, r) non vide est dans la
composante du centre de la boule minimale de S). Donc x est mur dans C au niveau r ssi il existe une K-partie S de
composante C au niveau r avec
    B(x, tau r) inter I(S, r) non vide   <=>   min_y max( max_s |y - s|^2 - r^2, |y - x|^2 - tau^2 r^2 ) <= 0.
  (a) A un niveau beta = r^2 RATIONNEL (et tau rationnel) tous les rayons carres sont rationnels : le minimum du
      maximum de fonctions puissance est atteint en un point rationnel (systeme lineaire sur un support d'au plus
      quatre contraintes actives) ; la decision est EXACTE (Fraction) : puissance_min, mur.
  (b) La date de maturite rho_tau(S, x) = min_y max(max_s |y - s|, |y - x| / tau) (boule minimax ponderee) est
      algebrique : son carre est racine d'un polynome de degre 2 a coefficients rationnels (spheres d'Apollonius
      |y - x| = tau |y - s|) ; le rayon est un radical imbrique, hors de la classe « somme de racines de
      rationnels » des dates du juge. Ici : ENCADREMENT certifie par bissection sur des niveaux rationnels (chaque
      etape est une decision exacte (a)) ; forme close seulement quand x est dans S avec K = 2 : |x s| / (1 + tau).
  (c) Date de maturite de x dans la lignee d'un noeud o : min sur S de max(rho_tau(S, x), niveau ou la composante
      de S rejoint la lignee de o) : encadrement.
RELATION AVEC LA FORME (i) (preuves : CONCEPTION.md § 5). Pour toute composante : dist(x, C_r) >= d_K(x) - r, donc
la date geometrique est >= d_K(x) / (1 + tau). Pour la lignee de la premiere boule couvrante : dist(x, C_r) <=
2 alpha_K(x) - r, donc la date est <= 2 alpha_K(x) / (1 + tau). Avec theta = (1 - tau) / (1 + tau) les deux formes
tombent dans le meme intervalle [(1 + theta) d_K / 2, (1 + theta) alpha_K] ; a K = 2 (alpha = d / 2) elles
COINCIDENT pour la lignee cover.

Aucun assert : identique sous python3 -O.
"""
from fractions import Fraction
from itertools import combinations
import json
import os
import sys

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
os.environ.setdefault('MHGP10_FIXTURES_SCRATCH', '/tmp/mhgp10-existence-mure/lib_scratch')
if ICI not in sys.path:
    sys.path.insert(0, ICI)


class GeoErreur(RuntimeError):
    """Invariant viole (jamais un assert)."""


def exiger(cond, msg):
    if not cond:
        raise GeoErreur(msg)


def _d2(a, b):
    return sum((Fraction(x) - Fraction(y)) ** 2 for x, y in zip(a, b))


def _resoudre(G, h):
    """Solution exacte de G t = h (Fraction, elimination de Gauss) ; None si G est singuliere."""
    m = len(h)
    A = [list(G[i]) + [h[i]] for i in range(m)]
    for c in range(m):
        piv = next((r for r in range(c, m) if A[r][c] != 0), None)
        if piv is None:
            return None
        A[c], A[piv] = A[piv], A[c]
        inv = 1 / A[c][c]
        A[c] = [v * inv for v in A[c]]
        for r in range(m):
            if r != c and A[r][c] != 0:
                f = A[r][c]
                A[r] = [a - f * b for a, b in zip(A[r], A[c])]
    return [A[i][m] for i in range(m)]


def puissance_min(centres, rayons2):
    """min sur y de max_i (|y - c_i|^2 - R_i^2), exact. centres : triplets ; rayons2 : Fraction >= 0.
    Enumeration des supports actifs d'au plus quatre contraintes : pour un support A, le point de l'enveloppe affine
    de A ou les puissances sont egales ; s'il est combinaison convexe des centres de A et si aucune autre contrainte
    ne le depasse, c'est le minimiseur (conditions de Karush-Kuhn-Tucker d'un probleme convexe). Rend (valeur, y)."""
    # un meme centre deux fois : seule la contrainte la plus serree compte
    garde = {}
    for c, r2 in zip(centres, rayons2):
        c = tuple(Fraction(v) for v in c)
        r2 = Fraction(r2)
        if c not in garde or r2 < garde[c]:
            garde[c] = r2
    cs = list(garde)
    rs = [garde[c] for c in cs]
    m = len(cs)
    for taille in range(1, min(4, m) + 1):
        for A in combinations(range(m), taille):
            c0, r0 = cs[A[0]], rs[A[0]]
            vec = [[cs[j][k] - c0[k] for k in range(3)] for j in A[1:]]
            if vec:
                G = [[sum(a * b for a, b in zip(u, w)) for w in vec] for u in vec]
                h = [(sum(a * a for a in u) - rs[j] + r0) / 2 for u, j in zip(vec, A[1:])]
                t = _resoudre(G, h)
                if t is None or any(x < 0 for x in t) or sum(t) > 1:
                    continue
                y = tuple(c0[k] + sum(t[i] * vec[i][k] for i in range(len(vec))) for k in range(3))
            else:
                y = c0
            f = _d2(y, c0) - r0
            if all(_d2(y, cs[i]) - rs[i] <= f for i in range(m) if i not in A):
                return f, y
    raise GeoErreur('puissance_min : aucun support (contradiction avec la convexite)')


def mur(P, S, x, tau, beta):
    """Le site x est-il mur relativement a la K-partie S au niveau beta = r^2 (decision exacte) ?"""
    tau, beta = Fraction(tau), Fraction(beta)
    f, _y = puissance_min([P[s] for s in S] + [P[x]], [beta] * len(S) + [tau * tau * beta])
    return f <= 0


def date_encadree(P, S, x, tau, niveau_S, precision=Fraction(1, 10 ** 12)):
    """Encadrement (lo, hi) de rho_tau(S, x)^2 : mur a hi, non mur a lo (ou lo = niveau de S si deja mur)."""
    tau = Fraction(tau)
    lo = Fraction(niveau_S)
    if mur(P, S, x, tau, lo):
        return lo, lo
    if tau == 0:
        hi = max(_d2(P[x], P[s]) for s in S)
        return hi, hi                              # x dans I(S, r) ssi r >= max |x - s| (exact)
    hi = max(max(_d2(P[x], P[s]) for s in S) / (tau * tau), lo * 4)
    while not mur(P, S, x, tau, hi):
        hi *= 2
    while hi - lo > precision * hi:
        mid = (lo + hi) / 2
        if mur(P, S, x, tau, mid):
            hi = mid
        else:
            lo = mid
    return lo, hi


class Geo:
    """Maturite geometrique d'une em_exact.Base (petite scene) : K-parties exhaustives, composantes par
    vc_exact.Structure.localiser (boule minimale de la K-partie, noeud de FULL_K a son niveau)."""

    def __init__(self, base, limite=20000):
        import math
        self.base = base
        st = base.st
        self.st, self.T, self.n, self.K = st, base.T, base.n, base.K
        exiger(math.comb(self.n, self.K) <= limite, 'forme (ii) : trop de K-parties pour l oracle exhaustif')
        sites = st.ex.sites
        self.P = [tuple(int(c) for c in base.sc.P[p]) for p in range(self.n)]
        self.parties = []                          # (points de S, niveau, noeud)
        for f in combinations(range(self.n), self.K):
            lvl, v = st.localiser(f)
            self.parties.append((tuple(st.point[s] for s in f), lvl, v))
        exiger(all(tuple(sites[st.site[p]]) == self.P[p] for p in range(self.n)), 'sites et points incoherents')
        self._dates = {}

    def taille(self, v, beta, tau):
        """Sites murs dans le noeud v au niveau beta (rationnel, dans la vie de v) : liste triee."""
        beta = Fraction(beta)
        T = self.T
        exiger(T.birth[v] <= beta and (T.death[v] is None or beta < T.death[v]), 'niveau hors de la vie du noeud')
        out = set()
        for S, lvl, u in self.parties:
            if lvl > beta or T.anc(u, beta) != v:
                continue
            for x in range(self.n):
                if x not in out and mur(self.P, S, x, tau, beta):
                    out.add(x)
        return sorted(out)

    def maturite(self, x, o, tau):
        """Encadrement (lo, hi), en rayons carres, de la date de maturite geometrique de x dans la lignee du noeud o."""
        T = self.T
        best_lo = best_hi = None
        for S, lvl, u in self.parties:
            w = T.lca(u, o)
            j = max(lvl, T.birth[o], T.birth[w])
            if best_hi is not None and j >= best_hi:
                continue
            cle = (S, x, Fraction(tau))
            if cle not in self._dates:
                self._dates[cle] = date_encadree(self.P, S, x, tau, lvl)
            lo, hi = self._dates[cle]
            lo, hi = max(lo, j), max(hi, j)
            if best_hi is None or hi < best_hi:
                best_hi = hi
            if best_lo is None or lo < best_lo:
                best_lo = lo
        return best_lo, best_hi


def controle_bornes(graines, n, K, taus):
    """Sur des nuages de mise au point : d_K / (1 + tau) <= date geometrique (lignee cover) <= 2 alpha / (1 + tau),
    et egalite des deux bornes a K = 2. Rend des comptes."""
    import em_exact as EX
    import vc_proprietes as VP
    out = {'sites': 0, 'borne_basse_violee': 0, 'borne_haute_violee': 0, 'egalites_K2': 0, 'ecart_relatif_max': 0.0}
    for g in graines:
        P = VP.nuage(g, n)
        base = EX.Base(EX.RG.Scene(P, K, nom='geo_%d' % g, gamma='non', cache=False))
        geo = Geo(base)
        for tau in taus:
            tau = Fraction(tau)
            for x in range(base.n):
                a2, o = base.st.couv1[x]
                d2 = base.st.coeur[x][0]
                lo, hi = geo.maturite(x, o, tau)
                bas = d2 / (1 + tau) ** 2
                haut = 4 * a2 / (1 + tau) ** 2
                out['sites'] += 1
                if hi < bas:
                    out['borne_basse_violee'] += 1
                if lo > haut:
                    out['borne_haute_violee'] += 1
                if K == 2:
                    if not (lo <= haut <= hi and bas == haut):
                        out['egalites_K2'] -= 10 ** 6        # garde : l'egalite doit tenir a chaque site
                    out['egalites_K2'] += 1
                out['ecart_relatif_max'] = max(out['ecart_relatif_max'], float((hi - lo) / hi))
    return out


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    res = {'K2': controle_bornes(range(2), 9, 2, ('0', '1/8', '1/2', '1')),
           'K3': controle_bornes(range(2), 8, 3, ('0', '1/2', '1'))}
    ok = all(r['borne_basse_violee'] == 0 and r['borne_haute_violee'] == 0 for r in res.values()) and \
        res['K2']['egalites_K2'] == res['K2']['sites'] and res['K2']['sites'] >= 60 and res['K3']['sites'] >= 40
    res['tenu'] = ok
    res['optimize'] = sys.flags.optimize
    with open(a.out, 'w') as f:
        json.dump(res, f, indent=1, sort_keys=True)
        f.write('\n')
    print(json.dumps(res))
    sys.exit(0 if ok else 3)
