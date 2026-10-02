"""Existence mure, chemin rapide : DATES NOUVELLES rangees exactement dans une table de niveaux.

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, mode=benchmark_only, public_status=not_claimed.

Une table de niveaux est une suite strictement croissante de rayons CARRES exacts l_0 < l_1 < ... (rationnels) ; on
n'en connait en flottant que val[k] = l_k (1 + d), |d| <= u = 2^-53 (division entiere correctement arrondie, ou
entier exact). Une date nouvelle est un RAYON exact m >= 0 :
    pq   : m = p sqrt(A) + q sqrt(D),  A = l_a, D = l_d niveaux de la table, p, q rationnels >= 0 (maturite
           interpolee) ;
    libre : m = somme exacte de racines (vc_certif.Racines) fournie a la demande (date de cone de ER0h, maturite
           interpolee a partir d'une date de cone).
Chaque date porte une valeur flottante m^ et une borne RELATIVE eps : |m^ - m| <= eps m.

PLACEMENT (proposition P, preuve dans README.md § 4). Avec eps' = eps + 4u :
    lo = fl(m^ fl(1 - eps')),  hi = fl(m^ fl(1 + eps')),  x1 = fl(fl(lo lo)(1 - 8u)),  x2 = fl(fl(hi hi)(1 + 8u)),
    r1 = max{k : val[k] <= x1},  r2 = max{k : val[k] <= x2}.
Alors lo < m < hi, l_k < m^2 pour tout k <= r1 et l_k > m^2 pour tout k > r2. Si r1 = r2 la date est CERTIFIEE
strictement entre les niveaux r1 et r1 + 1. Sinon REPLI EXACT : recherche dichotomique sur ]r1, r2] par le signe exact
de m - sqrt(l_k) ; pour une date pq ce signe est decide PAR ELEVATION AU CARRE (signe_pq : arithmetique rationnelle
seulement) ; pour une date libre, par vc_certif.Racines.signe (encadrement entier, classes de carres).
ORDRE dans un intervalle : deux dates dont les encadrements [lo, hi] sont disjoints sont ordonnees par le flottant
(certifie) ; les composantes de chevauchement sont triees par comparaisons exactes ; deux dates exactement egales
partagent leur sous-rang. Le resultat est une cle (rang de table r, sous-rang j) : j = 0 si la date EGALE le niveau r,
sinon j >= 1 numerote les dates distinctes strictement comprises entre les niveaux r et r + 1, dans l'ordre exact.

Aucun flottant ne decide sans sa borne ; aucun assert (identique sous python3 -O).
"""
from fractions import Fraction
from functools import cmp_to_key
import os
import sys

import numpy as np

sys.dont_write_bytecode = True
RACINE = os.environ.get('MHGP10_VROOT', '/workspaces/E-HGP/build')
VC_CODE = os.path.join(RACINE, 'v10-tour-vers-points', 'regles2', 'vote_condense_v2', 'code')
if VC_CODE not in sys.path:
    sys.path.insert(0, VC_CODE)
import vc_certif as CF  # noqa: E402  (vote_condense_v2, lecture seule)

U = 2.0 ** -53
BORNE_PQ = 6.0 * U          # borne relative de m^ = fl(fl(p^ a^) + fl(q^ d^)) (README § 4, lemme 1)
BORNE_TABLE = 2.0 * U       # borne relative de fl(sqrt(val[k])) par rapport a sqrt(l_k)
# Mutants CAUSAUX, pour les tests seulement : `sans_borne` (le flottant decide seul : eps = 0, aucun repli) ;
# `sans_carre` (le repli exact d'une date pq compare les flottants au lieu d'elever au carre).
MUTANTS = set()
MUTANTS_CONNUS = ('sans_borne', 'sans_carre')
# Mode de controle : 'tout_exact' envoie chaque placement et chaque ordre au repli exact, sans filtre flottant.
FORCE = set()


class DatesErreur(RuntimeError):
    """Invariant viole (jamais un assert)."""


def exiger(cond, msg):
    if not cond:
        raise DatesErreur(msg)


def signe_pq(p, A, q, D, L):
    """Signe EXACT de p sqrt(A) + q sqrt(D) - sqrt(L), par elevation au carre. p, q, A, D, L rationnels >= 0.
    (p sqrt A + q sqrt D)^2 - L = 2 p q sqrt(A D) - R, R = L - p^2 A - q^2 D ; si R < 0 le signe est + ; sinon on
    compare 4 p^2 q^2 A D a R^2 (deux nombres >= 0 de meme ordre que leurs racines)."""
    p, q, A, D, L = Fraction(p), Fraction(q), Fraction(A), Fraction(D), Fraction(L)
    exiger(p >= 0 and q >= 0 and A >= 0 and D >= 0 and L >= 0, 'signe_pq : argument negatif')
    R = L - p * p * A - q * q * D
    if R < 0:
        return 1
    X = 4 * p * p * q * q * A * D
    Y = R * R
    return (X > Y) - (X < Y)


class Nouvelles:
    """Dates nouvelles a ranger dans une table. val : tableau flottant des niveaux (non decroissant) ;
    niveau(k) : niveau exact (Fraction) du rang k ; rval[k] = fl(sqrt(val[k]))."""

    def __init__(self, val, niveau, rval=None):
        self.tval = np.asarray(val, dtype=np.float64)
        self.niveau = niveau
        self.rval = np.sqrt(self.tval) if rval is None else np.asarray(rval, dtype=np.float64)
        self.m, self.eps, self.forme, self.lo_rk, self.hi_rk = [], [], [], [], []
        self._cle = {}
        self._exact = {}
        self.gap, self.sub = [], []
        self.compte = {'dates': 0, 'rang_certifie': 0, 'replis_exacts_rang': 0, 'comparaisons_exactes_rang': 0,
                       'egales_a_un_niveau': 0, 'replis_exacts_ordre': 0, 'comparaisons_exactes_ordre': 0,
                       'egalites_exactes_entre_dates': 0}

    def __len__(self):
        return len(self.m)

    # -- declaration
    def pq(self, p, ra, q, rd):
        """Date p sqrt(l_ra) + q sqrt(l_rd), ra < rd rangs de table, p, q > 0, p + q = 1 : strictement entre les
        niveaux ra et rd. Dedoublonnee par (ra, rd, p, q). Rend l'identifiant."""
        cle = ('pq', int(ra), int(rd), p, q)
        i = self._cle.get(cle)
        if i is None:
            exiger(0 <= ra < rd and p > 0 and q > 0 and p + q == 1, 'date pq : rangs ou poids hors domaine')
            mf = float(p) * float(self.rval[ra]) + float(q) * float(self.rval[rd])
            i = self._ajoute(mf, BORNE_PQ, ('pq', int(ra), int(rd), Fraction(p), Fraction(q)), int(ra), int(rd) - 1)
            self._cle[cle] = i
        return i

    def libre(self, mf, eps, exact, cle=None, lo_rk=-1, hi_rk=None):
        """Date de valeur flottante mf (rayon), borne relative eps, forme exacte exact() -> vc_certif.Racines.
        lo_rk / hi_rk : rangs de table certains (niveau(lo_rk) <= date^2 ; date^2 < niveau(hi_rk + 1))."""
        if cle is not None and cle in self._cle:
            return self._cle[cle]
        # la proposition P exige eps^2 <= u (termes du second ordre) : eps <= 1e-8
        exiger(mf >= 0.0 and 0.0 <= eps <= 1e-8, 'date libre : valeur ou borne hors domaine')
        i = self._ajoute(float(mf), float(eps), ('libre', exact), int(lo_rk),
                         len(self.tval) - 1 if hi_rk is None else int(hi_rk))
        if cle is not None:
            self._cle[cle] = i
        return i

    def _ajoute(self, mf, eps, forme, lo_rk, hi_rk):
        self.m.append(mf)
        self.eps.append(eps)
        self.forme.append(forme)
        self.lo_rk.append(lo_rk)
        self.hi_rk.append(hi_rk)
        self.gap.append(None)
        self.sub.append(None)
        self.compte['dates'] += 1
        return len(self.m) - 1

    # -- formes exactes
    def exact(self, i):
        """Rayon exact (vc_certif.Racines) de la date i."""
        x = self._exact.get(i)
        if x is None:
            f = self.forme[i]
            if f[0] == 'pq':
                x = CF.Racines()
                x.ajoute(self.niveau(f[1]), f[3])
                x.ajoute(self.niveau(f[2]), f[4])
            else:
                x = f[1]()
                exiger(isinstance(x, CF.Racines), 'date libre : forme exacte Racines attendue')
            self._exact[i] = x
        return x

    def cmp_niveau(self, i, k):
        """Signe exact de date_i - sqrt(niveau(k))."""
        f = self.forme[i]
        if f[0] == 'pq':
            if 'sans_carre' in MUTANTS:     # mutant (tests) : comparaison des flottants
                d = self.m[i] * self.m[i] - float(self.tval[k])
                return (d > 0) - (d < 0)
            return signe_pq(f[3], self.niveau(f[1]), f[4], self.niveau(f[2]), self.niveau(k))
        return (self.exact(i) - CF.Racines.racine(self.niveau(k))).signe()

    def cmp(self, i, j):
        """Signe exact de date_i - date_j."""
        if i == j:
            return 0
        return (self.exact(i) - self.exact(j)).signe()

    # -- placement
    def _encadre(self, ids):
        m = np.array([self.m[i] for i in ids], dtype=np.float64)
        e = np.array([self.eps[i] for i in ids], dtype=np.float64)
        if 'sans_borne' in MUTANTS:
            return m, m, m * m, m * m
        ep = e + 4.0 * U
        lo = m * (1.0 - ep)
        hi = m * (1.0 + ep)
        x1 = (lo * lo) * (1.0 - 8.0 * U)
        x2 = (hi * hi) * (1.0 + 8.0 * U)
        return lo, hi, x1, x2

    def placer(self):
        """Rang de table de chaque date non encore placee : gap[i] = plus grand k avec niveau(k) <= date^2 ;
        sub[i] = 0 si egalite exacte, sinon -1 (sous-rang a fixer par ordonner)."""
        ids = [i for i in range(len(self.m)) if self.gap[i] is None]
        if not ids:
            return
        lo, hi, x1, x2 = self._encadre(ids)
        r1 = np.searchsorted(self.tval, x1, side='right') - 1
        r2 = np.searchsorted(self.tval, x2, side='right') - 1
        r1 = np.maximum(r1, np.array([self.lo_rk[i] for i in ids], dtype=np.int64))
        r2 = np.minimum(r2, np.array([self.hi_rk[i] for i in ids], dtype=np.int64))
        if FORCE:
            r1 = np.array([self.lo_rk[i] for i in ids], dtype=np.int64)
            r2 = np.array([self.hi_rk[i] for i in ids], dtype=np.int64)
        if 'sans_borne' in MUTANTS:
            for i, a in zip(ids, r1.tolist()):
                self.gap[i], self.sub[i] = int(a), -1
            return
        sur = r2 <= r1
        self.compte['rang_certifie'] += int(sur.sum())
        r1l, r2l = r1.tolist(), r2.tolist()
        for pos, i in enumerate(ids):
            a, b = r1l[pos], r2l[pos]
            if b <= a:
                exiger(a >= 0, 'date nouvelle sous tous les niveaux')
                self.gap[i], self.sub[i] = a, -1
                continue
            # repli exact : niveau(a) < date^2 (strict, certifie, ou a = -1) ; niveau(b + 1) > date^2
            self.compte['replis_exacts_rang'] += 1
            lo_k, hi_k, sg_lo = a, b + 1, 1
            while hi_k - lo_k > 1:
                mid = (lo_k + hi_k) // 2
                self.compte['comparaisons_exactes_rang'] += 1
                sg = self.cmp_niveau(i, mid)
                if sg >= 0:
                    lo_k, sg_lo = mid, sg
                else:
                    hi_k = mid
            exiger(lo_k >= 0, 'date nouvelle sous tous les niveaux')
            self.gap[i] = lo_k
            if sg_lo == 0:
                self.sub[i] = 0
                self.compte['egales_a_un_niveau'] += 1
            else:
                self.sub[i] = -1

    def ordonner(self):
        """Sous-rangs exacts des dates strictement comprises entre deux niveaux (apres placer)."""
        self.placer()
        ids = [i for i in range(len(self.m)) if self.sub[i] != 0]
        if not ids:
            return
        lo, hi, _x1, _x2 = self._encadre(ids)
        gap = np.array([self.gap[i] for i in ids], dtype=np.int64)
        o = np.lexsort((lo, gap))
        ids_o = [ids[k] for k in o.tolist()]
        gap_o, lo_o, hi_o = gap[o], lo[o], hi[o]
        n = len(ids_o)
        meme = np.zeros(n, dtype=bool)
        meme[1:] = gap_o[1:] == gap_o[:-1]
        cm = np.maximum.accumulate(hi_o)
        touche = np.zeros(n, dtype=bool)        # la date k chevauche (peut-etre) une date precedente du meme intervalle
        if 'sans_borne' not in MUTANTS:
            touche[1:] = meme[1:] & (lo_o[1:] <= cm[:-1])
        if FORCE:
            touche = meme.copy()
        ml, tl = meme.tolist(), touche.tolist()
        # inc[k] = 1 si la date k est strictement apres la date k - 1 (meme intervalle), 0 si exactement egale
        inc = [1] * n
        k = 0
        while k < n:
            if not tl[k]:
                k += 1
                continue
            a = k - 1                              # debut de la composante de chevauchement
            b = k
            while b + 1 < n and tl[b + 1]:
                b += 1
            comp = ids_o[a:b + 1]
            self.compte['replis_exacts_ordre'] += 1

            def cle(x, y):
                self.compte['comparaisons_exactes_ordre'] += 1
                return self.cmp(x, y)
            comp.sort(key=cmp_to_key(cle))
            ids_o[a:b + 1] = comp
            for t in range(a + 1, b + 1):
                self.compte['comparaisons_exactes_ordre'] += 1
                if self.cmp(ids_o[t], ids_o[t - 1]) == 0:
                    inc[t] = 0
                    self.compte['egalites_exactes_entre_dates'] += 1
            k = b + 1
        j = 0
        for k in range(n):
            if not ml[k]:
                j = 1
            else:
                j += inc[k]
            self.sub[ids_o[k]] = j

    def cles(self):
        """[(rang de table, sous-rang)] de toutes les dates, apres ordonner."""
        self.ordonner()
        return list(zip(self.gap, self.sub))


def raffiner(nr, cles):
    """Table raffinee : niveaux de la table (rang r -> cle (r, 0)) et dates nouvelles (cles (r, j), j >= 1).
    Rend (rk_map[r] : nouveau rang du niveau r, nouveau rang de chaque cle, nombre de rangs, inverse : nouveau rang
    -> ancien rang ou -1)."""
    cles = list(cles)
    cnt = np.zeros(nr + 1, dtype=np.int64)
    for r, j in cles:
        exiger(0 <= r < nr and j >= 0, 'cle de date hors de la table')
        if j > cnt[r + 1]:
            cnt[r + 1] = j
    off = np.cumsum(cnt)[:-1]                       # dates distinctes dans les intervalles precedant le niveau r
    rk_map = np.arange(nr, dtype=np.int64) + off
    nr2 = int(nr + cnt.sum())
    nouveaux = [int(rk_map[r]) + j for r, j in cles]
    inverse = np.full(nr2, -1, dtype=np.int64)
    inverse[rk_map] = np.arange(nr, dtype=np.int64)
    return rk_map, nouveaux, nr2, inverse
