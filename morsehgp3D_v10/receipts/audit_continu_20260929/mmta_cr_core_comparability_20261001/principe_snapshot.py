#!/usr/bin/env python3
"""Principe libre : de FULL_K complete a une hierarchie laminaire de points qui depend de min_cluster_size (mcs).

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=juge_final_principe_libre, public_status=not_claimed. GCP non utilise. Aucun moteur modifie. La bibliotheque
partagee fixtures_cibles/lib et le worktree partage sont IMPORTES en lecture seule (aucune ecriture, pas de bytecode).

Objet (K fixe, mcs fixe). FULL_K : arbre T (naissance b(v), mort d(v) = b(parent)). Relation de couverture :
c_y(v) = premier niveau (rayon carre exact) ou le noeud v couvre le point y pendant sa vie (lemme du catalogue,
fullk.native_tree). Les regles MMtA ne lisent que FULL_K et la relation de couverture : elles sont intrinseques (TI).

Admissibilite (principe Pi2 : un groupe n'est un cluster qu'a partir de mcs points).
  n_v(s) = |{y : c_y(v) <= s}| : taille de l'amas discret X n delta_r(v) (definition 8) au niveau s ;
  a(v)  = premier niveau de la vie de v ou n_v >= mcs (None si jamais) ;
  A(v)  = a(v) si defini, sinon A(parent(v)) : premier niveau ou la LIGNEE de v peut porter un cluster.
  Pour mcs <= K, a(v) = b(v) pour tout noeud (une composante nait avec au moins K points couverts).

Regle MMtA(kappa, eta, lam) (MMt « a admissibilite »), pour un point x :
  V_x : noeuds qui couvrent x ; A_x = alpha(x)^2 = min c_x(v) ; Ahat_x = min_v max(c_x(v), A(v)) (premiere
  couverture par une structure admissible) ; bord de bande E2 = max((1 + eta) A_x, Ahat_x + eta A_x).
  Ah_x(u) = min sur w de V_x sous u de max(c_x(w), A(w)) (premiere couverture admissible de x a l'interieur de u).
  Rivale d'une lignee v : enfant c d'un ancetre strict P de v, hors du cote de v, avec Ah_x(c) < b(P). Force
  sigma = min(1, (b(P) - Ah_x(c)) / (lam A_x)). Rencontre effective Rt_x(v) = min sur les rivales de
  b(P) + (1 - sigma) lam A_x (infini sans rivale).
  Poids de la prehistoire : omega_x(v) = min(1, (Rt_x(v) - A(v))_+ / (lam A_x)) ; 0 si A(v) = None.
  Masse du porteur v au niveau s : omega |[c_x(v), min(A(v), d(v), s, E2)]| + |[max(c_x(v), A(v)), min(d(v), s, E2)]|
  (temps de couverture de MMt ; la prehistoire, avant que la lignee ne puisse etre un cluster, compte avec le poids
  omega : pleinement si la lignee devient admissible nettement avant de rencontrer une rivale admissible, pas du
  tout si elle ne l'est qu'en la rencontrant).
  Majorite a marge de MMt sur les composantes de FULL_K, avec ces masses (pentes 0, omega ou 1) : T_1/2, cone
  t = sup_theta sqrt(T(theta)) - kappa alpha (2 theta - 1) ; plancher d'admissibilite : date finale max(t, sqrt(A(O)))
  ; proprietaire = ancetre de O vivant a la date finale.
  Variante dure (lam -> 0) : omega = 1 si A(v) < R_x(v) (rencontre de la premiere rivale), 0 sinon.
  Parametres par defaut (kappa, eta, lam) = (3, 1/2, 1/2) (MEMO, choix des parametres).

Variantes a respect du coeur (MMtA_CR) : la composante qui contient x a partir de d_K(x) est imposee des qu'elle
peut porter un cluster ; « noyau » : au moins mcs points de X dedans (proposee) ; « amas » : amas discret d'au
moins mcs points (refutee par Q1 +- 1, MEMO).

Tout est exact : niveaux, poids, masses, W, marges rationnels (Fraction) ; dates en sommes de racines (QS) ;
comparaisons exactes. Aucun assert : comportement identique sous python3 -O.
"""
import os
import sys
from fractions import Fraction

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
SCRATCH = '/tmp/mhgp10-juge-final/principe_libre'
os.environ.setdefault('MHGP10_FIXTURES_SCRATCH', os.path.join(SCRATCH, 'lib_scratch'))
LIB = '/workspaces/E-HGP/build/v10-verrou-points/fixtures_cibles/lib'
if LIB not in sys.path:
    sys.path.insert(0, LIB)
import regles as RG  # noqa: E402  (bibliotheque partagee, lecture seule)
from regles import Rayon, QS  # noqa: E402

CADRE = ('phase=exploration_v10_hors_registre backend=cpu_reference profile=quantized_u18_input_only '
         'mode=juge_final_principe_libre public_status=not_claimed ; GCP non utilise')
KAPPA, ETA, LAM = Fraction(3), Fraction(1, 2), Fraction(1, 2)


class PLErreur(RuntimeError):
    """Invariant viole (jamais un assert) : code 3 des executables."""


def exiger(cond, msg):
    if not cond:
        raise PLErreur(msg)


def qmax(a, b):
    return a if a.cmp(b) >= 0 else b


def bord(Ax, Ahat, eta):
    """Bord de bande E2 = max((1 + eta) A_x, Ahat + eta A_x) ; (1 + eta) A_x si aucune structure admissible."""
    eta = Fraction(eta)
    if Ahat is None:
        return (1 + eta) * Ax
    return max((1 + eta) * Ax, Ahat + eta * Ax)


# ------------------------------------------------------------------ donnees intrinseques d'une scene

class Donnees:
    """FULL_K natif (arbre des masses, memes indices que sc.foret), relation de couverture par point, attache coeur."""

    def __init__(self, sc):
        T, cover, _info = sc.arbre_masses()
        self.sc, self.T = sc, T
        self.n = sc.n
        self.nn = len(T.birth)
        self.cov = [None] * sc.n
        for s, cv in enumerate(cover):
            self.cov[sc.ctx.point_id[s]] = dict(cv)
        for p in range(sc.n):
            exiger(self.cov[p], 'point %s sans couverture' % sc.noms[p])
        self.par_noeud = {}
        for p in range(sc.n):
            for v, c in self.cov[p].items():
                exiger(T.birth[v] <= c and (T.death[v] is None or c < T.death[v]),
                       'couverture hors de la vie du noeud %d' % v)
                self.par_noeud.setdefault(v, []).append(c)
        for v in self.par_noeud:
            self.par_noeud[v].sort()
        self._adm = {}
        self._adm_coeur = {}
        self.tin, self.tout = [0] * self.nn, [0] * self.nn
        horloge = 0
        pile = [(T.root, 0)]
        while pile:
            v, etat = pile.pop()
            if etat == 0:
                self.tin[v] = horloge
                horloge += 1
                pile.append((v, 1))
                for c in T.children[v]:
                    pile.append((c, 0))
            else:
                self.tout[v] = horloge
                horloge += 1
        self.dk2 = [None] * sc.n
        self.coeur = [None] * sc.n
        for s in range(sc.ctx.n):
            p = sc.ctx.point_id[s]
            self.dk2[p] = sc.ctx.dk2(s)
            self.coeur[p] = int(sc.ctx.order.core_node[s])

    def sous(self, u, v):
        """u descendant de v (ou egal)."""
        return self.tin[v] <= self.tin[u] and self.tout[u] <= self.tout[v]

    def lca(self, u, v):
        T = self.T
        while T.depth[u] > T.depth[v]:
            u = T.parent[u]
        while T.depth[v] > T.depth[u]:
            v = T.parent[v]
        while u != v:
            u, v = T.parent[u], T.parent[v]
        return u

    def anc(self, v, s):
        """Ancetre de v vivant au niveau s (coupe fermee) ; v ne avant s."""
        T = self.T
        exiger(T.birth[v] <= s, 'noeud non ne')
        while T.parent[v] >= 0 and T.birth[T.parent[v]] <= s:
            v = T.parent[v]
        return v

    def anc_rayon(self, v, r):
        """Ancetre de v vivant au rayon r (Rayon, coupe fermee)."""
        T = self.T
        while T.parent[v] >= 0 and Rayon(b2=T.birth[T.parent[v]]).cmp(r) <= 0:
            v = T.parent[v]
        return v

    def admissibilite(self, mcs):
        """(a, A) : a[v] premier niveau de vie ou n_v >= mcs (None sinon) ; A[v] lignee (None = jamais)."""
        if mcs in self._adm:
            return self._adm[mcs]
        T = self.T
        a = [None] * self.nn
        for v in range(self.nn):
            cs = self.par_noeud.get(v, [])
            if len(cs) >= mcs:
                a[v] = cs[mcs - 1]
        A = [None] * self.nn
        for v in T.topdown:
            if a[v] is not None:
                A[v] = a[v]
            elif T.parent[v] >= 0:
                A[v] = A[T.parent[v]]
        self._adm[mcs] = (a, A)
        return a, A

    def admissibilite_coeur(self, mcs):
        """(ac, Ac) : ac[v] premier niveau de vie ou v contient au moins mcs points de X (coeur : d_K(y) <= r et y
        dans v) ; Ac[v] lignee. Un point y est dans v au niveau s ssi coeur(y) est sous v et d_K(y)^2 <= s."""
        if mcs not in self._adm_coeur:
            self._adm_coeur[mcs] = adm_coeur(self.T, self.nn, self.coeur, self.dk2, mcs)
        return self._adm_coeur[mcs]


def adm_coeur(T, nn, coeur, dk2, mcs):
    """Balayage ascendant : les mcs plus petits d_K^2 des points dont l'attache coeur est sous v."""
    directs = {}
    for y in range(len(coeur)):
        directs.setdefault(coeur[y], []).append(dk2[y])
    petits = [None] * nn
    ac = [None] * nn
    for v in reversed(T.topdown):
        vals = list(directs.get(v, []))
        for c in T.children[v]:
            vals += petits[c]
        vals.sort()
        petits[v] = vals[:mcs]
        if len(vals) >= mcs:
            lvl = max(vals[mcs - 1], T.birth[v])
            if T.death[v] is None or lvl < T.death[v]:
                ac[v] = lvl
    Ac = [None] * nn
    for v in T.topdown:
        if ac[v] is not None:
            Ac[v] = ac[v]
        elif T.parent[v] >= 0:
            Ac[v] = Ac[T.parent[v]]
    return ac, Ac


# ------------------------------------------------------------------ poids de prehistoire (un point)

def poids_point(D, x, mcs, eta, lam, dur=False, cv=None):
    """Porteurs de x : {v: (c, A(v), omega)}, A_x, Ahat, E2. cv : profil restreint (variante CR) ou complet."""
    T = D.T
    _a, A = D.admissibilite(mcs)
    if cv is None:
        cv = D.cov[x]
    Ax = min(cv.values())
    hats = [max(c, A[v]) for v, c in cv.items() if A[v] is not None]
    Ahat = min(hats) if hats else None
    E2 = bord(Ax, Ahat, eta)
    lamS = None if dur else Fraction(lam) * Ax
    Ah = {}
    for u in sorted(cv, key=lambda w: -T.depth[w]):
        val = None if A[u] is None else max(cv[u], A[u])
        for c in T.children[u]:
            if c in Ah and Ah[c] is not None and (val is None or Ah[c] < val):
                val = Ah[c]
        Ah[u] = val
    rivales = {}
    for P in cv:
        bP = T.birth[P]
        for c in T.children[P]:
            if c in cv and Ah[c] is not None and Ah[c] < bP:
                rivales.setdefault(P, []).append(c)
    out = {}
    for v, c in cv.items():
        if A[v] is None:
            out[v] = (c, None, Fraction(0))
            continue
        if A[v] <= c:
            out[v] = (c, A[v], Fraction(1))      # pas de prehistoire : poids sans objet
            continue
        R = None
        u, prec = T.parent[v], v
        while u >= 0:
            for cc in rivales.get(u, ()):
                if cc == prec:
                    continue
                bP = T.birth[u]
                if dur:
                    terme = bP
                else:
                    sig = min(Fraction(1), (bP - Ah[cc]) / lamS)
                    terme = bP + (1 - sig) * lamS
                if R is None or terme < R:
                    R = terme
            if dur and R is not None:
                break
            prec, u = u, T.parent[u]
        if dur:
            om = Fraction(1) if (R is None or A[v] < R) else Fraction(0)
        else:
            om = Fraction(1) if R is None else min(Fraction(1), max(Fraction(0), R - A[v]) / lamS)
        out[v] = (c, A[v], om)
    return out, Ax, Ahat, E2


# ------------------------------------------------------------------ moteur MMt a porteurs ponderes (exact)

def _masse(c, Av, om, d, E2, s):
    """Masse d'un porteur au niveau s : om sur [c, min(Av, fin)], 1 sur [max(c, Av), fin], fin = min(d, s, E2)."""
    fin = E2 if (d is None or d > E2) else d
    if s < fin:
        fin = s
    if fin <= c:
        return Fraction(0)
    if Av is None or Av >= fin:
        return om * (fin - c)
    if Av <= c:
        return fin - c
    return om * (Av - c) + (fin - Av)


def _taux(c, Av, om, d, E2, s):
    """Pente du porteur sur [s, evenement suivant)."""
    fin = E2 if (d is None or d > E2) else d
    if s < c or s >= fin:
        return Fraction(0)
    if Av is not None and s >= Av:
        return Fraction(1)
    return om


def _date(T_half, termes, crit, T1, W, S, kappa):
    """t = max(sqrt(T_1/2), sqrt(e) - kappa alpha mu(e-), D(s*) aux points critiques, sqrt(T1) - kappa alpha)."""
    a = QS.sqrt(S)
    date = QS.sqrt(T_half)
    arg = ('T_half', T_half)
    for e, mu in termes:
        if e <= T_half:
            continue
        exiger(mu > 0, 'marge non positive apres T_1/2')
        val = QS.sqrt(e) - a * (kappa * mu)
        if val.cmp(date) > 0:
            date, arg = val, ('saut', e)
    for lo, hi, m0, s0, p in crit:
        if p <= 0:
            continue
        s_star = W * W / (16 * kappa * kappa * S * p * p)
        if lo < s_star < hi:
            G = m0 + p * (s_star - s0)
            if G < W:
                val = QS.sqrt(s_star) - a * (kappa * (2 * G / W - 1))
                if val.cmp(date) > 0:
                    date, arg = val, ('critique', s_star)
    if T1 is not None and T1 > T_half:
        val = QS.sqrt(T1) - a * kappa
        if val.cmp(date) > 0:
            date, arg = val, ('unanimite', T1)
    return date, arg


def mmt_pondere(D, porteurs, S, E2, kappa):
    """MMt sur FULL_K avec porteurs ponderes, balayage incremental exact (reference : mmt_pondere_lent).
    porteurs : {v: (c, A(v), omega)} (clos par ancetres jusqu'au LCA des porteurs massifs) ; S = A_x (echelle du
    cone) ; E2 bord de bande. Rend dict (date QS, O, T_half, W, argmax, termes) ou None si W = 0."""
    T = D.T
    kappa = Fraction(kappa)
    W = Fraction(0)
    for v, (c, Av, om) in porteurs.items():
        W += _masse(c, Av, om, T.death[v], E2, E2)
    if W == 0:
        return None
    naissances = {}
    evs = {E2}
    for v, (c, Av, om) in porteurs.items():
        evs.add(c)
        evs.add(T.birth[v])
        naissances.setdefault(T.birth[v], []).append(v)
        if Av is not None and Av > c:
            evs.add(Av)
    evs = sorted(evs)
    demi = W / 2
    masse, taux = {}, {}
    T_half = O = OC = None
    termes, crit = [], []
    T1 = None
    prec = None
    for e in evs:
        if prec is not None:
            dt = e - prec
            if T_half is None:
                for C, p in taux.items():
                    if p > 0:
                        m = masse[C]
                        if m <= demi < m + p * dt:
                            exiger(T_half is None, 'franchissement non exclusif')
                            T_half, O, OC = prec + (demi - m) / p, C, C
                if T_half is not None:
                    m0, p0 = masse[OC], taux[OC]
                    Gm = m0 + p0 * dt
                    if Gm < W:
                        termes.append((e, 2 * Gm / W - 1))
                    crit.append((T_half, e, m0, prec, p0))
                    if Gm == W and m0 < W and T1 is None:
                        T1 = e
            else:
                m0, p0 = masse[OC], taux.get(OC, Fraction(0))
                Gm = m0 + p0 * dt
                if Gm < W:
                    termes.append((e, 2 * Gm / W - 1))
                if p0 > 0:
                    crit.append((prec, e, m0, prec, p0))
                    if Gm == W and m0 < W and T1 is None:
                        T1 = e
            for C, p in taux.items():
                if p > 0:
                    masse[C] += p * dt
        for u in naissances.get(e, ()):
            tot = Fraction(0)
            for ch in T.children[u]:
                if ch in masse:
                    tot += masse.pop(ch)
                    taux.pop(ch, None)
                    if OC == ch:
                        OC = u
            masse[u] = tot
        for C in masse:
            c, Av, om = porteurs[C]
            taux[C] = _taux(c, Av, om, T.death[C], E2, e)
        if T_half is None:
            gagnants = [C for C, m in masse.items() if 2 * m > W]
            if gagnants:
                exiger(len(gagnants) == 1, 'majorite non exclusive')
                T_half, O, OC = e, gagnants[0], gagnants[0]
        prec = e
    exiger(T_half is not None, 'majorite jamais atteinte')
    date, arg = _date(T_half, termes, crit, T1, W, S, kappa)
    return {'date': date, 'O': O, 'T_half': T_half, 'W': W, 'E2': E2, 'argmax': arg, 'termes': termes}


def mmt_pondere_lent(D, porteurs, S, E2, kappa):
    """Reference directe (recalcul complet des masses a chaque evenement) : recoupe du balayage."""
    T = D.T
    kappa = Fraction(kappa)
    W = Fraction(0)
    for v, (c, Av, om) in porteurs.items():
        W += _masse(c, Av, om, T.death[v], E2, E2)
    if W == 0:
        return None
    evs = {E2}
    for v, (c, Av, om) in porteurs.items():
        evs.add(c)
        evs.add(T.birth[v])
        if Av is not None and Av > c:
            evs.add(Av)
        u = v
        while T.parent[u] >= 0:
            u = T.parent[u]
            evs.add(T.birth[u])
    evs = sorted(evs)
    vs = list(porteurs)

    def etat(s):
        out = {}
        for v in vs:
            if T.birth[v] > s:
                continue
            c, Av, om = porteurs[v]
            C = D.anc(v, s)
            m = _masse(c, Av, om, T.death[v], E2, s)
            old = out.get(C, (Fraction(0), Fraction(0)))
            out[C] = (old[0] + m, old[1])
        for C in list(out):
            p = Fraction(0)
            if C in porteurs:
                c, Av, om = porteurs[C]
                p = _taux(c, Av, om, T.death[C], E2, s)
            out[C] = (out[C][0], p)
        return out

    demi = W / 2
    T_half = O = None
    termes, crit = [], []
    T1 = None
    prec = None
    for e in evs:
        if prec is not None:
            s0, ms = prec
            if T_half is None:
                for C, (m, p) in ms.items():
                    if p > 0 and m <= demi < m + p * (e - s0):
                        exiger(T_half is None, 'franchissement non exclusif')
                        T_half, O = s0 + (demi - m) / p, C
                if T_half is not None:
                    m0, p0 = ms[O]
                    Gm = m0 + p0 * (e - s0)
                    if Gm < W:
                        termes.append((e, 2 * Gm / W - 1))
                    crit.append((T_half, e, m0, s0, p0))
                    if Gm == W and T1 is None:
                        T1 = s0 + (W - m0) / p0
            else:
                OC = D.anc(O, s0)
                m0, p0 = ms[OC]
                Gm = m0 + p0 * (e - s0)
                if Gm < W:
                    termes.append((e, 2 * Gm / W - 1))
                if p0 > 0:
                    crit.append((s0, e, m0, s0, p0))
                    if Gm == W and T1 is None and m0 < W:
                        T1 = s0 + (W - m0) / p0
        ms = etat(e)
        if T_half is None:
            gagnants = [C for C, (m, _p) in ms.items() if 2 * m > W]
            if gagnants:
                exiger(len(gagnants) == 1, 'majorite non exclusive')
                T_half, O = e, gagnants[0]
        prec = (e, ms)
    exiger(T_half is not None, 'majorite jamais atteinte')
    date, arg = _date(T_half, termes, crit, T1, W, S, kappa)
    return {'date': date, 'O': O, 'T_half': T_half, 'W': W, 'E2': E2, 'argmax': arg, 'termes': termes}


# ------------------------------------------------------------------ regles completes

class Regle:
    def __init__(self, nom, h, details, params):
        self.nom, self.h, self.details, self.params = nom, h, details, params


def _repli_racine(D, x, A):
    T = D.T
    root = T.root
    lvl = max(T.birth[root], D.cov[x][root])
    if A[root] is not None:
        lvl = max(lvl, A[root])
    return QS.sqrt(lvl), root


def mmta(sc, mcs, kappa=KAPPA, eta=ETA, lam=LAM, dur=False, D=None):
    """Regle MMtA (ou sa variante dure) sur la scene sc, pour min_cluster_size = mcs."""
    exiger(isinstance(mcs, int) and mcs >= 1, 'mcs entier >= 1')
    if D is None:
        D = Donnees(sc)
    _a, A = D.admissibilite(mcs)
    dates, owners, det = [None] * sc.n, [None] * sc.n, [None] * sc.n
    for x in range(sc.n):
        porteurs, Ax, Ahat, E2 = poids_point(D, x, mcs, eta, lam, dur)
        r = mmt_pondere(D, porteurs, Ax, E2, kappa) if Ahat is not None else None
        info = {'Ax': Ax, 'Ahat': Ahat, 'E2': E2,
                'omega': {v: str(om) for v, (c, Av, om) in porteurs.items() if Av is None or Av > c}}
        if r is None:
            date, O = _repli_racine(D, x, A)
            info['repli_racine'] = True
        else:
            date, O = r['date'], r['O']
            info.update({'T_half': r['T_half'], 'W': r['W'], 'argmax': r['argmax']})
            if A[O] is not None and QS.sqrt(A[O]).cmp(date) > 0:
                date = QS.sqrt(A[O])
                info['plancher_admissibilite'] = A[O]
        rd = Rayon.somme(date)
        dates[x], owners[x] = rd, D.anc_rayon(O, rd)
        det[x] = info
    nom = ('MMtA_dur[k=%s,e=%s;mcs=%d]' % (Fraction(kappa), Fraction(eta), mcs) if dur else
           'MMtA[k=%s,e=%s,l=%s;mcs=%d]' % (Fraction(kappa), Fraction(eta), Fraction(lam), mcs))
    h = RG.Hierarchie(sc, nom, 'principe_libre', sc.foret, dates, owners)
    return Regle(nom, h, det, {'mcs': mcs, 'kappa': Fraction(kappa), 'eta': Fraction(eta),
                               'lam': None if dur else Fraction(lam), 'dur': dur})


def niveau_coeur(D, x, mcs, variante):
    """r_c(x) : premier niveau >= d_K(x)^2 ou la composante qui contient x peut porter un cluster ; None sinon.
    noyau : au moins mcs points de X dans la composante ; amas : amas discret d'au moins mcs points."""
    k0 = D.coeur[x]
    if variante == 'noyau':
        _ac, Ac = D.admissibilite_coeur(mcs)
        L = Ac[k0]
    else:
        _a, A = D.admissibilite(mcs)
        L = A[k0]
    return None if L is None else max(D.dk2[x], L)


def mmta_cr(sc, mcs, kappa=KAPPA, eta=ETA, lam=LAM, variante='noyau', D=None):
    """MMtA_CR : CR_mcs par construction. r_c = niveau_coeur ; Cc = la composante qui contient x a r_c. Le profil
    de x est restreint aux noeuds comparables a Cc ; MMtA sur ce profil ; date = min(max(t, A(O)), r_c)."""
    exiger(isinstance(mcs, int) and mcs >= 1, 'mcs entier >= 1')
    exiger(variante in ('noyau', 'amas'), 'variante CR inconnue')
    if D is None:
        D = Donnees(sc)
    _a, A = D.admissibilite(mcs)
    dates, owners, det = [None] * sc.n, [None] * sc.n, [None] * sc.n
    for x in range(sc.n):
        rc = niveau_coeur(D, x, mcs, variante)
        cv = D.cov[x]
        Cc = None
        if rc is not None:
            Cc = D.anc(D.coeur[x], rc)
            cv = {v: c for v, c in cv.items() if D.sous(v, Cc) or D.sous(Cc, v)}
            exiger(Cc in cv, 'composante coeur hors du profil de couverture')
        porteurs, Ax, Ahat, E2 = poids_point(D, x, mcs, eta, lam, False, cv)
        r = mmt_pondere(D, porteurs, Ax, E2, kappa) if Ahat is not None else None
        info = {'rc': rc, 'Ax': Ax, 'Ahat': Ahat, 'E2': E2}
        if r is None:
            if rc is not None:
                date, O = QS.sqrt(rc), Cc
                info['repli'] = 'coeur'
            else:
                date, O = _repli_racine(D, x, A)
                info['repli_racine'] = True
        else:
            date, O = r['date'], r['O']
            if A[O] is not None and QS.sqrt(A[O]).cmp(date) > 0:
                date = QS.sqrt(A[O])
            if rc is not None and (r['T_half'] >= rc or date.cmp(QS.sqrt(rc)) >= 0):
                date, O = QS.sqrt(rc), Cc
                info['plafond_coeur'] = True
        rd = Rayon.somme(date)
        dates[x], owners[x] = rd, D.anc_rayon(O, rd)
        det[x] = info
    nom = 'MMtA_CR%s[k=%s,e=%s,l=%s;mcs=%d]' % ('n' if variante == 'noyau' else 'a', Fraction(kappa), Fraction(eta),
                                              Fraction(lam), mcs)
    h = RG.Hierarchie(sc, nom, 'principe_libre', sc.foret, dates, owners)
    return Regle(nom, h, det, {'mcs': mcs, 'kappa': Fraction(kappa), 'eta': Fraction(eta), 'lam': Fraction(lam),
                               'dur': False, 'cr': variante})


def controler(sc, regle, D=None):
    """Invariants exacts : Hierarchie.valider ; NP ancree (le proprietaire couvre x a sa date) ; admissibilite
    (le proprietaire est admissible a sa date, sauf repli racine). Rend le nombre de controles."""
    if D is None:
        D = Donnees(sc)
    _a, A = D.admissibilite(regle.params['mcs'])
    n = regle.h.valider()
    for x in range(sc.n):
        t = regle.h.dates[x]
        o = regle.h.proprietaires[x]
        c = D.cov[x].get(o)
        exiger(c is not None and Rayon(b2=c).cmp(t) <= 0,
               '%s : le proprietaire de %s ne le couvre pas a sa date' % (regle.nom, sc.noms[x]))
        if not regle.details[x].get('repli_racine'):
            exiger(A[o] is not None and Rayon(b2=A[o]).cmp(t) <= 0,
                   '%s : proprietaire de %s non admissible a sa date' % (regle.nom, sc.noms[x]))
        n += 2
    return n


def verifier_cr(sc, h, D, mcs, variante='noyau'):
    """Respect du coeur, controle exhaustif aux rayons critiques (changements de la hierarchie, naissances FULL,
    niveaux d'admissibilite, niveaux d_K) : pour tout x et tout r >= d_K(x), soit C la composante qui contient x ;
    CR exige que x soit entre et etiquete C ; CR_mcs l'exige seulement si C peut porter un cluster a r (noyau : au
    moins mcs points de X dans C ; amas : amas discret d'au moins mcs points). Rend (CR_mcs, CR, viol CR_mcs, viol CR)."""
    T = D.T
    if variante == 'noyau':
        a, _A = D.admissibilite_coeur(mcs)
    else:
        a, _A = D.admissibilite(mcs)
    crit = list(h.rayons_de_changement())
    crit += [Rayon(b2=T.birth[v]) for v in range(D.nn)]
    crit += [Rayon(b2=a[v]) for v in range(D.nn) if a[v] is not None]
    crit += [Rayon(b2=D.dk2[x]) for x in range(sc.n)]
    crit = RG.trier_uniques(crit)
    ok_m, ok_c, v_m, v_c = True, True, None, None
    for r in crit:
        for x in range(sc.n):
            if Rayon(b2=D.dk2[x]).cmp(r) > 0:
                continue
            C = D.anc_rayon(D.coeur[x], r)
            entre = h.dates[x].cmp(r) <= 0
            if entre and D.anc_rayon(h.proprietaires[x], r) == C:
                continue
            viol = {'point': sc.noms[x], 'rayon': r.json(), 'composante_coeur': C}
            if ok_c:
                ok_c, v_c = False, viol
            if a[C] is not None and Rayon(b2=a[C]).cmp(r) <= 0 and ok_m:
                ok_m, v_m = False, viol
    return ok_m, ok_c, v_m, v_c
